#pragma once

#include <Geode/Geode.hpp>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

// Renders a set of convex polygons into an offscreen buffer and then composites
// that buffer over the scene. Overlapping polygons therefore form a true union
// instead of stacking their alpha values. Solid and hazard polygons live in
// separate layers so hazards are always composited on top of solids.
class HitzoneUnionRenderer : public cocos2d::CCNode {
   public:
    enum class Group {
        Solid,
        Hazard,
    };

    ~HitzoneUnionRenderer() override;

    bool init() override;
    void draw() override;

    CREATE_FUNC(HitzoneUnionRenderer);

    void addBorderPolygon(std::span<const cocos2d::CCPoint> points,
                          const cocos2d::ccColor4F& color, Group group);

    void addInsetFillPolygon(std::span<const cocos2d::CCPoint> points,
                             const cocos2d::ccColor4F& color,
                             float insetAmount, Group group);

    void clear();

   private:
    struct Vertex {
        cocos2d::CCPoint position;
        cocos2d::ccColor4B color;
    };

    struct Layer {
        std::vector<Vertex> border;
        std::vector<Vertex> fill;

        void clear() {
            border.clear();
            fill.clear();
        }
    };

    void appendTriangles(std::vector<Vertex>& target,
                         std::span<const cocos2d::CCPoint> points,
                         const cocos2d::ccColor4F& color);

    void ensureCapacity(uint32_t capacity);
    bool prepareFramebuffer();

   private:
    bool m_dirty = false;

    std::array<Layer, 2> m_layers;

    uint32_t m_vertexBuffer = 0;
    uint32_t m_vertexBufferCapacity = 0;

    cocos2d::CCTexture2D* m_framebufferTexture = nullptr;
    unsigned int m_fbo = 0;
    cocos2d::CCSize m_lastScreenSize = {0, 0};
    uint32_t m_fullscreenQuadBuffer = 0;
    cocos2d::CCGLProgram* m_fullscreenShader = nullptr;
};
