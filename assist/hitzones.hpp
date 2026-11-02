#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "settings/settings.hpp"
#include "shared/value/value.hpp"

class HitzoneUnionRenderer;

enum class HitzoneCategory {
    Solid,
    Hazard,
};

// One convex polygon in object-layer space. A polygon is the set of player
// center positions that would overlap the object while the object is in the
// state it had on a particular tick.
struct HitzonePolygon {
    std::vector<cocos2d::CCPoint> points;
    cocos2d::CCRect bounds;
    HitzoneCategory category = HitzoneCategory::Solid;
};

class Hitzones {
    class HitzonesContainer : public cocos2d::CCNode {
        Hitzones* m_owner = nullptr;
        GJBaseGameLayer* m_gameLayer = nullptr;

       public:
        static HitzonesContainer* create(Hitzones* owner,
                                         GJBaseGameLayer* gameLayer);
        bool initContainer(Hitzones* owner, GJBaseGameLayer* gameLayer);
        void visit() override;
    };

    struct ObjectTrail {
        uint64_t levelKey = 0;
        int objectID = 0;
        int uniqueID = 0;

        // Polygons accumulated across ticks. `seen` keeps backstepping and
        // stationary objects from adding the same geometry over and over.
        std::vector<HitzonePolygon> history;
        std::unordered_set<uint64_t> seen;
        cocos2d::CCRect historyBounds;
        bool hasHistoryBounds = false;

        // The exact geometry for the most recent tick. Used when accumulation
        // is temporarily disabled; it never replaces the saved history.
        std::vector<HitzonePolygon> current;
        uint64_t lastSeenTick = 0;
    };

    HitzonesContainer* m_container = nullptr;
    HitzoneUnionRenderer* m_renderer = nullptr;

    std::unordered_map<uint64_t, ObjectTrail> m_trails;
    uint64_t m_levelKey = 0;
    uint64_t m_tickCount = 0;

    void addPolygon(ObjectTrail& trail, HitzonePolygon&& polygon,
                    bool accumulate);
    void drawPolygon(const HitzonePolygon& polygon,
                     const cocos2d::ccColor4F& border,
                     const cocos2d::ccColor4F& fill, float borderWidth,
                     bool hazard);

   public:
    bool m_initialized = false;

    SLValuePtr<bool> m_enabled = SLValue<bool>::create(
        "hitzones.enabled", &SLSettings::get()->hitzones.enabled);
    SLValuePtr<bool> m_showOnDeath = SLValue<bool>::create(
        "hitzones.show_on_death", &SLSettings::get()->hitzones.showOnDeath);
    SLValuePtr<bool> m_accumulate = SLValue<bool>::create(
        "hitzones.accumulate", &SLSettings::get()->hitzones.accumulate);
    SLValuePtr<double> m_fillOpacity = SLValue<double>::create(
        "hitzones.fill_opacity", &SLSettings::get()->hitzones.fillOpacity);
    SLValuePtr<double> m_borderOpacity = SLValue<double>::create(
        "hitzones.border_opacity", &SLSettings::get()->hitzones.borderOpacity);
    SLValuePtr<double> m_borderWidth = SLValue<double>::create(
        "hitzones.border_width", &SLSettings::get()->hitzones.borderWidth);

    void init(GJBaseGameLayer* pl);
    void tick(GJBaseGameLayer* pl);
    void draw(GJBaseGameLayer* pl);
    void destroy();

    // The only way to remove accumulated history. Called by the Silicate UI
    // clear button (and implicitly when the process exits).
    void clearHistory();

    size_t polygonCount() const;
};
