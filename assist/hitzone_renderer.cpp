#include "hitzone_renderer.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

using namespace geode::prelude;

static float fullscreenQuad[] = {
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,

    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
};

HitzoneUnionRenderer::~HitzoneUnionRenderer() {
    if (m_vertexBuffer) glDeleteBuffers(1, &m_vertexBuffer);
    if (m_fullscreenQuadBuffer) glDeleteBuffers(1, &m_fullscreenQuadBuffer);
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_framebufferTexture) m_framebufferTexture->release();
}

bool HitzoneUnionRenderer::init() {
    setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(
        kCCShader_PositionColor));
    m_fullscreenShader = CCShaderCache::sharedShaderCache()->programForKey(
        kCCShader_PositionTexture);

    ensureCapacity(256);

    glGenBuffers(1, &m_fullscreenQuadBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenQuadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), fullscreenQuad,
                 GL_STATIC_DRAW);

    return true;
}

void HitzoneUnionRenderer::clear() {
    for (auto& layer : m_layers) layer.clear();
    m_dirty = true;
}

void HitzoneUnionRenderer::appendTriangles(
    std::vector<Vertex>& target, std::span<const CCPoint> points,
    const ccColor4F& colorFloat) {
    if (points.size() < 3) return;

    CCPoint anchor = points[0];
    auto color = ccc4BFromccc4F(colorFloat);

    target.reserve(target.size() + (points.size() - 2) * 3);

    for (size_t i = 1; i < points.size() - 1; i++) {
        target.push_back({anchor, color});
        target.push_back({points[i], color});
        target.push_back({points[i + 1], color});
    }
}

void HitzoneUnionRenderer::addBorderPolygon(
    std::span<const CCPoint> points, const ccColor4F& color, Group group) {
    appendTriangles(m_layers[static_cast<size_t>(group)].border, points, color);
    m_dirty = true;
}

void HitzoneUnionRenderer::addInsetFillPolygon(
    std::span<const CCPoint> points, const ccColor4F& color,
    float insetAmount, Group group) {
    if (points.size() < 3) return;

    std::vector<CCPoint> insetPoints;
    insetPoints.reserve(points.size());

    for (size_t i = 0; i < points.size(); i++) {
        const auto& point = points[i];
        const auto& prevPoint =
            points[i == 0 ? (points.size() - 1) : (i - 1)];
        const auto& nextPoint =
            points[i >= points.size() - 1 ? 0 : (i + 1)];

        auto prevVec = prevPoint - point;
        auto nextVec = nextPoint - point;

        if (prevVec.getLengthSq() <= 0.0f || nextVec.getLengthSq() <= 0.0f) {
            insetPoints.push_back(point);
            continue;
        }

        float angle = CC_RADIANS_TO_DEGREES(ccpAngle(prevVec, nextVec));
        float halfAngle = (180.0f - angle) / 2.0f;
        float cosHalf = std::cos(CC_DEGREES_TO_RADIANS(halfAngle));

        if (std::abs(cosHalf) < 0.0001f) {
            insetPoints.push_back(point);
            continue;
        }

        float inset = insetAmount / cosHalf;
        auto insetPoint =
            point + (prevVec.normalize() + nextVec.normalize()).normalize() *
                        inset;
        insetPoints.push_back(insetPoint);
    }

    appendTriangles(m_layers[static_cast<size_t>(group)].fill, insetPoints,
                    color);
    m_dirty = true;
}

void HitzoneUnionRenderer::ensureCapacity(uint32_t capacity) {
    if (capacity <= m_vertexBufferCapacity) return;

    uint32_t newCapacity = 1;
    while (newCapacity < capacity) newCapacity *= 2;
    capacity = newCapacity;

    m_vertexBufferCapacity = capacity;

    if (!m_vertexBuffer) glGenBuffers(1, &m_vertexBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * capacity, nullptr,
                 GL_STREAM_DRAW);

    m_dirty = true;
}

void HitzoneUnionRenderer::draw() {
    if (!prepareFramebuffer()) return;

    CC_NODE_DRAW_SETUP();

    auto* program = getShaderProgram();
    program->setUniformsForBuiltins();

    kmMat4 matrixP;
    kmMat4 matrixMV;
    kmMat4 matrixMVP;

    kmGLGetMatrix(KM_GL_PROJECTION, &matrixP);
    kmGLGetMatrix(KM_GL_MODELVIEW, &matrixMV);

    kmMat4Multiply(&matrixMVP, &matrixP, &matrixMV);

    program->setUniformLocationWithMatrix4fv(
        program->getUniformLocationForName(kCCUniformPMatrix_s), matrixP.mat,
        1);
    program->setUniformLocationWithMatrix4fv(
        program->getUniformLocationForName(kCCUniformMVMatrix_s), matrixMV.mat,
        1);
    program->setUniformLocationWithMatrix4fv(
        program->getUniformLocationForName(kCCUniformMVPMatrix_s),
        matrixMVP.mat, 1);

    glDisable(GL_BLEND);
    int32_t prevFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawn in this order so hazards end up on top of solids, matching the
    // reference implementation. The offscreen buffer keeps the union crisp.
    const std::array<std::vector<Vertex> const*, 4> segments = {
        &m_layers[static_cast<size_t>(Group::Solid)].border,
        &m_layers[static_cast<size_t>(Group::Solid)].fill,
        &m_layers[static_cast<size_t>(Group::Hazard)].border,
        &m_layers[static_cast<size_t>(Group::Hazard)].fill,
    };

    size_t total = 0;
    for (auto* segment : segments) total += segment->size();

    if (total > 0) {
        ensureCapacity(static_cast<uint32_t>(total));

        if (m_dirty) {
            glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

            size_t offset = 0;
            for (auto* segment : segments) {
                if (!segment->empty()) {
                    glBufferSubData(
                        GL_ARRAY_BUFFER,
                        static_cast<GLintptr>(offset * sizeof(Vertex)),
                        static_cast<GLsizeiptr>(segment->size() *
                                                sizeof(Vertex)),
                        segment->data());
                }
                offset += segment->size();
            }

            m_dirty = false;
        }

        ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position |
                                kCCVertexAttribFlag_Color);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

        glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), nullptr);
        glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE,
                              GL_TRUE, sizeof(Vertex),
                              (void*)offsetof(Vertex, color));

        size_t offset = 0;
        for (auto* segment : segments) {
            if (!segment->empty()) {
                glDrawArrays(GL_TRIANGLES, static_cast<GLint>(offset),
                             static_cast<GLsizei>(segment->size()));
            }
            offset += segment->size();
        }
    }

    glEnable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, prevFramebuffer);

    float matrix[] = {2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, -1, -1, 0, 1};

    m_fullscreenShader->use();
    m_fullscreenShader->setUniformsForBuiltins();
    m_fullscreenShader->setUniformLocationWithMatrix4fv(
        m_fullscreenShader->getUniformLocationForName(kCCUniformMVPMatrix_s),
        matrix, 1);

    ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ccGLBindTexture2D(m_framebufferTexture->getName());

    ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position |
                            kCCVertexAttribFlag_TexCoords);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenQuadBuffer);

    glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 2, nullptr);
    glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 2,
                          (void*)(sizeof(float) * 8));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool HitzoneUnionRenderer::prepareFramebuffer() {
    CCSize screenSize = CCEGLView::get()->getFrameSize();

    if (screenSize.width <= 0.0f || screenSize.height <= 0.0f) {
        m_lastScreenSize = screenSize;
        return false;
    }

    if (screenSize == m_lastScreenSize) return true;

    m_lastScreenSize = screenSize;

    if (m_framebufferTexture) {
        if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
        m_framebufferTexture->release();
        m_framebufferTexture = nullptr;
        m_fbo = 0;
    }

    m_framebufferTexture = new CCTexture2D;
    m_framebufferTexture->initWithData(
        nullptr, cocos2d::kCCTexture2DPixelFormat_RGBA8888, screenSize.width,
        screenSize.height, screenSize);
    m_framebufferTexture->retain();

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_framebufferTexture->getName(), 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}
