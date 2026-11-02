#include "hook.hpp"

#include <winuser.h>

#include <tabby.hpp>

#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "context/context.hpp"
#include "imgui.h"
#include "render/renderer.hpp"
#include "replay/system.hpp"
#include "shared/value/value.hpp"
#include "ui/manager.hpp"
#include "ui/widgets/info_modal.hpp"
#include "ui/widgets/yes_no_modal.hpp"

using namespace geode::prelude;

constexpr int KEY_MW_UP = 0x97;
constexpr int KEY_MW_DOWN = 0x98;

LRESULT CALLBACK h_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    bool shiftHeld = GetKeyState(VK_SHIFT) & 0x8000;
    bool ctrlHeld = GetKeyState(VK_CONTROL) & 0x8000;
    bool altHeld = GetKeyState(VK_MENU) & 0x8000;
    auto* bindings = SLBindingManager::get();

    if (uMsg == WM_SIZE && wParam != SIZE_MINIMIZED) {
        ImGuiHookCtx::get().handleResize(LOWORD(lParam), HIWORD(lParam));
    }

    if (uMsg == WM_KILLFOCUS) {
        bindings->releaseAllHeldKeybinds();
        if (!slui::isYesNoModalOpen() && !slui::isInfoModalOpen()) {
            Bot::get()->ui().m_state.m_visible->inner() = false;
        }
    }

    const bool isRelease =
        uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP || uMsg == WM_MBUTTONUP;
    const int releasedKey =
        uMsg == WM_MBUTTONUP ? VK_MBUTTON : static_cast<int>(wParam);
    if (isRelease && bindings->isKeyHeld(releasedKey)) {
        bindings->processKeyEvent(releasedKey, false, ctrlHeld, shiftHeld,
                                  altHeld);
    }

    if ((uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR) &&
        CCIMEDispatcher::sharedDispatcher()->hasDelegate() &&
        !ImGui::GetIO().WantCaptureKeyboard) {
        return CallWindowProc(ImGuiHookCtx::get().m_oWndProc, hWnd, uMsg,
                              wParam, lParam);
    }

    bool uiVisible = Bot::get()->ui().m_state.m_visible->inner() || slui::isInfoModalOpen();

    if (uiVisible) {
        ImGuiHookCtx::get().m_ctx.handleWndproc(hWnd, uMsg, wParam, lParam);

        const bool keyboardMessage =
            uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN || uMsg == WM_KEYUP ||
            uMsg == WM_SYSKEYUP || uMsg == WM_CHAR;
        const bool mouseMessage = uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST;
        if ((slui::isYesNoModalOpen() || slui::isInfoModalOpen()) && (keyboardMessage || mouseMessage)) {
            return true;
        }

        if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN || uMsg == WM_KEYUP ||
            uMsg == WM_SYSKEYUP || uMsg == WM_CHAR) {
            bool systemKey = uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP ||
                             wParam == VK_LWIN || wParam == VK_RWIN ||
                             wParam == VK_MENU || wParam == VK_LMENU ||
                             wParam == VK_RMENU;
            auto* b = bindings;

            bool isKeyboardRelease = uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP;
            if (b->wantsCapture()) {
                if (isKeyboardRelease) {
                    b->takeInput(static_cast<int>(wParam), shiftHeld, ctrlHeld,
                                 altHeld);
                }

                return true;
            } else {
                if (ImGui::GetIO().WantCaptureKeyboard && !systemKey) {
                    return true;
                }
            }
        }

        if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN ||
            uMsg == WM_MBUTTONDOWN || uMsg == WM_MOUSEWHEEL) {
            int key = uMsg == WM_MBUTTONDOWN ? VK_MBUTTON : wParam;

            if (uMsg == WM_MOUSEWHEEL) {
                if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) {
                    key = KEY_MW_UP;
                } else {
                    key = KEY_MW_DOWN;
                }
            }

            bindings->processKeyEvent(key, true, ctrlHeld, shiftHeld, altHeld);
            if (uMsg == WM_MOUSEWHEEL) {
                bindings->processKeyEvent(key, false, ctrlHeld, shiftHeld,
                                          altHeld);
            }
        } else if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP ||
                   uMsg == WM_MBUTTONUP) {
            int key = uMsg == WM_MBUTTONUP ? VK_MBUTTON : wParam;
            bindings->processKeyEvent(key, false, ctrlHeld, shiftHeld, altHeld);
        }

        // return true;

        if (ImGui::GetIO().WantCaptureMouse) {
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
                return true;
            }
        }
    } else {
        if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN ||
            uMsg == WM_MBUTTONDOWN || uMsg == WM_MOUSEWHEEL) {
            int key = uMsg == WM_MBUTTONDOWN ? VK_MBUTTON : wParam;

            if (uMsg == WM_MOUSEWHEEL) {
                if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) {
                    key = KEY_MW_UP;
                } else {
                    key = KEY_MW_DOWN;
                }
            }

            bindings->processKeyEvent(key, true, ctrlHeld, shiftHeld, altHeld);
            if (uMsg == WM_MOUSEWHEEL) {
                bindings->processKeyEvent(key, false, ctrlHeld, shiftHeld,
                                          altHeld);
            }
        } else if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP ||
                   uMsg == WM_MBUTTONUP) {
            int key = uMsg == WM_MBUTTONUP ? VK_MBUTTON : wParam;
            bindings->processKeyEvent(key, false, ctrlHeld, shiftHeld, altHeld);
        }
    }

    if (uMsg == WM_DROPFILES) {
        HDROP drop = (HDROP)wParam;

        TCHAR fileName[256];
        UINT numFiles = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);

        for (UINT i = 0; i < numFiles; i++) {
            auto& rs = Bot::get()->replaySystem();

            DragQueryFile(drop, i, fileName, 256);
            WIN32_FILE_ATTRIBUTE_DATA fileInfo;
            GetFileAttributesEx(fileName, GetFileExInfoStandard, &fileInfo);

            std::filesystem::path path(fileName);
            geode::log::info("Loading file via drag-and-drop: {}", path);
            if (path.extension() != ".slc") {
                break;
            }

            std::filesystem::path dest =
                Mod::get()->getPersistentDir() / "replays" / path.filename();

            rs.createBackup();
            rs.backupExisting(dest);
            rs.load(path);
            rs.save(dest);

            rs.m_replayName = path.stem().string();
        }

        DragFinish(drop);
    }

    return CallWindowProc(ImGuiHookCtx::get().m_oWndProc, hWnd, uMsg, wParam,
                          lParam);
}

const char* BLUR_VERT = R"(#version 130
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texCoord;

out vec2 v_texCoord;

void main() {
    gl_Position = vec4(a_position.xy, 0.0, 1.0);
    v_texCoord = a_texCoord;
}
)";

static const char* BLUR_FRAG = R"(#version 130
#extension GL_ARB_explicit_attrib_location : require
#extension GL_ARB_explicit_uniform_location : require

in vec2 v_texCoord;
out vec4 fragColor;

layout(location = 0) uniform sampler2D u_texture;
layout(location = 1) uniform vec2 u_texelSize;
layout(location = 2) uniform vec2 u_direction;
layout(location = 3) uniform vec4 u_window;

vec4 sampleBlur(vec2 texCoord) {
    vec4 color = vec4(0.0);
    vec2 off1 = vec2(1.3846153846) * u_direction * u_texelSize;
    vec2 off2 = vec2(4.2307692308) * u_direction * u_texelSize;
    color += texture2D(u_texture, texCoord) * 0.2270270270;
    color += texture2D(u_texture, texCoord + off1) * 0.3162162162;
    color += texture2D(u_texture, texCoord - off1) * 0.3162162162;
    color += texture2D(u_texture, texCoord + off2) * 0.0702702703;
    color += texture2D(u_texture, texCoord - off2) * 0.0702702703;

    return color;
}

void main() {
    fragColor = sampleBlur(v_texCoord);
}
)";

static const char* GLASS_FRAG = R"(#version 130
#extension GL_ARB_explicit_attrib_location : require
#extension GL_ARB_explicit_uniform_location : require

in vec2 v_texCoord;
out vec4 fragColor;

layout(location = 0) uniform sampler2D u_texture;
layout(location = 1) uniform vec2 u_texelSize;
layout(location = 3) uniform vec4 u_window;
layout(location = 4) uniform float u_cornerRadius;
layout(location = 5) uniform sampler2D u_scene;

float roundedRectDistance(vec2 texCoord) {
    vec2 framebufferSize = 1.0 / u_texelSize;
    vec2 center = (u_window.xy + u_window.zw) * 0.5;
    vec2 halfSize = (u_window.zw - u_window.xy) * framebufferSize * 0.5;
    vec2 point = (texCoord - center) * framebufferSize;
    float radius = min(u_cornerRadius, min(halfSize.x, halfSize.y));
    vec2 q = abs(point) - halfSize + vec2(radius);
    return length(max(q, vec2(0.0))) +
           min(max(q.x, q.y), 0.0) - radius;
}

vec2 roundedRectNormal(vec2 texCoord) {
    float left = roundedRectDistance(texCoord - vec2(u_texelSize.x, 0.0));
    float right = roundedRectDistance(texCoord + vec2(u_texelSize.x, 0.0));
    float down = roundedRectDistance(texCoord - vec2(0.0, u_texelSize.y));
    float up = roundedRectDistance(texCoord + vec2(0.0, u_texelSize.y));
    vec2 gradient = vec2(right - left, up - down);
    float magnitude = length(gradient);
    return magnitude > 0.0001 ? gradient / magnitude : vec2(0.0, 1.0);
}

void main() {
    vec4 base = texture2D(u_texture, v_texCoord);
    float distanceToEdge = roundedRectDistance(v_texCoord);
    vec2 windowSizePixels = (u_window.zw - u_window.xy) / u_texelSize;
    float sizeScale = clamp(120.0 / min(windowSizePixels.x,
                                        windowSizePixels.y), 0.62, 1.3);
    float edgeWidth = clamp(u_cornerRadius * 1.5, 6.0, 13.0) * sizeScale;
    float edge = smoothstep(-edgeWidth, -0.2, distanceToEdge);
    edge = edge * edge * (3.0 - 2.0 * edge);
    float lens = pow(edge, 0.72);

    vec2 normal = roundedRectNormal(v_texCoord);
    float displacement = mix(2.0, 18.0 * sizeScale, lens * lens);
    vec2 refractedCoord = clamp(
        v_texCoord - normal * u_texelSize * displacement,
        u_texelSize * 3.0, vec2(1.0) - u_texelSize * 3.0
    );
    vec2 chromaOffset = normal * u_texelSize * (0.6 * lens);
    vec3 refracted = vec3(
        texture2D(u_scene, refractedCoord + chromaOffset).r,
        texture2D(u_scene, refractedCoord).g,
        texture2D(u_scene, refractedCoord - chromaOffset).b
    );

    float luminance = dot(refracted, vec3(0.2126, 0.7152, 0.0722));
    refracted = vec3(luminance) +
                (refracted - vec3(luminance)) * 1.28;

    vec2 reflectedCoord = clamp(
        v_texCoord + normal * u_texelSize *
            ((4.0 + 12.0 * lens) * sizeScale),
        u_texelSize * 3.0, vec2(1.0) - u_texelSize * 3.0
    );
    vec3 reflected = texture2D(u_texture, reflectedCoord).rgb;
    float reflectedLuminance = dot(reflected, vec3(0.2126, 0.7152, 0.0722));
    reflected = mix(vec3(reflectedLuminance), reflected, 1.35) * 1.12;

    float baseLuminance = dot(base.rgb, vec3(0.2126, 0.7152, 0.0722));
    vec3 graded = vec3(baseLuminance) +
                  (base.rgb - vec3(baseLuminance)) * 1.18;
    graded = graded * vec3(0.96, 0.99, 1.06) + vec3(0.006, 0.010, 0.020);

    vec3 color = mix(base.rgb, graded, 0.22);
    color = mix(color, refracted, 0.88 * lens);
    color = mix(color, reflected, 0.24 * lens * sizeScale);

    fragColor = vec4(color, base.a);
}
)";

static GLuint createProgram(const char* vertexSource,
                            const char* fragmentSource) {
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char infoLog[512];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        geode::log::error("failed to link glass shader: {}", infoLog);
    }
    return program;
}

static GLuint createGlassTexture(GLsizei width, GLsizei height) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    return texture;
}

static void resizeGlassTexture(GLuint texture, GLsizei width, GLsizei height) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
}

void ImGuiHookCtx::init(cocos2d::CCEGLView* view) {
    auto* glfwWindow = view->getWindow();
    if (glfwWindow == nullptr) return;
    HDC hdc =
        *reinterpret_cast<HDC*>(reinterpret_cast<uintptr_t>(glfwWindow) + 632);
    if (hdc == nullptr) return;
    HWND hWnd = WindowFromDC(hdc);
    if (hWnd == nullptr) return;

    if (m_inited) {
        if (glfwWindow == m_glfwWindow && hWnd == m_hWnd) {
            return;
        }

        geode::log::info("gl window recreated; reinitializing overlay");
        reinitForNewWindow(view, hdc, hWnd);
        return;
    }

    m_time = 0.0f;

    auto mouse = MouseInputEvent().listen([](MouseInputData&) {
        if (ImGui::GetIO().WantCaptureMouse) {
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
                return ListenerResult::Stop;
            }
        }

        return ListenerResult::Propagate;
    });
    mouse.leak();

    auto kb = KeyboardInputEvent().listen([](KeyboardInputData&) {
        if (ImGui::GetIO().WantCaptureKeyboard) {
            return ListenerResult::Stop;
        }

        return ListenerResult::Propagate;
    });
    kb.leak();

    m_hWnd = hWnd;

    DragAcceptFiles(m_hWnd, TRUE);

    m_oWndProc =
        (WNDPROC)SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)h_WndProc);
    m_ctx.init(tabby::Context::CtxInitParams{.hdc = (void*)hdc});

    float width = view->getFrameSize().width / BLUR_DOWNSCALING_FACTOR;
    float height = view->getFrameSize().height / BLUR_DOWNSCALING_FACTOR;

    m_width = width * BLUR_DOWNSCALING_FACTOR;
    m_height = height * BLUR_DOWNSCALING_FACTOR;

    Bot::get()->ui().setup();

    m_blurPass = RenderPass{.m_width = (unsigned int)width,
                            .m_height = (unsigned int)height,
                            .m_vertexShader = BLUR_VERT,
                            .m_fragmentShader = BLUR_FRAG,
                            .m_readPixels = [](float, float) {}};

    // m_postprocessPass = RenderPass{
    //     .m_width = (unsigned int)width,
    //     .m_height = (unsigned int)height,
    //     .m_vertexShader = BLUR_VERT,
    //     .m_fragmentShader = POSTPROCESS_FRAG,
    //     .m_readPixels = []() {}};

    GLint oldFbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);

    m_blurPass.initialize();
    m_glassProgram = createProgram(BLUR_VERT, GLASS_FRAG);
    // Bot::get()->ui().m_theme->initialize();

    GLint oldTex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex);
    m_inputTex = createGlassTexture(m_blurPass.m_width, m_blurPass.m_height);
    m_sceneTex = createGlassTexture(m_blurPass.m_width, m_blurPass.m_height);
    m_postprocessInputTex =
        createGlassTexture((GLsizei)m_width, (GLsizei)m_height);
    glBindTexture(GL_TEXTURE_2D, oldTex);

    glBindFramebuffer(GL_FRAMEBUFFER, m_blurPass.m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           m_inputTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                           m_sceneTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);

    geode::log::info("Initialized ImGUI");
    m_glfwWindow = glfwWindow;
    m_inited = true;
}

void ImGuiHookCtx::reinitForNewWindow(cocos2d::CCEGLView* view, HDC hdc,
                                      HWND hWnd) {
    m_time = 0.0f;
    m_glfwWindow = view->getWindow();
    m_hWnd = hWnd;

    DragAcceptFiles(m_hWnd, TRUE);
    m_oWndProc =
        (WNDPROC)SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)h_WndProc);

    m_ctx.reinit(tabby::Context::CtxInitParams{.hdc = (void*)hdc});

    float width = view->getFrameSize().width / BLUR_DOWNSCALING_FACTOR;
    float height = view->getFrameSize().height / BLUR_DOWNSCALING_FACTOR;

    m_width = width * BLUR_DOWNSCALING_FACTOR;
    m_height = height * BLUR_DOWNSCALING_FACTOR;

    m_blurPass.m_width = (unsigned int)width;
    m_blurPass.m_height = (unsigned int)height;

    GLint oldFbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);

    m_blurPass.initialize();
    if (m_glassProgram != 0) glDeleteProgram(m_glassProgram);
    m_glassProgram = createProgram(BLUR_VERT, GLASS_FRAG);

    GLint oldTex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex);
    m_inputTex = createGlassTexture(m_blurPass.m_width, m_blurPass.m_height);
    m_sceneTex = createGlassTexture(m_blurPass.m_width, m_blurPass.m_height);
    m_postprocessInputTex =
        createGlassTexture((GLsizei)m_width, (GLsizei)m_height);
    glBindTexture(GL_TEXTURE_2D, oldTex);

    glBindFramebuffer(GL_FRAMEBUFFER, m_blurPass.m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           m_inputTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                           m_sceneTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);

    // reinit textures since those get fucked
    Bot::get()->ui().reinitThemeGraphics();

    geode::log::info("Reinitialized overlay for recreated window");
}

void ImGuiHookCtx::handleResize(float width, float height) {
    if (!m_inited) return;

    if (width <= 0.0f || height <= 0.0f) return;

    const float fullWidth = width;
    const float fullHeight = height;

    width /= BLUR_DOWNSCALING_FACTOR;
    height /= BLUR_DOWNSCALING_FACTOR;

    m_width = width * BLUR_DOWNSCALING_FACTOR;
    m_height = height * BLUR_DOWNSCALING_FACTOR;

    m_blurPass.m_width = (unsigned int)width;
    m_blurPass.m_height = (unsigned int)height;
    m_blurPass.resize();

    Bot::get()->ui().m_theme->resize((uint32_t)fullWidth,
                                     (uint32_t)fullHeight);

    GLint oldTex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex);
    resizeGlassTexture(m_inputTex, m_blurPass.m_width, m_blurPass.m_height);
    resizeGlassTexture(m_sceneTex, m_blurPass.m_width, m_blurPass.m_height);
    resizeGlassTexture(m_postprocessInputTex, (GLsizei)fullWidth,
                       (GLsizei)fullHeight);
    glBindTexture(GL_TEXTURE_2D, oldTex);
}

void ImGuiHookCtx::preSampleBlur(ImVec4 window, float cornerRadius) {
    if (!m_inited) return;

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_oldFbo);
    glGetIntegerv(GL_CURRENT_PROGRAM, &m_oldProgram);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &m_oldActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_oldTex);
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_oldTex1);
    glActiveTexture(GL_TEXTURE0);

    glViewport(0, 0, m_blurPass.m_width, m_blurPass.m_height);
    glDisable(GL_SCISSOR_TEST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_blurPass.m_fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_blurPass.m_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           m_inputTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                           m_sceneTex, 0);

    glReadBuffer(GL_BACK);

    if (auto renderer = Renderer::get(); renderer->isRecording()) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER,
                          renderer->m_texture.m_old_fbo);  // kind and jorkful
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_blurPass.m_width,
                      m_blurPass.m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 20, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 20, (void*)8);

    m_windowPos = window;
    m_cornerRadius = cornerRadius;
}

void ImGuiHookCtx::preSamplePostprocess(ImVec4 window, float cornerRadius) {
    if (!m_inited) return;

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_oldFbo);
    glGetIntegerv(GL_CURRENT_PROGRAM, &m_oldProgram);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &m_oldActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_oldTex);
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_oldTex1);
    glActiveTexture(GL_TEXTURE0);

    auto& postprocessPass = Bot::get()->ui().m_theme->m_postprocessPass;
    glViewport(0, 0, (GLsizei)m_width, (GLsizei)m_height);
    glDisable(GL_SCISSOR_TEST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postprocessPass.m_fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, postprocessPass.m_tex, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_2D, m_postprocessInputTex, 0);

    glReadBuffer(GL_BACK);
    if (auto renderer = Renderer::get(); renderer->isRecording()) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->m_texture.m_old_fbo);
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glBlitFramebuffer(0, 0, (GLsizei)m_width, (GLsizei)m_height, 0, 0,
                      (GLsizei)m_width, (GLsizei)m_height, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 20, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 20, (void*)8);

    m_windowPos = window;
    m_cornerRadius = cornerRadius;
}

void ImGuiHookCtx::sampleBlurFirstPass() {
    if (!m_inited) return;

    glUseProgram(m_blurPass.m_program);

    glUniform1i(0, 0);
    glUniform2f(1, 1.0f / m_blurPass.m_width, 1.0f / m_blurPass.m_height);
    float blurExp = 1.6f;
    glUniform2f(2, blurExp, 0.0f);
    glUniform4f(3, m_windowPos.x, m_windowPos.y, m_windowPos.z, m_windowPos.w);

    glDrawBuffer(GL_COLOR_ATTACHMENT1);
}

void ImGuiHookCtx::sampleBlurSecondPass() {
    if (!m_inited) return;

    glUseProgram(m_blurPass.m_program);

    glUniform1i(0, 0);
    glUniform2f(1, 1.0f / m_blurPass.m_width, 1.0f / m_blurPass.m_height);
    float blurExp = 1.6f;
    glUniform2f(2, 0.0f, blurExp);
    glUniform4f(3, m_windowPos.x, m_windowPos.y, m_windowPos.z, m_windowPos.w);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void ImGuiHookCtx::sampleBlurSnapshot() {
    if (!m_inited) return;

    glUseProgram(m_blurPass.m_program);
    glUniform1i(0, 0);
    glUniform2f(1, 1.0f / m_blurPass.m_width, 1.0f / m_blurPass.m_height);
    glUniform2f(2, 0.0f, 0.0f);
    glUniform4f(3, m_windowPos.x, m_windowPos.y, m_windowPos.z, m_windowPos.w);
    glDrawBuffer(GL_COLOR_ATTACHMENT2);
}

void ImGuiHookCtx::sampleBlurPostprocessToInput() {
    if (!m_inited) return;

    glUseProgram(Bot::get()->ui().m_theme->m_postprocessPass.m_program);

    glUniform1i(0, 0);
    glUniform2f(1, 1.0f / m_blurPass.m_width, 1.0f / m_blurPass.m_height);
    float blurExp = 1.6f;
    glUniform2f(2, blurExp, 0.0f);
    glUniform4f(3, m_windowPos.x, m_windowPos.y, m_windowPos.z, m_windowPos.w);
    glUniform1f(4, m_time);

    glDrawBuffer(GL_COLOR_ATTACHMENT1);
}

void ImGuiHookCtx::samplePostprocessFullResolution() {
    if (!m_inited) return;

    glUseProgram(Bot::get()->ui().m_theme->m_postprocessPass.m_program);

    glUniform1i(0, 0);
    glUniform2f(1, 1.0f / m_width, 1.0f / m_height);
    glUniform2f(2, 1.6f, 0.0f);
    glUniform4f(3, m_windowPos.x, m_windowPos.y, m_windowPos.z, m_windowPos.w);
    glUniform1f(4, m_time);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

static void configureGlassPass(ImGuiHookCtx& context, GLenum output) {
    glUseProgram(context.m_glassProgram);
    glUniform1i(0, 0);
    glUniform2f(1, 1.0f / context.m_blurPass.m_width,
                1.0f / context.m_blurPass.m_height);
    glUniform4f(3, context.m_windowPos.x, context.m_windowPos.y,
                context.m_windowPos.z, context.m_windowPos.w);
    glUniform1f(4,
                context.m_cornerRadius / ImGuiHookCtx::BLUR_DOWNSCALING_FACTOR);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, context.m_sceneTex);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(5, 1);
    glDrawBuffer(output);
}

void ImGuiHookCtx::sampleGlassToInput() {
    configureGlassPass(*this, GL_COLOR_ATTACHMENT1);
}

void ImGuiHookCtx::sampleGlassToBlur() {
    configureGlassPass(*this, GL_COLOR_ATTACHMENT0);
}

void ImGuiHookCtx::postSampleBlur() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_oldFbo);
    glUseProgram(m_oldProgram);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_oldTex1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_oldTex);
    glActiveTexture(m_oldActiveTexture);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                        GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_PRIMITIVE_RESTART);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glViewport(0, 0, m_width, m_height);
}

void ImGuiHookCtx::draw() {
    init(CCEGLView::get());

    if (!m_inited) return;

    m_ctx.draw([&]() { Bot::get()->ui().draw(); });
}

struct SLEGLView : Modify<SLEGLView, CCEGLView> {
    void swapBuffers() {
        auto& ctx = ImGuiHookCtx::get();

        ctx.draw();

        CCEGLView::swapBuffers();

        // glClearColor(1.0, 1.0, 1.0, 1.0);
    }
};
