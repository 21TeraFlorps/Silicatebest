#pragma once

#include <Windows.h>
// #include "backends/imgui_impl_win32.h"

#ifndef IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// #include "backends/imgui_impl_opengl3.h"
#include <Geode/Geode.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <tabby.hpp>

#include "bot/bot.hpp"
#include "imgui.h"
#include "render/pass.hpp"
#include "theme.hpp"
#include "ui/manager.hpp"

LRESULT CALLBACK h_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class ImGuiHookCtx {
   private:
    ImGuiHookCtx() = default;

    bool m_inited = false;

   public:
    constexpr static float BLUR_DOWNSCALING_FACTOR = 3.0f;

    tabby::Context m_ctx;
    HWND m_hWnd = nullptr;
    void* m_glfwWindow = nullptr;
    GLuint m_inputTex = 0;
    GLuint m_sceneTex = 0;
    GLuint m_postprocessInputTex = 0;
    GLint m_oldFbo;
    GLint m_oldProgram;
    GLint m_oldTex;
    GLint m_oldTex1;
    GLint m_oldActiveTexture;
    RenderPass m_blurPass;
    GLuint m_glassProgram = 0;

    float m_width;
    float m_height;
    float m_time;

    WNDPROC m_oWndProc = nullptr;
    ImVec4 m_windowPos;
    float m_cornerRadius = 0.0f;

    static ImGuiHookCtx& get() {
        static ImGuiHookCtx ctx;
        return ctx;
    }

    struct RenderData {
        cocos2d::CCSize m_size;
        cocos2d::CCPoint m_pos;
    };

    RenderData m_renderData;
    void init(cocos2d::CCEGLView* view);
    void reinitForNewWindow(cocos2d::CCEGLView* view, HDC hdc, HWND hWnd);

    void preSampleBlur(ImVec4 windowPos, float cornerRadius);
    void preSamplePostprocess(ImVec4 windowPos, float cornerRadius);
    void sampleBlurFirstPass();
    void sampleBlurSecondPass();
    void sampleBlurSnapshot();
    void sampleBlurPostprocessToInput();
    void samplePostprocessFullResolution();
    void sampleGlassToInput();
    void sampleGlassToBlur();

    void postSampleBlur();

    void handleResize(float width, float height);

    void draw();
};
