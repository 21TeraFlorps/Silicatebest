#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/OBB2D.hpp>
#include <vector>

// Oriented box used for the Minkowski difference between an object and the
// player hitbox.
struct HitzoneBox {
    cocos2d::CCPoint center, unitX, unitY;

    static HitzoneBox fromOBB2D(const OBB2D& obb);
};

// Returns the convex polygon of player-center positions that overlap the
// object box `box0` for a player with box `box1`.
std::vector<cocos2d::CCPoint> calculateMinkowskiDifference(
    const HitzoneBox& box0, const HitzoneBox& box1);
