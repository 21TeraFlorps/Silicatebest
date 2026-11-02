#include "hitzones.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/LevelSettingsObject.hpp>
#include <Geode/binding/OBB2D.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/cocos/CCDirector.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "hitzone_geometry.hpp"
#include "hitzone_renderer.hpp"
#include "settings/settings.hpp"

using namespace geode::prelude;

static constexpr float kPi = 3.14159265358979323846f;
static constexpr int kCircleSegments = 48;

static std::vector<CCPoint> generateRect(const CCRect& rect) {
    return {
        ccp(rect.getMinX(), rect.getMinY()),
        ccp(rect.getMinX(), rect.getMaxY()),
        ccp(rect.getMaxX(), rect.getMaxY()),
        ccp(rect.getMaxX(), rect.getMinY()),
    };
}

static std::vector<CCPoint> generateCircle(const CCPoint& center,
                                           float radius) {
    std::vector<CCPoint> points;
    points.reserve(kCircleSegments);

    for (int i = 0; i < kCircleSegments; i++) {
        float angle = static_cast<float>(i) /
                      static_cast<float>(kCircleSegments) * kPi * 2.0f;
        points.push_back(
            CCPoint(std::cos(angle), std::sin(angle)) * radius + center);
    }

    return points;
}

static bool doIntersect(CCPoint p1, CCPoint q1, CCPoint p2, CCPoint q2,
                        CCPoint& result) {
    double a1 = q1.y - p1.y;
    double b1 = p1.x - q1.x;
    double c1 = a1 * p1.x + b1 * p1.y;

    double a2 = q2.y - p2.y;
    double b2 = p2.x - q2.x;
    double c2 = a2 * p2.x + b2 * p2.y;

    double determinant = a1 * b2 - a2 * b1;

    if (determinant != 0) {
        result.x = static_cast<float>((c1 * b2 - c2 * b1) / determinant);
        result.y = static_cast<float>((a1 * c2 - a2 * c1) / determinant);
        return true;
    }

    return false;
}

static cocos2d::CCRect polygonBounds(
    std::span<const cocos2d::CCPoint> points) {
    if (points.empty()) return {};

    float minX = points[0].x;
    float maxX = points[0].x;
    float minY = points[0].y;
    float maxY = points[0].y;

    for (auto const& point : points) {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }

    return {{minX, minY}, {maxX - minX, maxY - minY}};
}

static HitzonePolygon makePolygon(std::vector<CCPoint> points,
                                  HitzoneCategory category) {
    HitzonePolygon polygon;
    polygon.points = std::move(points);
    polygon.category = category;
    polygon.bounds = polygonBounds(polygon.points);
    return polygon;
}

static uint64_t hashPolygon(const HitzonePolygon& polygon) {
    uint64_t hash = 1469598103934665603ull;

    auto mix = [&hash](uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ull;
    };

    mix(static_cast<uint64_t>(polygon.category));
    mix(static_cast<uint64_t>(polygon.points.size()));

    for (auto const& point : polygon.points) {
        mix(static_cast<uint64_t>(
            std::llround(static_cast<double>(point.x) * 16.0)));
        mix(static_cast<uint64_t>(
            std::llround(static_cast<double>(point.y) * 16.0)));
    }

    return hash;
}

static uint64_t computeObjectKey(uint64_t levelKey, GameObject* object) {
    uint64_t hash = levelKey;

    auto mix = [&hash](uint64_t value) {
        hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    };

    mix(static_cast<uint64_t>(object->m_objectID));
    mix(static_cast<uint64_t>(object->m_uniqueID));
    mix(static_cast<uint64_t>(std::llround(
        static_cast<double>(object->m_startPosition.x) * 16.0)));
    mix(static_cast<uint64_t>(std::llround(
        static_cast<double>(object->m_startPosition.y) * 16.0)));

    return hash;
}

static uint64_t computeLevelKey(GJBaseGameLayer* pl) {
    auto level = pl ? pl->m_level : nullptr;
    if (!level) return 0;

    uint64_t hash = 1469598103934665603ull;

    auto mix = [&hash](uint64_t value) {
        hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    };

    mix(static_cast<uint64_t>(level->m_levelID.value()));
    mix(static_cast<uint64_t>(std::hash<std::string_view>{}(
        std::string_view(level->m_levelName))));
    mix(static_cast<uint64_t>(std::hash<std::string_view>{}(
        std::string_view(level->m_levelString))));

    return hash;
}

static cocos2d::CCRect objectSpaceViewport(cocos2d::CCNode* layer) {
    if (!layer) return {};

    auto director = cocos2d::CCDirector::get();
    const auto size = director->getWinSize();
    const auto worldToObject = layer->worldToNodeTransform();

    const std::array<cocos2d::CCPoint, 4> corners = {
        cocos2d::CCPointApplyAffineTransform({0.f, 0.f}, worldToObject),
        cocos2d::CCPointApplyAffineTransform({size.width, 0.f}, worldToObject),
        cocos2d::CCPointApplyAffineTransform({0.f, size.height}, worldToObject),
        cocos2d::CCPointApplyAffineTransform({size.width, size.height},
                                             worldToObject),
    };

    float minX = corners[0].x;
    float maxX = corners[0].x;
    float minY = corners[0].y;
    float maxY = corners[0].y;
    for (auto const& corner : corners) {
        minX = std::min(minX, corner.x);
        maxX = std::max(maxX, corner.x);
        minY = std::min(minY, corner.y);
        maxY = std::max(maxY, corner.y);
    }

    return {{minX, minY}, {maxX - minX, maxY - minY}};
}

static void iterateObjects(GJBaseGameLayer* pl,
                           std::function<void(GameObject*)> const& callback) {
    auto& sections = pl->m_sections;

    for (int i = pl->m_leftSectionIndex;
         i <= pl->m_rightSectionIndex &&
         static_cast<size_t>(i) < sections.size();
         i++) {
        auto horizontalSection = sections[i];
        if (!horizontalSection) continue;

        for (int j = pl->m_bottomSectionIndex;
             j <= pl->m_topSectionIndex &&
             static_cast<size_t>(j) < horizontalSection->size();
             j++) {
            auto section = horizontalSection->at(j);
            if (!section) continue;
            if (static_cast<size_t>(i) >= pl->m_sectionSizes.size() ||
                !pl->m_sectionSizes[i] ||
                static_cast<size_t>(j) >= pl->m_sectionSizes[i]->size()) {
                continue;
            }

            for (int k = 0; k < pl->m_sectionSizes[i]->at(j); k++) {
                auto object = section->at(k);
                if (!object) continue;

                callback(object);
            }
        }
    }
}

static void generateSolidPolygon(GameObject* object, PlayerObject* player,
                                 std::vector<CCPoint>& points) {
    CCRect hitbox = object->getObjectRect();
    const auto playerRect = player->getObjectRect();

    const CCPoint halfPlayer = {playerRect.size.width / 2.0f,
                                playerRect.size.height / 2.0f};

    hitbox.origin = hitbox.origin - halfPlayer;
    hitbox.size = {hitbox.size.width + playerRect.size.width,
                   hitbox.size.height + playerRect.size.height};

    if (object->m_isPassable) {
        if (player->m_isSideways) {
            hitbox.origin += player->m_isUpsideDown
                                 ? CCPoint(0, 0)
                                 : CCPoint(hitbox.size.width - 1, 0);
            hitbox.size = {1, hitbox.size.height};
        } else {
            hitbox.origin += player->m_isUpsideDown
                                 ? CCPoint(0, 0)
                                 : CCPoint(0, hitbox.size.height - 1);
            hitbox.size = {hitbox.size.width, 1};
        }
    }

    points = generateRect(hitbox);
}

static bool generateSlopePolygon(GameObject* object, PlayerObject* player,
                                 std::vector<CCPoint>& points) {
    const CCRect hitbox = object->getObjectRect();
    const auto playerRect = player->getObjectRect();
    const CCPoint playerSize = {playerRect.size.width,
                                playerRect.size.height};
    const CCPoint halfPlayer = {playerSize.x / 2.0f, playerSize.y / 2.0f};

    auto cornerVec = CCPoint(hitbox.getMaxX(), hitbox.getMinY()) -
                     object->getRealPosition();
    auto vecX = CCPoint(-hitbox.size.width, 0);
    auto vecY = CCPoint(0, hitbox.size.height);

    auto slopeDir = CCPoint(-1.0f, 1.0f);

    if (object->slopeFloorTop() == object->m_slopeUphill) {
        vecX.x = -vecX.x;
        cornerVec.x = -cornerVec.x;
        slopeDir.x = -slopeDir.x;
    }

    if (object->slopeFloorTop()) {
        vecY.y = -vecY.y;
        cornerVec.y = -cornerVec.y;
        slopeDir.y = -slopeDir.y;
    }

    const auto corner = cornerVec + object->getRealPosition();

    const CCPoint hitboxCorner =
        corner - CCPoint(slopeDir.x * halfPlayer.x, slopeDir.y * halfPlayer.y);
    const CCPoint cornerX =
        corner + vecX + CCPoint(slopeDir.x * halfPlayer.x,
                                -slopeDir.y * halfPlayer.y);
    const CCPoint cornerY =
        corner + vecY + CCPoint(-slopeDir.x * halfPlayer.x,
                                slopeDir.y * halfPlayer.y);

    const CCPoint normalVec = {vecY.y, vecX.x};
    if (normalVec.getLengthSq() <= 0.0f) return false;

    const CCPoint slopeOffset =
        ((vecX.x < 0) != (vecY.y < 0)) ? -normalVec.normalize()
                                       : normalVec.normalize();

    const CCPoint slopeLine0 =
        corner + vecX + slopeOffset * (playerSize.y / 2.0f);
    const CCPoint slopeLine1 =
        corner + vecY + slopeOffset * (playerSize.y / 2.0f);

    CCPoint slopePointX;
    if (!doIntersect(slopeLine0, slopeLine1, cornerX,
                     cornerX + CCPoint(0, slopeDir.y), slopePointX)) {
        return false;
    }

    CCPoint slopePointY;
    if (!doIntersect(slopeLine0, slopeLine1, cornerY,
                     cornerY + CCPoint(slopeDir.x, 0), slopePointY)) {
        return false;
    }

    points.clear();
    points.push_back(hitboxCorner);
    points.push_back(cornerX);
    points.push_back(slopePointX);
    points.push_back(slopePointY);
    points.push_back(cornerY);
    return true;
}

static void generateHazardPolygons(GJBaseGameLayer* pl, GameObject* object,
                                   PlayerObject* player,
                                   std::vector<std::vector<CCPoint>>& out) {
    const CCRect playerRect = player->getObjectRect();

    if (object->m_shouldUseOuterOb) {
        auto objectBox = object->getOrientedBox();
        auto playerBox = player->getOrientedBox();
        if (objectBox && playerBox) {
            out.push_back(calculateMinkowskiDifference(
                HitzoneBox::fromOBB2D(*objectBox),
                HitzoneBox::fromOBB2D(*playerBox)));
            return;
        }
    }

    if (object->m_objectRadius <= 0.0f) {
        CCRect hitbox = object->getObjectRect();
        hitbox.origin =
            hitbox.origin - CCPoint(playerRect.size.width / 2.0f,
                                    playerRect.size.height / 2.0f);
        hitbox.size = {hitbox.size.width + playerRect.size.width,
                       hitbox.size.height + playerRect.size.height};
        out.push_back(generateRect(hitbox));
        return;
    }

    const auto realPosition = object->getRealPosition();
    const float radius = object->m_objectRadius;

    if (pl->m_levelSettings && pl->m_levelSettings->m_fixRadiusCollision) {
        out.push_back(generateCircle(
            realPosition, radius + playerRect.size.width / 2.0f));
        return;
    }

    const float playerHalfWidth = playerRect.size.width / 2.0f;
    const float playerHalfHeight = playerRect.size.height / 2.0f;

    out.push_back(generateCircle(
        realPosition + CCPoint(-playerHalfWidth, -playerHalfHeight), radius));
    out.push_back(generateCircle(
        realPosition + CCPoint(-playerHalfWidth, playerHalfHeight), radius));
    out.push_back(generateCircle(
        realPosition + CCPoint(playerHalfWidth, -playerHalfHeight), radius));
    out.push_back(generateCircle(
        realPosition + CCPoint(playerHalfWidth, playerHalfHeight), radius));
    out.push_back(generateRect(
        CCRect(realPosition - CCPoint(playerHalfWidth, playerHalfHeight),
               playerRect.size)));
}

Hitzones::HitzonesContainer* Hitzones::HitzonesContainer::create(
    Hitzones* owner, GJBaseGameLayer* gameLayer) {
    auto node = new HitzonesContainer();
    if (node && node->initContainer(owner, gameLayer)) {
        node->autorelease();
        return node;
    }

    CC_SAFE_DELETE(node);
    return nullptr;
}

bool Hitzones::HitzonesContainer::initContainer(Hitzones* owner,
                                                GJBaseGameLayer* gameLayer) {
    if (!CCNode::init() || !owner || !gameLayer) return false;

    m_owner = owner;
    m_gameLayer = gameLayer;
    return true;
}

void Hitzones::HitzonesContainer::visit() {
    if (m_owner) m_owner->draw(m_gameLayer);
    CCNode::visit();
}

void Hitzones::init(GJBaseGameLayer* pl) {
    if (m_initialized) {
        this->destroy();
    }

    if (!pl || !pl->m_objectLayer) return;

    m_levelKey = computeLevelKey(pl);

    m_container = HitzonesContainer::create(this, pl);
    if (!m_container) return;

    m_container->retain();
    m_container->setID("hitzone-layer"_spr);

    m_renderer = HitzoneUnionRenderer::create();
    if (!m_renderer) {
        CC_SAFE_RELEASE_NULL(m_container);
        return;
    }

    m_renderer->setID("hitzone-renderer"_spr);
    m_container->addChild(m_renderer, 0);
    pl->m_objectLayer->addChild(m_container, 5000);

    m_initialized = true;
}

void Hitzones::addPolygon(ObjectTrail& trail, HitzonePolygon&& polygon,
                          bool accumulate) {
    if (!accumulate) {
        trail.current.push_back(std::move(polygon));
        return;
    }

    auto hash = hashPolygon(polygon);
    if (!trail.seen.insert(hash).second) return;

    if (!trail.hasHistoryBounds) {
        trail.historyBounds = polygon.bounds;
        trail.hasHistoryBounds = true;
    } else {
        const float minX =
            std::min(trail.historyBounds.getMinX(), polygon.bounds.getMinX());
        const float minY =
            std::min(trail.historyBounds.getMinY(), polygon.bounds.getMinY());
        const float maxX =
            std::max(trail.historyBounds.getMaxX(), polygon.bounds.getMaxX());
        const float maxY =
            std::max(trail.historyBounds.getMaxY(), polygon.bounds.getMaxY());
        trail.historyBounds = {{minX, minY}, {maxX - minX, maxY - minY}};
    }

    trail.history.push_back(std::move(polygon));
}

void Hitzones::tick(GJBaseGameLayer* pl) {
    if (!m_initialized || !pl) return;
    if (!m_enabled->inner() && !m_showOnDeath->inner()) return;

    m_tickCount++;

    const bool accumulate = m_accumulate->inner();

    iterateObjects(pl, [&](GameObject* object) {
        if (!object || object == pl->m_player1 || object == pl->m_player2 ||
            object == pl->m_anticheatSpike) {
            return;
        }

        if (object->m_isGroupDisabled || !object->m_isActivated ||
            object->m_objectType == GameObjectType::Decoration) {
            return;
        }

        const bool isSolid = object->m_objectType == GameObjectType::Solid ||
                             object->m_objectType == GameObjectType::Slope;
        const bool isHazard =
            object->m_objectType == GameObjectType::Hazard ||
            object->m_objectType == GameObjectType::AnimatedHazard;

        if (!isSolid && !isHazard) return;

        auto& trail = m_trails[computeObjectKey(m_levelKey, object)];
        if (trail.levelKey != m_levelKey ||
            trail.objectID != object->m_objectID ||
            trail.uniqueID != object->m_uniqueID) {
            trail = ObjectTrail{};
            trail.levelKey = m_levelKey;
            trail.objectID = object->m_objectID;
            trail.uniqueID = object->m_uniqueID;
        }

        if (!accumulate) {
            trail.current.clear();
            trail.lastSeenTick = m_tickCount;
        }

        const std::array<PlayerObject*, 2> players = {
            pl->m_player1,
            pl->m_gameState.m_isDualMode ? pl->m_player2 : nullptr,
        };

        for (auto player : players) {
            if (!player) continue;

            if (isSolid) {
                std::vector<CCPoint> points;
                if (object->m_objectType == GameObjectType::Slope) {
                    if (!generateSlopePolygon(object, player, points)) {
                        continue;
                    }
                } else {
                    generateSolidPolygon(object, player, points);
                }

                if (points.size() >= 3) {
                    addPolygon(trail, makePolygon(std::move(points),
                                                  HitzoneCategory::Solid),
                               accumulate);
                }
                continue;
            }

            std::vector<std::vector<CCPoint>> polygons;
            generateHazardPolygons(pl, object, player, polygons);

            for (auto& points : polygons) {
                if (points.size() < 3) continue;
                addPolygon(
                    trail,
                    makePolygon(std::move(points), HitzoneCategory::Hazard),
                    accumulate);
            }
        }
    });
}

void Hitzones::drawPolygon(const HitzonePolygon& polygon,
                           const cocos2d::ccColor4F& border,
                           const cocos2d::ccColor4F& fill, float borderWidth,
                           bool hazard) {
    if (!m_renderer || polygon.points.size() < 3) return;

    const auto group = hazard ? HitzoneUnionRenderer::Group::Hazard
                              : HitzoneUnionRenderer::Group::Solid;

    m_renderer->addBorderPolygon(polygon.points, border, group);
    m_renderer->addInsetFillPolygon(polygon.points, fill, borderWidth, group);
}

void Hitzones::draw(GJBaseGameLayer* pl) {
    if (!m_initialized || !m_renderer || !pl) return;

    const bool visible =
        m_enabled->inner() ||
        (m_showOnDeath->inner() && pl->m_playerDied);

    m_renderer->setVisible(visible);
    m_renderer->clear();

    if (!visible || !pl->m_objectLayer) return;

    const auto viewport = objectSpaceViewport(pl->m_objectLayer);
    auto& settings = SLSettings::get()->hitzones;

    const float borderWidth =
        static_cast<float>(settings.borderWidth /
                           std::max(pl->m_gameState.m_cameraZoom, 0.0001f));

    const ccColor4F solidBorder = {
        settings.solidColor[0],
        settings.solidColor[1],
        settings.solidColor[2],
        settings.solidColor[3] *
            static_cast<float>(settings.borderOpacity),
    };
    const ccColor4F solidFill = {
        settings.solidColor[0],
        settings.solidColor[1],
        settings.solidColor[2],
        settings.solidColor[3] * static_cast<float>(settings.fillOpacity),
    };
    const ccColor4F hazardBorder = {
        settings.hazardColor[0],
        settings.hazardColor[1],
        settings.hazardColor[2],
        settings.hazardColor[3] *
            static_cast<float>(settings.borderOpacity),
    };
    const ccColor4F hazardFill = {
        settings.hazardColor[0],
        settings.hazardColor[1],
        settings.hazardColor[2],
        settings.hazardColor[3] * static_cast<float>(settings.fillOpacity),
    };

    const bool accumulate = m_accumulate->inner();

    for (auto& entry : m_trails) {
        auto& trail = entry.second;
        if (trail.levelKey != m_levelKey) continue;

        if (accumulate) {
            if (!trail.hasHistoryBounds ||
                !trail.historyBounds.intersectsRect(viewport)) {
                continue;
            }

            for (auto const& polygon : trail.history) {
                if (!polygon.bounds.intersectsRect(viewport)) continue;

                const bool hazard =
                    polygon.category == HitzoneCategory::Hazard;
                drawPolygon(polygon, hazard ? hazardBorder : solidBorder,
                            hazard ? hazardFill : solidFill, borderWidth,
                            hazard);
            }
        } else {
            if (trail.lastSeenTick != m_tickCount) continue;

            for (auto const& polygon : trail.current) {
                if (!polygon.bounds.intersectsRect(viewport)) continue;

                const bool hazard =
                    polygon.category == HitzoneCategory::Hazard;
                drawPolygon(polygon, hazard ? hazardBorder : solidBorder,
                            hazard ? hazardFill : solidFill, borderWidth,
                            hazard);
            }
        }
    }
}

void Hitzones::destroy() {
    if (!m_initialized) return;

    if (m_container) {
        m_container->removeFromParent();
        CC_SAFE_RELEASE_NULL(m_container);
    }

    m_renderer = nullptr;
    m_initialized = false;
}

void Hitzones::clearHistory() {
    m_trails.clear();
    if (m_renderer) m_renderer->clear();
}

size_t Hitzones::polygonCount() const {
    size_t total = 0;
    for (auto const& entry : m_trails) {
        total += entry.second.history.size();
    }
    return total;
}
