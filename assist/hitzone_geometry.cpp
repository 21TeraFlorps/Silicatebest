#include "hitzone_geometry.hpp"

#include <cmath>

using namespace geode::prelude;

HitzoneBox HitzoneBox::fromOBB2D(const OBB2D& obb) {
    HitzoneBox ret;
    ret.center = obb.m_center;
    ret.unitX = (obb.m_corners[1] - obb.m_corners[0]) / 2;
    ret.unitY = (obb.m_corners[3] - obb.m_corners[0]) / 2;
    return ret;
}

static void calculateMinkowskiDifferenceOnCorner(
    const HitzoneBox& box, const CCPoint& center, const CCPoint& cornerX,
    const CCPoint& cornerY, std::vector<CCPoint>& out) {
    auto cornerVec = cornerX + cornerY;
    auto cornerNorm = (cornerX.normalize() + cornerY.normalize()).normalize();

    auto unitX = box.unitX;
    auto unitY = box.unitY;

    if (cornerNorm.dot(unitX) < 0) unitX = -unitX;
    if (cornerNorm.dot(unitY) < 0) unitY = -unitY;

    auto dotX = std::abs(cornerNorm.dot(box.unitX.normalize()));
    auto dotY = std::abs(cornerNorm.dot(box.unitY.normalize()));

    auto cornerPos = center + cornerVec;
    auto cornerNormPerp = ccpPerp(cornerNorm);

    bool positiveFirst;
    if (dotX > dotY)
        positiveFirst = cornerNormPerp.dot(unitY) >= 0.0f;
    else
        positiveFirst = cornerNormPerp.dot(unitX) >= 0.0f;

    bool isOnePoint = std::abs(dotX - dotY) < 0.0001f;

    auto positivePoint = cornerPos + unitX + unitY;

    if (isOnePoint || positiveFirst) out.push_back(positivePoint);

    if (isOnePoint) return;

    if (dotX > dotY)
        out.push_back(cornerPos + unitX - unitY);
    else
        out.push_back(cornerPos - unitX + unitY);

    if (!positiveFirst) out.push_back(positivePoint);
}

std::vector<CCPoint> calculateMinkowskiDifference(const HitzoneBox& box0,
                                                  const HitzoneBox& box1) {
    std::vector<CCPoint> points;

    calculateMinkowskiDifferenceOnCorner(box1, box0.center, -box0.unitX,
                                        -box0.unitY, points);
    calculateMinkowskiDifferenceOnCorner(box1, box0.center, -box0.unitX,
                                        box0.unitY, points);
    calculateMinkowskiDifferenceOnCorner(box1, box0.center, box0.unitX,
                                        box0.unitY, points);
    calculateMinkowskiDifferenceOnCorner(box1, box0.center, box0.unitX,
                                        -box0.unitY, points);

    return points;
}
