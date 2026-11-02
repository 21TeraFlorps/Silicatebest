#include "manager.hpp"

#include <imgui_internal.h>
#include <winuser.h>

#include <Geode/Enums.hpp>
#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/utils/hash.hpp>
#include <algorithm>
#include <asp/fs/fs.hpp>
#include <cctype>
#include <deque>
#include <slc/formats/v3/atom.hpp>
#include <slc/formats/v3/replay.hpp>
#include <tabby.hpp>
#include <tuple>
#include <unordered_set>
#include <variant>
#include <widgets/button.hpp>
#include <widgets/drag.hpp>

#include "Geode/utils/general.hpp"
#include "Geode/utils/string.hpp"
#include "assist/autoclicker.hpp"
#include "assist/hitboxes.hpp"
#include "assist/hitzones.hpp"
#include "bot/bot.hpp"
#include "bot/updater.hpp"
#include "cbf/cbf.hpp"
#include "cbf/sidecar.hpp"
#include "checkpoint/fix.hpp"
#include "hook.hpp"
#include "label/label.hpp"
#include "render/dsp.hpp"
#include "render/renderer.hpp"
#include "replay/system.hpp"
#include "settings/settings.hpp"
#include "shared/keys.hpp"
#include "shared/value/value.hpp"
#include "trajectory/trajectory.hpp"
// #include "widgets/hotkey.hpp"
#include "hashes.hpp"
#include "imgui.h"
#include "util/config.hpp"
#include "widgets/checkbox.hpp"
#include "widgets/image.hpp"
#include "widgets/info_modal.hpp"
#include "widgets/input.hpp"
#include "widgets/tooltip.hpp"
#include "widgets/yes_no_modal.hpp"

#ifdef SILICATE_PROTECT
#include "VMProtect/VMProtectSDK.h"
#endif

using namespace geode::prelude;

UIManager::UIManager() : m_font(nullptr), m_medium(nullptr), m_bold(nullptr) {}

void UIManager::toggle() { m_state.toggle(); }

static std::vector<Theme> s_themes = {
    Theme("Silica", "title_new.png",
          R"(#version 130
        #extension GL_ARB_explicit_attrib_location : require
        #extension GL_ARB_explicit_uniform_location : require

        in vec2 v_texCoord;
        out vec4 fragColor;

        layout(location = 0) uniform sampler2D u_texture;
        layout(location = 1) uniform vec2 u_texelSize;
        layout(location = 2) uniform vec2 u_direction;
        layout(location = 3) uniform vec4 u_window;
        layout(location = 4) uniform float u_time;

        void main() {
            fragColor = texture2D(u_texture, v_texCoord);
        }
        )",
          1.0),
    Theme("Polychrome", "title_gay_new.png",
          R"(#version 130
        #extension GL_ARB_explicit_attrib_location : require
        #extension GL_ARB_explicit_uniform_location : require

        in vec2 v_texCoord;
        out vec4 fragColor;

        layout(location = 0) uniform sampler2D u_texture;
        layout(location = 1) uniform vec2 u_texelSize;
        layout(location = 2) uniform vec2 u_direction;
        layout(location = 3) uniform vec4 u_window;
        layout(location = 4) uniform float u_time;

        void main() {
            fragColor = vec4(
                texture2D(u_texture, v_texCoord + vec2(0.01, 0.0)).x,
                texture2D(u_texture, v_texCoord + vec2(0.0, -0.01)).y,
                texture2D(u_texture, v_texCoord + vec2(-0.01, 0.0)).z,
                1.0
            );
            fragColor = clamp(fragColor, 0.15, 1.0);

            vec2 uv = v_texCoord;

            vec4 col =  vec4(
                abs(sin(uv.y + 2.0)) * abs(sin(uv.x * 2.0 + u_time)) * 1.0,
                abs(sin(uv.y + 2.0)) * abs(sin(uv.x * 2.0 + u_time + 0.4)) * 1.0,
                abs(sin(uv.y + 2.0)) * abs(sin(uv.x * 2.0 + u_time + 0.8)) * 1.0,
                1.0
            );

            vec4 col2 =  vec4(
                abs(sin(uv.y)) * abs(sin(uv.x * 2.0 + u_time + 1.0)) * 1.0,
                abs(sin(uv.y)) * abs(sin(uv.x * 2.0 + u_time + 1.5)) * 1.0,
                abs(sin(uv.y)) * abs(sin(uv.x * 2.0 + u_time + 2.0)) * 1.0,
                1.0
            );

            vec4 rainbowCol = col + col2;

            fragColor *= rainbowCol;
        }
        )",
          3.0),
    Theme("Estradiol", "title_trans_new.png",
          R"(#version 130
        #extension GL_ARB_explicit_attrib_location : require
        #extension GL_ARB_explicit_uniform_location : require

        in vec2 v_texCoord;
        out vec4 fragColor;

        layout(location = 0) uniform sampler2D u_texture;
        layout(location = 1) uniform vec2 u_texelSize;
        layout(location = 2) uniform vec2 u_direction;
        layout(location = 3) uniform vec4 u_window;
        layout(location = 4) uniform float u_time;

        void main() {

            fragColor = texture2D(u_texture, v_texCoord);

            vec2 uv = v_texCoord;

            vec4 col =  vec4(
                abs(sin(uv.y + 2.0)) * abs(sin(uv.x * 2.0 + u_time)) * 2.0,
                abs(sin(uv.y + 2.0)) * abs(sin(uv.x * 2.0 + u_time + 0.4)) * 0.0,
                abs(sin(uv.y + 2.0)) * abs(sin(uv.x * 2.0 + u_time)) * 2.0,
                1.0
            );

            vec4 col2 =  vec4(
                abs(sin(uv.y)) * abs(sin(uv.x * 2.0 + u_time + 1.0)) * 0.0,
                abs(sin(uv.y)) * abs(sin(uv.x * 2.0 + u_time + 2.0)) * 2.0,
                abs(sin(uv.y)) * abs(sin(uv.x * 2.0 + u_time + 2.0)) * 2.0,
                1.0
            );

            vec4 rainbowCol = col + col2;

            fragColor = mix(fragColor, rainbowCol, 0.4);
        }
        )",
          1.55),
    Theme("Endothermic", "title_jolly_new.png",
          R"(#version 130
        #extension GL_ARB_explicit_attrib_location : require
        #extension GL_ARB_explicit_uniform_location : require

        #define _SnowflakeAmount 250
        #define _BlizardFactor 0.15

        in vec2 v_texCoord;
        out vec4 fragColor;

        layout(location = 0) uniform sampler2D u_texture;
        layout(location = 1) uniform vec2 u_texelSize;
        layout(location = 2) uniform vec2 u_direction;
        layout(location = 3) uniform vec4 u_window;
        layout(location = 4) uniform float u_time;

        float rnd(float x) {
            return fract(sin(dot(vec2(x+47.49,38.2467/(x+2.3)), vec2(12.9898, 78.233)))* (43758.5453));
        }

        float drawCircle(vec2 center, float radius) {
            vec2 uv = v_texCoord * vec2(1.0, u_texelSize.x / u_texelSize.y);

            return 1.0 - smoothstep(0.0, radius, length(uv - center));
        }

        void main() {
            fragColor = texture2D(u_texture, v_texCoord);

            vec2 uv = v_texCoord;
            fragColor += vec4(0.2, 0.2, 0.6, 1.0);

            for (int i=0; i < _SnowflakeAmount; i++) {

                float j = float(i);
                float speed = 0.3+rnd(cos(j))*(0.7+0.5*cos(j/(float(_SnowflakeAmount)*0.25)));

                vec2 center = vec2((0.25-uv.y)*_BlizardFactor+rnd(j)+0.1*cos(u_time+sin(j)), mod(sin(j)-speed*(u_time*1.5*(0.1+_BlizardFactor)), 0.65));

                float circle = drawCircle(center, 0.001 + speed * 0.012) * ((0.001 + speed * 0.012) * 50.0);

                fragColor += vec4(circle * 0.4, circle * 0.4, circle * 0.5, circle * 0.35);

            }
        }
        )",
          1.0),
    Theme("Cyanide", "title_new.png",
          R"(#version 130
        #extension GL_ARB_explicit_attrib_location : require
        #extension GL_ARB_explicit_uniform_location : require

        in vec2 v_texCoord;
        out vec4 fragColor;

        layout(location = 0) uniform sampler2D u_texture;
        layout(location = 1) uniform vec2 u_texelSize;
        layout(location = 2) uniform vec2 u_direction;
        layout(location = 3) uniform vec4 u_window;
        layout(location = 4) uniform float u_time;

        // Adapted from https://www.shadertoy.com/view/4st3DM
        void main() {
            fragColor = texture2D(u_texture, v_texCoord);
            float gray = 0.299 * fragColor.x + 0.587 * fragColor.y + 0.114 * fragColor.z;
            fragColor = mix(vec4(gray, gray, gray, 1.0), fragColor, 0.3);
            fragColor -= vec4(0.3);

            vec2 uv = v_texCoord;

            float n = u_time;
            float x = uv.x * (sin(uv.y + u_time * 0.5)* 2.0);
            float y = uv.y * (sin(uv.x + u_time * 0.2)* 2.0);

            float xp = uv.x-0.5+sin(x*3.+n-sin(y*7.+n));
            float yp = uv.y-0.5+sin(y*3.+n+sin(x*5.-n));

            float eh = ((sqrt(xp*xp+yp*yp)*5.+n));

            vec3 one = vec3(0.2, 0.7, 0.5) * 0.8;
            vec3 two = vec3(0.9, 0.2, 0.5) * 0.8;

            vec4 phase = vec4(mix(one, two, (sin(u_time) + 1.0) / 2.0), 1.0);

            fragColor += phase;
            fragColor += vec4(
                sin(eh*0.6+(y+n)*5.-n*5.),
                sin(eh*0.6+(y+n)*5.-n*5.),
                sin(eh*0.6+(y+n)*5.-n*5.),
            1.0) * vec4(one, 1.0) * vec4(0.4);

            fragColor += vec4(
                sin(eh*0.5+(y+n*1.1)*5.-n*5.),
                sin(eh*0.5+(y+n*1.1)*5.-n*5.),
                sin(eh*0.5+(y+n*1.1)*5.-n*5.),
            1.0) * vec4(two, 1.0) * vec4(0.4);

            float light = 0.299 * fragColor.x + 0.587 * fragColor.y + 0.114 * fragColor.z;
            vec4 mixed = (fragColor * vec4(mix(two, one, (sin(u_time) + 1.0) / 2.0), 1.0));
            fragColor = mix(fragColor, mixed, clamp(light - 0.6, 0.0, 1.0));

            // fragColor -= vec4(
            //     sin(eh*0.7+(y+n*1.2)*5.-n*5.),
            //     sin(eh*0.7+(y+n*1.2)*5.-n*5.),
            //     sin(eh*0.7+(y+n*1.2)*5.-n*5.),
            // 1.0) * phase * vec4(0.2);
        }
        )",
          2.0),
    Theme("Phosphor", "title_new_crt.png",
          R"(#version 130
        #extension GL_ARB_explicit_attrib_location : require
        #extension GL_ARB_explicit_uniform_location : require

        in vec2 v_texCoord;
        out vec4 fragColor;

        layout(location = 0) uniform sampler2D u_texture;
        layout(location = 1) uniform vec2 u_texelSize;
        layout(location = 2) uniform vec2 u_direction;
        layout(location = 3) uniform vec4 u_window;
        layout(location = 4) uniform float u_time;

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

        float random(vec2 uv) {
            return fract(sin(dot(uv.xy, vec2(24.91233, 1123.233))) * 60132.468112);
        }

        void main() {
            float scanline = 0.02;
            float scanline2 = 0.03;

            fragColor = texture2D(u_texture, v_texCoord);
            vec2 uvScanlines = vec2(v_texCoord.x, v_texCoord.y + u_time * 0.2);
            vec2 uvScanlines2 = vec2(v_texCoord.x, v_texCoord.y + u_time * 0.05);

            fragColor += sampleBlur(v_texCoord) * 0.45;
            fragColor *= vec4(0.9, 0.5, 0.2, 1.0);

            fragColor += sin(uvScanlines.y * 800.0) * scanline2 - scanline2;
            fragColor += sin(uvScanlines2.y * 30.0) * scanline - scanline;

            fragColor += vec4(0.07 * random(uvScanlines));

            // fragColor *= min(1.0, sin(v_texCoord.y * 25) + 0.2);
        }
        )",
          2.0, true),
};

static uint64_t fnv1aHash(const std::vector<uint8_t>& data) {
    uint64_t hash = 14695981039346656037ull;

    for (const uint8_t byte : data) {
        hash ^= byte;
        hash *= 1099511628211;
    }

    return hash;
}

using DpiGetterType = decltype(GetDpiForWindow)*;
static DpiGetterType g_dpiGetter = nullptr;

static float getWindowDpi() {
    if (!g_dpiGetter) return 1.0;

    uint32_t dpi = g_dpiGetter(ImGuiHookCtx::get().m_hWnd);
    float scaling = (float)dpi / 96.0;

    return scaling;
}

// EPIC code
static std::string ffmpegUrl = "https://cdn.silicate.dev/ffmpeg.zip";

void UIManager::setup() {
    m_state.m_animationSpeed->handle([](float& speed) {
        if (speed < 0.1f || speed > 3.0f) {
            speed = 1.0f;
        }

        tabby::TabbyGlobalCfg::get().animationSpeed = speed;
    });

    m_state.m_playAnimations->handle(
        [](bool& play) { tabby::TabbyGlobalCfg::get().playAnimations = play; });

    g_dpiGetter = (DpiGetterType)GetProcAddress(GetModuleHandleA("user32.dll"),
                                                "GetDpiForWindow");

    m_state.m_uiScale->handle([this](float& scale) {
        tabby::TabbyGlobalCfg::get().uiScale = scale * getWindowDpi();
        m_state.m_restartGameInfo = true;
    });

    m_state.m_visible->handle([](bool&) {
        if (slui::isInfoModalOpen()) {
            slui::hideInfoModal();
        }
    });

    tabby::TabbyGlobalCfg::get().uiScale =
        m_state.m_uiScale->inner() * geode::utils::getDisplayFactor();

    static ImVector<ImWchar> glyphRanges;

    ImFontGlyphRangesBuilder builder;
    builder.AddChar(0xf192);
    builder.AddChar(0xefba);
    builder.AddChar(0xf03d);
    builder.AddChar(0xf121);
    builder.AddChar(0xf013);
    builder.AddChar(0xf078);
    builder.AddChar(0xf044);
    builder.AddChar(0xf054);
    builder.AddChar(0xf00c);
    builder.AddChar(0xf51b);
    builder.AddChar(0xf004);
    builder.AddChar(0xf0fe);
    builder.BuildRanges(&glyphRanges);

    ImFontConfig mediumFontCfg;
    mediumFontCfg.OversampleH = 3;
    mediumFontCfg.OversampleV = 3;
    // mediumFontCfg.FontDataOwnedByAtlas = true;
    mediumFontCfg.GlyphExtraAdvanceX = -1.0f * 20.0f * 0.02f;

    ImFont* mediumFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(
        geode::utils::string::pathToString(Mod::get()->getResourcesDir() /
                                           "font_medium.ttf")
            .c_str(),
        18.0f, &mediumFontCfg);
    mediumFontCfg.MergeMode = true;
    mediumFontCfg.GlyphOffset = ImVec2(0.0f, -1.0f);
    ImGui::GetIO().Fonts->AddFontFromFileTTF(
        geode::utils::string::pathToString(Mod::get()->getResourcesDir() /
                                           "font_symbols.ttf")
            .c_str(),
        18.0f, &mediumFontCfg, glyphRanges.Data);

    ImGui::GetIO().Fonts->Build();

    ImFontConfig mainFontCfg;
    mainFontCfg.OversampleH = 3;
    mainFontCfg.OversampleV = 3;
    // mainFontCfg.FontDataOwnedByAtlas = true;
    mainFontCfg.GlyphExtraAdvanceX = -1.0f * 17.0f * 0.03f;

    ImFont* mainFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(
        geode::utils::string::pathToString(Mod::get()->getResourcesDir() /
                                           "font_main.ttf")
            .c_str(),
        17.0f, &mainFontCfg);
    mainFontCfg.MergeMode = true;
    mainFontCfg.GlyphOffset = ImVec2(0.0f, -2.0f);
    ImGui::GetIO().Fonts->AddFontFromFileTTF(
        geode::utils::string::pathToString(Mod::get()->getResourcesDir() /
                                           "font_symbols.ttf")
            .c_str(),
        14.0f, &mainFontCfg, glyphRanges.Data);

    ImGui::GetIO().Fonts->Build();

    m_font = tabby::Font(mainFont);
    m_medium = tabby::Font(mediumFont);

    m_bold =
        tabby::Font::load(geode::utils::string::pathToString(
                              Mod::get()->getResourcesDir() / "font_bold.ttf"),
                          32.0f);

    m_state.m_rainbow->notifyChange();
    m_state.m_playAnimations->notifyChange();
    m_state.m_animationSpeed->notifyChange();

    for (const auto& label : Bot::get()->labels().m_labels) {
        m_state.m_labelState.options.push_back(label.getFriendlyName().c_str());
    }

    m_state.m_replayNames.clear();
    if (auto iter =
            asp::fs::iterdir(Mod::get()->getPersistentDir() / "replays");
        iter.isOk()) {
        for (const auto& entry : iter.unwrap()) {
            if (entry.is_regular_file() && entry.path().extension() == ".slc") {
                m_state.m_replayNames.push_back(entry.path().stem().string());
            }
        }
    }

    m_state.m_presetNames.clear();
    if (auto iter =
            asp::fs::iterdir(Mod::get()->getPersistentDir() / "presets");
        iter.isOk()) {
        for (const auto& entry : iter.unwrap()) {
            if (entry.is_regular_file() &&
                entry.path().extension() == ".json") {
                m_state.m_presetNames.push_back(entry.path().stem().string());
            }
        }
    }

    m_state.m_scriptNames.clear();
    if (auto iter =
            asp::fs::iterdir(Mod::get()->getPersistentDir() / "scripts");
        iter.isOk()) {
        for (const auto& entry : iter.unwrap()) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                m_state.m_scriptNames.push_back(entry.path().stem().string());
            }
        }
    }

    m_replayAutocomplete.suggestions = m_state.m_replayNames;
    m_state.m_presetAutocomplete.suggestions = m_state.m_presetNames;
    m_state.m_scriptAutocomplete.suggestions = m_state.m_scriptNames;

    m_state.m_bgColorState.colors = SLSettings::get()->layoutBgColor;
    m_state.m_groundColorState.colors = SLSettings::get()->layoutGroundColor;

    for (auto& theme : s_themes) {
        theme.initialize();
    }

    SLSettings::get()->theme =
        std::clamp(SLSettings::get()->theme, 0, (int)s_themes.size() - 1);

    m_theme = &s_themes[SLSettings::get()->theme];
    m_state.m_themeState.selectedIndex = SLSettings::get()->theme;
    m_theme->apply();

    for (const auto& theme : s_themes) {
        m_state.m_themeState.options.push_back(theme.m_name);
    }

    Renderer::get()->loadFFmpeg();
}

void UIManager::reinitThemeGraphics() {
    for (auto& theme : s_themes) {
        theme.initialize();
    }
    if (m_theme) {
        m_theme->apply();
    }
}

struct BlurEffectParams {
    ImVec4 rect;
    float cornerRadius;
};

static std::deque<BlurEffectParams> s_blurEffectParams;

static void preDrawBlurEffect(const ImDrawList*, const ImDrawCmd* cmd) {
    auto pos = ImGui::GetDrawData()->DisplayPos;
    auto sz = ImGui::GetDrawData()->DisplaySize;
    const auto* params =
        static_cast<const BlurEffectParams*>(cmd->UserCallbackData);
    const ImVec4 rect = params != nullptr
                            ? params->rect
                            : ImVec4(cmd->ClipRect.x, cmd->ClipRect.y,
                                     cmd->ClipRect.z, cmd->ClipRect.w);

    ImGuiHookCtx::get().preSampleBlur(
        ImVec4((rect.x - pos.x) / sz.x, 1.0 - ((rect.w - pos.y) / sz.y),
               (rect.z - pos.x) / sz.x, 1.0 - ((rect.y - pos.y) / sz.y)),
        params != nullptr ? params->cornerRadius : 0.0f);
}

static void preDrawPostprocessEffect(const ImDrawList*, const ImDrawCmd* cmd) {
    auto pos = ImGui::GetDrawData()->DisplayPos;
    auto sz = ImGui::GetDrawData()->DisplaySize;
    const auto* params =
        static_cast<const BlurEffectParams*>(cmd->UserCallbackData);
    const ImVec4 rect = params != nullptr
                            ? params->rect
                            : ImVec4(cmd->ClipRect.x, cmd->ClipRect.y,
                                     cmd->ClipRect.z, cmd->ClipRect.w);

    ImGuiHookCtx::get().preSamplePostprocess(
        ImVec4((rect.x - pos.x) / sz.x, 1.0 - ((rect.w - pos.y) / sz.y),
               (rect.z - pos.x) / sz.x, 1.0 - ((rect.y - pos.y) / sz.y)),
        params != nullptr ? params->cornerRadius : 0.0f);
}

static void drawBlurEffectFirstPass(const ImDrawList*, const ImDrawCmd*) {
    ImGuiHookCtx::get().sampleBlurFirstPass();
}

static void drawBlurEffectSecondPass(const ImDrawList*, const ImDrawCmd*) {
    ImGuiHookCtx::get().sampleBlurSecondPass();
}

static void drawBlurSnapshot(const ImDrawList*, const ImDrawCmd*) {
    ImGuiHookCtx::get().sampleBlurSnapshot();
}

static void drawPostprocessToInput(const ImDrawList*, const ImDrawCmd*) {
    ImGuiHookCtx::get().sampleBlurPostprocessToInput();
}

static void drawPostprocessFullResolution(const ImDrawList*, const ImDrawCmd*) {
    ImGuiHookCtx::get().samplePostprocessFullResolution();
}

static void drawGlassToInput(const ImDrawList*, const ImDrawCmd*) {
    ImGuiHookCtx::get().sampleGlassToInput();
}

static void drawGlassToBlur(const ImDrawList*, const ImDrawCmd*) {
    ImGuiHookCtx::get().sampleGlassToBlur();
}

static void postDrawBlurEffect(const ImDrawList*, const ImDrawCmd*) {
    ImGuiHookCtx::get().postSampleBlur();
}

static void renderBlurBg(float rounding = 24.0f, float borderSize = 2.5f,
                         bool useShader = true, float bgOpacity = 0.15f,
                         bool pp = false, float opacityMultiplier = 1.0f,
                         bool ppOnly = false) {
    rounding *= tabby::TabbyGlobalCfg::get().uiScale;
    borderSize *= tabby::TabbyGlobalCfg::get().uiScale;

    bgOpacity = std::pow(bgOpacity, Bot::get()->ui().m_theme->m_opacityExp) *
                opacityMultiplier;

    if (!useShader) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetWindowPos(),
            ImGui::GetWindowPos() + ImGui::GetWindowSize(),
            ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, bgOpacity)), rounding,
            ImDrawFlags_RoundCornersAll);

        // window border
        ImGui::GetWindowDrawList()->AddRect(
            ImGui::GetWindowPos(),
            ImGui::GetWindowPos() + ImGui::GetWindowSize(),
            ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.2f)), rounding,
            ImDrawFlags_RoundCornersAll, borderSize);

        return;
    }

    ImGuiHookCtx::get().m_renderData.m_size =
        cocos2d::CCSize(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
    ImGuiHookCtx::get().m_renderData.m_pos =
        cocos2d::CCPoint(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);

    // An apply-after pass must execute after Tabby's child-window draw lists,
    // where most widgets live. The foreground draw list is rendered last.
    ImDrawList* drawList =
        ppOnly ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();

    cocos2d::CCSize frameSize = cocos2d::CCSize(ImGuiHookCtx::get().m_width,
                                                ImGuiHookCtx::get().m_height);

    const float edgeInset = borderSize * 0.5f;
    ImVec2 origin_pos = ImGui::GetWindowPos() + ImVec2(edgeInset, edgeInset);
    ImVec2 dest_pos = ImGui::GetWindowPos() + ImGui::GetWindowSize() -
                      ImVec2(edgeInset, edgeInset);
    const float innerRounding = std::max(0.0f, rounding - edgeInset);

    s_blurEffectParams.push_back(
        {.rect = ImVec4(origin_pos.x, origin_pos.y, dest_pos.x, dest_pos.y),
         .cornerRadius = innerRounding});
    drawList->AddCallback(
        ppOnly ? &preDrawPostprocessEffect : &preDrawBlurEffect,
        &s_blurEffectParams.back());

    float origin_uv_x = origin_pos.x / frameSize.width;
    float origin_uv_y = (frameSize.height - origin_pos.y) / frameSize.height;

    float dest_uv_x = dest_pos.x / frameSize.width;
    float dest_uv_y = (frameSize.height - dest_pos.y) / frameSize.height;

    const int SAMPLE_COUNT = 8;

    for (int i = 0; i < SAMPLE_COUNT && !ppOnly; i++) {
        drawList->AddCallback(&drawBlurEffectFirstPass, nullptr);
        drawList->AddImage(
            ImTextureRef((ImTextureID)ImGuiHookCtx::get().m_blurPass.m_tex),
            {-1.0, -1.0}, {1.0, 1.0});

        drawList->AddCallback(&drawBlurEffectSecondPass, nullptr);

        drawList->AddImage(
            ImTextureRef((ImTextureID)ImGuiHookCtx::get().m_inputTex),
            {-1.0, -1.0}, {1.0, 1.0});

        if (i == 1) {
            drawList->AddCallback(&drawBlurSnapshot, nullptr);
            drawList->AddImage(
                ImTextureRef((ImTextureID)ImGuiHookCtx::get().m_blurPass.m_tex),
                {-1.0, -1.0}, {1.0, 1.0});
        }
    }

    ImTextureID finalTexture;

    if (ppOnly) {
        drawList->AddCallback(&drawPostprocessFullResolution, nullptr);
        drawList->AddImage(
            ImTextureRef(
                (ImTextureID)ImGuiHookCtx::get().m_postprocessInputTex),
            {-1.0, -1.0}, {1.0, 1.0});

        finalTexture =
            (ImTextureID)Bot::get()->ui().m_theme->m_postprocessPass.m_tex;
    } else {
        if (pp) {
            drawList->AddCallback(&drawPostprocessToInput, nullptr);
            drawList->AddImage(
                ImTextureRef((ImTextureID)ImGuiHookCtx::get().m_blurPass.m_tex),
                {-1.0, -1.0}, {1.0, 1.0});

            drawList->AddCallback(&drawGlassToBlur, nullptr);
            drawList->AddImage(
                ImTextureRef((ImTextureID)ImGuiHookCtx::get().m_inputTex),
                {-1.0, -1.0}, {1.0, 1.0});

            finalTexture = (ImTextureID)ImGuiHookCtx::get().m_blurPass.m_tex;
        } else {
            drawList->AddCallback(&drawGlassToInput, nullptr);
            drawList->AddImage(
                ImTextureRef((ImTextureID)ImGuiHookCtx::get().m_blurPass.m_tex),
                {-1.0, -1.0}, {1.0, 1.0});
            finalTexture = (ImTextureID)ImGuiHookCtx::get().m_inputTex;
        }
    }

    drawList->AddCallback(&postDrawBlurEffect, nullptr);
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

    drawList->AddImageRounded(ImTextureRef(finalTexture), origin_pos, dest_pos,
                              {origin_uv_x, origin_uv_y},
                              {dest_uv_x, dest_uv_y},
                              ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)),
                              innerRounding, ImDrawFlags_RoundCornersAll);

    ImGui::GetWindowDrawList()->AddRectFilled(
        origin_pos, dest_pos,
        ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, bgOpacity)), innerRounding,
        ImDrawFlags_RoundCornersAll);

    // window border
    ImGui::GetWindowDrawList()->AddRect(
        origin_pos, dest_pos, ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.1f)),
        innerRounding, ImDrawFlags_RoundCornersAll, borderSize);
}

static std::vector<std::string> filterCandidates(
    const std::vector<std::string>& candidates, const std::string& filter) {
    std::vector<std::string> filtered;
    std::string lowerFilter = filter;
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(),
                   ::tolower);

    for (const auto& candidate : candidates) {
        std::string lowerCandidate = candidate;
        std::transform(lowerCandidate.begin(), lowerCandidate.end(),
                       lowerCandidate.begin(), ::tolower);
        if (lowerCandidate.find(lowerFilter) == 0) {
            filtered.push_back(candidate);
        }
    }

    return filtered;
}

static std::string keybindActionName(std::string_view tag) {
    if (tag == "ui.visible") return "Show / Hide UI";
    if (tag == "updater.advance_one") return "Step Forward";
    if (tag == "updater.advance_back") return "Step Backward";
    if (tag == "updater.frame_advance") return "Frame Advance";
    if (tag == "updater.max_upr") return "Maximum Updates per Render";
    if (tag == "updater.ssb_fix") return "Scroll Speed Bug Fix";
    if (tag == "replay.althook") return "Alternate Input Hook";

    std::string name(tag);
    bool capitalize = true;
    for (char& character : name) {
        if (character == '_' || character == '.') {
            character = ' ';
            capitalize = true;
        } else if (capitalize) {
            character = static_cast<char>(
                std::toupper(static_cast<unsigned char>(character)));
            capitalize = false;
        }
    }
    return name;
}

static bool keybindTextMatches(std::string_view text, std::string_view filter) {
    return std::ranges::search(text, filter, [](char left, char right) {
               return std::tolower(static_cast<unsigned char>(left)) ==
                      std::tolower(static_cast<unsigned char>(right));
           }).begin() != text.end();
}

static bool keybindMatches(const KeybindControl& keybind,
                           std::string_view filter) {
    if (filter.empty()) return true;
    return keybindTextMatches(keybind.getTag(), filter) ||
           keybindTextMatches(keybindActionName(keybind.getTag()), filter) ||
           keybindTextMatches(keybind.getKeyLabel(), filter);
}

static std::vector<std::string> filterKeybindActions(
    const std::vector<std::string>& actions, std::string_view filter) {
    std::vector<std::string> filtered;
    for (const auto& action : actions) {
        if (keybindTextMatches(action, filter) ||
            keybindTextMatches(keybindActionName(action), filter)) {
            filtered.push_back(action);
        }
    }
    return filtered;
}

static void applyNewStyle() {
    auto& style = ImGui::GetStyle();
    const float scale = tabby::TabbyGlobalCfg::get().uiScale;

    style.FrameRounding = 8.0f * scale;
    style.FrameBorderSize = 1.25f * scale;
    style.FramePadding = ImVec2(18.0f * scale, 7.0f * scale);
    style.PopupRounding = 14.0f * scale;
    style.ChildRounding = 20.0f * scale;
    style.GrabRounding = 8.0f * scale;
    style.ScrollbarRounding = 10.0f * scale;
    style.GrabMinSize = 12.0f * scale;
    style.DisabledAlpha = 0.56f;
    style.TouchExtraPadding = ImVec2(1.0f * scale, 1.0f * scale);
    style.ItemSpacing = ImVec2(8.0f * scale, 10.0f * scale);
    style.ItemInnerSpacing = ImVec2(6.0f * scale, 6.0f * scale);

    auto* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.94f, 0.95f, 0.97f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.94f, 0.95f, 0.97f, 0.42f);
    colors[ImGuiCol_Border] = ImVec4(0.88f, 0.91f, 0.96f, 0.16f);
    colors[ImGuiCol_Separator] = ImVec4(0.88f, 0.91f, 0.96f, 0.10f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.88f, 0.91f, 0.96f, 0.16f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.88f, 0.91f, 0.96f, 0.22f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.82f, 0.86f, 0.94f, 0.085f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.86f, 0.90f, 0.98f, 0.14f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.93f, 1.0f, 0.21f);

    colors[ImGuiCol_Button] = ImVec4(0.86f, 0.90f, 0.98f, 0.13f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.88f, 0.92f, 1.0f, 0.20f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.95f, 1.0f, 0.28f);

    colors[ImGuiCol_Header] = ImVec4(0.86f, 0.90f, 0.98f, 0.06f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.88f, 0.92f, 1.0f, 0.10f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.90f, 0.94f, 1.0f, 0.14f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.96f, 1.0f, 0.92f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.90f, 0.93f, 1.0f, 0.56f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.96f, 0.97f, 1.0f, 0.88f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.72f, 0.82f, 1.0f, 0.22f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.90f, 0.94f, 1.0f, 0.68f);

    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.88f, 0.91f, 0.96f, 0.24f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.90f, 0.93f, 1.0f, 0.36f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.95f, 1.0f, 0.48f);
}

struct ScopedInset {
    float m_inset;
    double m_originalWidgetWidth;

    explicit ScopedInset(double inset = 20.0)
        : m_inset(
              static_cast<float>(inset * tabby::TabbyGlobalCfg::get().uiScale)),
          m_originalWidgetWidth(tabby::TabbyGlobalCfg::get().widgetWidth) {
        ImGui::Indent(m_inset);
        tabby::TabbyGlobalCfg::get().widgetWidth -= m_inset;
    }

    ScopedInset(const ScopedInset&) = delete;
    ScopedInset& operator=(const ScopedInset&) = delete;

    ~ScopedInset() {
        tabby::TabbyGlobalCfg::get().widgetWidth = m_originalWidgetWidth;
        ImGui::Unindent(m_inset);
    }
};

template <typename F>
void animated(const char* id, bool visible, F&& content, bool inset = true) {
    ImGui::PushID(id);
    const ImGuiID progressId = ImGui::GetID("##VisibilityProgress");
    const ImGuiID heightId = ImGui::GetID("##VisibilityHeight");
    ImGui::PopID();

    auto* storage = ImGui::GetStateStorage();
    float progress = storage->GetFloat(progressId, visible ? 1.0f : 0.0f);
    const float target = visible ? 1.0f : 0.0f;
    const auto& cfg = tabby::TabbyGlobalCfg::get();
    const float step = cfg.playAnimations
                           ? ImGui::GetIO().DeltaTime *
                                 static_cast<float>(cfg.animationSpeed) / 0.18f
                           : 1.0f;

    if (progress < target) {
        progress = std::min(target, progress + step);
    } else if (progress > target) {
        progress = std::max(target, progress - step);
    }
    storage->SetFloat(progressId, progress);

    if (!visible && progress <= 0.0f) {
        return;
    }

    const float eased =
        visible
            ? 1.0f - (1.0f - progress) * (1.0f - progress) * (1.0f - progress)
            : progress * progress * (3.0f - 2.0f * progress);
    const float startCursorY = ImGui::GetCursorPosY();
    const ImVec2 startScreenPos = ImGui::GetCursorScreenPos();
    const float storedHeight =
        storage->GetFloat(heightId, ImGui::GetFrameHeightWithSpacing() * 3.0f);
    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();

    ImGui::PushClipRect(ImVec2(windowPos.x, startScreenPos.y),
                        ImVec2(windowPos.x + windowSize.x,
                               startScreenPos.y + storedHeight * eased),
                        true);

    const double originalWidth = cfg.widgetWidth;
    const float insetWidth = inset ? 20.0f * cfg.uiScale : 0.0f;
    if (inset) {
        ImGui::Indent(insetWidth);
        tabby::TabbyGlobalCfg::get().widgetWidth -= insetWidth;
    }

    content();

    if (inset) {
        tabby::TabbyGlobalCfg::get().widgetWidth = originalWidth;
        ImGui::Unindent(insetWidth);
    }

    const float fullAdvance = ImGui::GetCursorPosY() - startCursorY;
    storage->SetFloat(heightId, fullAdvance);
    ImGui::PopClipRect();
    ImGui::SetCursorPosY(startCursorY + fullAdvance * eased);
}

template <typename F>
bool settingsCard(const char* id, const char* title, bool* enabled,
                  tabby::Font& titleFont, F&& content) {
    auto& cfg = tabby::TabbyGlobalCfg::get();
    const float scale = cfg.uiScale;
    const float outerPadding = 18.0f * scale;
    const float topPadding = ImGui::GetStyle().ItemSpacing.y;
    const float cardSpacing = 8.0f * scale;
    const float cardWidth =
        static_cast<float>(cfg.widgetWidth) + outerPadding * 2.0f;
    const ImVec2 contentStart = ImGui::GetCursorScreenPos();
    const ImVec2 cardStart(contentStart.x - outerPadding, contentStart.y);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGui::PushID(id);
    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + topPadding);

    bool pressed = false;
    {
        tabby::ScopedFont heading(titleFont);
        if (enabled) {
            pressed = tabby::checkbox(title, *enabled).pressed;
        } else {
            tabby::text(title);
        }
    }

    animated(
        "CardBody", enabled == nullptr || *enabled, [&]() { content(); },
        false);

    const ImVec2 cardEnd = ImGui::GetCursorScreenPos();
    drawList->ChannelsSetCurrent(0);
    const bool active = enabled == nullptr || *enabled;
    const ImVec2 clipMin = drawList->GetClipRectMin();
    const ImVec2 clipMax = drawList->GetClipRectMax();
    drawList->PushClipRect(
        ImVec2(std::min(clipMin.x, cardStart.x), clipMin.y),
        ImVec2(std::max(clipMax.x, cardStart.x + cardWidth), clipMax.y), false);
    drawList->AddRectFilled(
        cardStart, ImVec2(cardStart.x + cardWidth, cardEnd.y),
        ImGui::GetColorU32(active ? ImVec4(0.86f, 0.90f, 0.98f, 0.075f)
                                  : ImVec4(0.86f, 0.90f, 0.98f, 0.04f)),
        14.0f * scale);
    drawList->AddRect(
        cardStart, ImVec2(cardStart.x + cardWidth, cardEnd.y),
        ImGui::GetColorU32(active ? ImVec4(0.88f, 0.92f, 1.0f, 0.14f)
                                  : ImVec4(0.88f, 0.92f, 1.0f, 0.08f)),
        14.0f * scale, ImDrawFlags_RoundCornersAll, 1.0f * scale);
    drawList->PopClipRect();

    drawList->ChannelsMerge();
    ImGui::PopID();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + cardSpacing);
    return pressed;
}

void UIManager::draw() {
#ifdef SILICATE_PROTECT
    VMProtectBegin("UIDraw");
#endif

    s_blurEffectParams.clear();

    auto bot = Bot::get();
    auto& cfg = tabby::TabbyGlobalCfg::get();

    ImGuiHookCtx::get().m_time += cocos2d::CCDirector::get()->getDeltaTime();
    const auto popupShaderFn = [&]() {
        renderBlurBg(12.0f, 1.5f, m_state.m_useShader->inner(),
                     m_state.m_opacity->inner());
    };

    const auto tooltipShaderFn = [&]() {
        renderBlurBg(8.0f, 1.5f, m_state.m_useShader->inner(), 0.0, false,
                     0.75f);
    };

    cfg.uiScale = m_state.m_uiScale->inner() * getWindowDpi();
    const bool visible = m_state.m_visible->inner();
    if (!visible) SLBindingManager::get()->stopEdit();

    if (m_state.m_playAnimations->inner()) {
        const float visibilityStep = ImGui::GetIO().DeltaTime *
                                     static_cast<float>(cfg.animationSpeed) /
                                     0.18f;
        if (visible) {
            m_visibilityProgress =
                std::min(1.0f, m_visibilityProgress + visibilityStep);
        } else {
            m_visibilityProgress =
                std::max(0.0f, m_visibilityProgress - visibilityStep);
        }
    } else {
        m_visibilityProgress = visible ? 1.0f : 0.0f;
    }

    tabby::ScopedFont s(m_font);

    if (!visible) {
        auto view = CCEGLView::get();

        auto pl = PlayLayer::get();
        bool shouldHide = false;
        if (pl) {
            shouldHide = !pl->m_isPaused && !pl->m_hasCompletedLevel &&
                         !GameManager::get()->getGameVariable("0024") &&
                         !slui::isInfoModalOpen();
        }

        view->showCursor(!shouldHide);
        slui::drawInfoModal([this]() {
            renderBlurBg(16.0f, 1.5f, m_state.m_useShader->inner(), 0.0, false,
                         0.45f);
        });

        if (m_visibilityProgress <= 0.0f) {
            return;
        }
    } else {
        CCEGLView::get()->showCursor(true);
    }

    const float visibilityAlpha = m_visibilityProgress * m_visibilityProgress *
                                  (3.0f - 2.0f * m_visibilityProgress);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, visibilityAlpha);

    // clip all that start with m_state.m_replayName (case insensitive) (std)

    tabby::window("Silicate", [this, bot, popupShaderFn, tooltipShaderFn]() {
        applyNewStyle();

        if (m_state.m_useShader->inner()) {
            renderBlurBg(24.0f, 2.5f, true, m_state.m_opacity->inner(),
                         !m_theme->m_applyAfter);
        } else {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImGui::GetWindowPos(),
                ImGui::GetWindowPos() + ImGui::GetWindowSize(),
                ImGui::GetColorU32(
                    ImVec4(0.1f, 0.1f, 0.1f, m_state.m_opacity->inner())),
                24.0f * tabby::TabbyGlobalCfg::get().uiScale,
                ImDrawFlags_RoundCornersAll);
        }

        tabby::section(
            "Navigation",
            [&]() {
                double width = tabby::TabbyGlobalCfg::get().widgetWidth;
                auto pre = ImGui::GetCursorScreenPos();
                ImGui::Image(static_cast<ImTextureID>(m_theme->m_texture),
                             ImVec2(width, width * 0.3572923f));

                auto post = ImGui::GetCursorScreenPos();

                auto mp = ImGui::GetMousePos();

                bool mouseIsWithinBounds = pre.y < mp.y && post.y >= mp.y &&
                                           pre.x < mp.x &&
                                           (pre.x + width * 0.35f) > mp.x;

                if (ImGui::IsItemClicked() && mouseIsWithinBounds) {
                    FMODAudioEngine::get()->playEffect("boop.mp3"_spr, 1.0, 0.0,
                                                       1.f);
                }

                tabby::divider(false);

                float uiScale = tabby::TabbyGlobalCfg::get().uiScale;
                tabby::ScopedFont sm(m_medium);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                                    ImVec2(18.0f * uiScale, 9.0f * uiScale));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                                    12.0f * uiScale);

                if (tabby::button_selector(
                        "\uf192    Record##Tab",
                        m_state.m_currentTab == UIState::UITab::Record)
                        .pressed) {
                    m_state.m_currentTab = UIState::UITab::Record;
                }

                if (tabby::button_selector(
                        "\uefba    Assist##Tab",
                        m_state.m_currentTab == UIState::UITab::Assist)
                        .pressed) {
                    m_state.m_currentTab = UIState::UITab::Assist;
                }

                if (tabby::button_selector(
                        "\uf51b    Prediction##Tab",
                        m_state.m_currentTab == UIState::UITab::Prediction)
                        .pressed) {
                    m_state.m_currentTab = UIState::UITab::Prediction;
                }

                if (tabby::button_selector(
                        "\uf01f    Edit##Tab",
                        m_state.m_currentTab == UIState::UITab::Edit)
                        .pressed) {
                    m_state.m_currentTab = UIState::UITab::Edit;
                }

                if (tabby::button_selector(
                        "\uf03d    Render##Tab",
                        m_state.m_currentTab == UIState::UITab::Render)
                        .pressed) {
                    m_state.m_currentTab = UIState::UITab::Render;
                }

                if (tabby::button_selector(
                        "\uf0fe    Keybinds##Tab",
                        m_state.m_currentTab == UIState::UITab::Keybinds)
                        .pressed) {
                    m_state.m_currentTab = UIState::UITab::Keybinds;
                }

                if (tabby::button_selector(
                        "\uf013    Settings##Tab",
                        m_state.m_currentTab == UIState::UITab::Settings)
                        .pressed) {
                    m_state.m_currentTab = UIState::UITab::Settings;
                }

                ImGui::PopStyleVar(2);
            },
            200.0f, true);

        if (m_state.m_currentTab != UIState::UITab::Keybinds) {
            SLBindingManager::get()->stopEdit();
        }

        tabby::section("Main", [&]() {
            if (!Bot::get()->isEnabled()) {
                // the whole lambda spam is genuinely a clanker idiom i really
                // like and i've started adopting in my own human code
                //
                // fight me
                constexpr auto centerAlignText = [](std::string_view text) {
                    auto sz = ImGui::CalcTextSize(text.data());

                    auto w = (ImGui::GetContentRegionAvail().x - sz.x) / 2;
                    w -= ImGui::GetStyle().WindowPadding.x;

                    ImGui::SetCursorPosX(w);

                    tabby::text(text);
                };

                auto h = ImGui::GetContentRegionAvail().y;
                ImGui::SetCursorPosY(
                    h / 2 - (30 * tabby::TabbyGlobalCfg::get().uiScale));

                {
                    tabby::ScopedFont f(m_medium);
                    centerAlignText("Silicate is currently disabled.");
                }

                if (GJBaseGameLayer::get()) {
                    centerAlignText(
                        "Please exit the level you're in to enable the bot.");
                } else {
                    if (tabby::button("Enable").pressed) {
                        Bot::get()->m_enabled->inner() = true;
                        Bot::get()->m_enabled->notifyChange();
                    }
                }

                return;
            }

            tabby::tab(m_state.m_currentTab, UIState::UITab::Record, [&]() {
                tabby::text("Record", m_bold);

                tabby::divider(false);
                auto& rs = Bot::get()->replaySystem();

                if (tabby::input_text_autocomplete(
                        "Replay Name", "Replay", rs.m_replayName,
                        m_replayAutocomplete, popupShaderFn)
                        .changed) {
                    m_replayAutocomplete.suggestions = filterCandidates(
                        m_state.m_replayNames, rs.m_replayName);
                }
                if (m_state.m_lastReplayName != rs.m_replayName) {
                    m_state.m_lastReplayName = rs.m_replayName;
                    m_state.m_replayNames.clear();
                    if (auto iter = asp::fs::iterdir(
                            Mod::get()->getPersistentDir() / "replays");
                        iter.isOk()) {
                        for (const auto& entry : iter.unwrap()) {
                            if (entry.is_regular_file() &&
                                entry.path().extension() == ".slc") {
                                m_state.m_replayNames.push_back(
                                    entry.path().stem().string());
                            }
                        }
                    }
                }

                tabby::fraction(2.0, [&]() {
                    if (tabby::button("Load").pressed) {
                        auto path = rs.getCurrentPath();
                        rs.load(path);
                    }

                    tabby::same_line();

                    if (tabby::button("Save").pressed) {
                        auto path = rs.getCurrentPath();

                        const auto saveReplay = [&rs, this, path]() {
                            rs.save(path);

                            m_state.m_replayNames.clear();
                            if (auto iter = asp::fs::iterdir(
                                    Mod::get()->getPersistentDir() / "replays");
                                iter.isOk()) {
                                for (const auto& entry : iter.unwrap()) {
                                    if (entry.is_regular_file() &&
                                        entry.path().extension() == ".slc") {
                                        m_state.m_replayNames.push_back(
                                            entry.path().stem().string());
                                    }
                                }
                            }
                        };

                        if (SLSettings::get()->confirmSaveOverwrite &&
                            asp::fs::exists(path)) {
                            slui::showYesNoModal({
                                .title = "Overwrite?",
                                .message = fmt::format(
                                    "Are you sure you'd like to overwrite "
                                    "your existing '{}' replay?",
                                    path.filename()),
                                .onYes = saveReplay,
                            });
                        } else {
                            saveReplay();
                        }
                    }
                });

                if (tabby::radio(bot->m_mode, Bot::Mode::Recording,
                                 "Record##Mode")
                        .changed) {
                    // prune all inputs after current frame if in a level
                    if (PlayLayer::get()) {
                        if (rs.getInputIndex() < rs.m_actionAtom.length()) {
                            slui::showYesNoModal(
                                {.title = "Switch to Record?",
                                 .message = "Are you sure you'd like to switch "
                                            "to Record mode? This will remove "
                                            "all inputs after this tick.",
                                 .onYes =
                                     [bot, &rs]() {
                                         rs.createBackup();
                                         rs.m_actionAtom.clipActions(
                                             bot->updater().getFrame());
                                     },
                                 .onNo =
                                     [bot]() {
                                         bot->setMode(Bot::Mode::Playing);
                                     }});
                        }
                    }
                }

                tabby::spacer(16.0);
                tabby::radio(bot->m_mode, Bot::Mode::Playing, "Play##Mode");

                tabby::divider();

                if (tabby::drag("TPS", Bot::get()->updater().m_tps->inner(),
                                0.0, std::numeric_limits<double>::max(), 1.0f,
                                "{:g}")
                        .changed) {
                    Bot::get()->updater().m_tps->notifyChange();
                }

                // Subtick grid (SubTPS): subticks per physics tick. The
                // effective input rate is TPS * SubTPS and is shown beside
                // the drag.
                {
                    auto& cbf = Bot::get()->cbf();
                    m_state.m_physicsModeState.selectedIndex =
                        SLSettings::get()->physicsMode;

                    if (tabby::drag("SubTPS", cbf.m_subtps->inner(), 1u, 4000u,
                                    1.0f, "{}")
                            .changed) {
                        cbf.m_subtps->notifyChange();
                    }
                    slui::tooltip(
                        "Subticks per physics tick. Every input press is "
                        "placed on this many-step subgrid inside the tick "
                        "it belongs to (CBF and CBS physics modes).");

                    tabby::same_line();
                    tabby::text(fmt::format("{} Hz", cbf.totalInputHz()));

                    tabby::spacer(4.0);

                    if (tabby::dropdown("Physics Mode##Record",
                                        m_state.m_physicsModeState,
                                        m_state.m_physicsModeState
                                            .selectedIndex,
                                        popupShaderFn)
                            .changed) {
                        SLSettings::get()->physicsMode =
                            m_state.m_physicsModeState.selectedIndex;

                        cbf.m_physicsMode->notifyChange();
                        cbf.applyMode();
                    }
                    slui::tooltip(
                        "Vanilla: stock GD physics.\n"
                        "CBF: vendored Click Between Frames engine (works "
                        "without the CBF mod).\n"
                        "CBS: RobTop's built-in tick splitting.");
                }

                auto pl = PlayLayer::get();
                if (pl && bot->isRecording() &&
                    m_state.m_showExperimentalFeatures) {
                    if (tabby::button("Add TPS Change").pressed) {
                        (void)bot->replaySystem().m_actionAtom.addAction(
                            bot->updater().getFrame() - 1,
                            bot->updater().getTps());
                        cbf::sidecar::pushRecordedPlaceholder(false);
                        bot->updater().estimatedStepCount = 0;
                    }
                }

                if (tabby::drag("Speed",
                                Bot::get()->updater().m_speedhack->inner(), 0.0,
                                std::numeric_limits<double>::max(), 0.01f,
                                "{:.2G}x")
                        .changed) {
                    Bot::get()->updater().m_speedhack->notifyChange();
                }

                tabby::divider();

                tabby::checkbox("Frame Advance",
                                Bot::get()->updater().m_paused->inner());

                // tabby::overlay([]() {
                //     tabby::checkbox("Frame Advance",
                //                     Bot::get()->updater().m_paused->inner());
                // }, [this, popupShaderFn]() {
                //     tabby::text("keybinds for frame advance");
                //
                //     tabby::dropdown("hi", m_state.m_lockDeltaState,
                //     m_state.m_lockDeltaState.selectedIndex, popupShaderFn,
                //     false);
                // }, popupShaderFn);

                tabby::checkbox("Intentional Death",
                                Bot::get()->updater().m_canDie->inner());

                slui::tooltip(
                    "Record a death input in the replay. The next death or "
                    "restart you perform will be saved as an action in the "
                    "replay.",
                    tooltipShaderFn);

                if (m_state.m_showExperimentalFeatures) {
                    tabby::checkbox(
                        "Frame Extrapolation",
                        Bot::get()->updater().m_extrapolateFrames->inner());

                    slui::tooltip(
                        "Add additional visual frames in between physics ticks "
                        "to allow for a smoother appearance of gameplay. Note "
                        "that this has accuracy issues.",
                        tooltipShaderFn);
                }

                tabby::divider();

                tabby::checkbox("Seed Override",
                                Bot::get()->replaySystem().m_overrideSeed);

                slui::tooltip(
                    "Force a specific seed to always be selected when entering "
                    "a level. Note that the level seed is always saved in the "
                    "replay, no matter if this option is enabled or not.",
                    tooltipShaderFn);

                animated("SeedOverrideOptions",
                         Bot::get()->replaySystem().m_overrideSeed, [&]() {
                             tabby::drag(
                                 "Seed",
                                 Bot::get()->replaySystem().m_overriddenSeed,
                                 0ull, std::numeric_limits<uint64_t>::max());
                         });
            });

            tabby::tab(m_state.m_currentTab, UIState::UITab::Assist, [&]() {
                tabby::text("Assist", m_bold);

                tabby::divider(false);

                auto& hitboxesEnabled =
                    Bot::get()->hitboxes().m_enabled->inner();
                settingsCard(
                    "HitboxesCard", "Hitboxes", &hitboxesEnabled, m_medium,
                    [&]() {
                        tabby::checkbox(
                            "Show Trail##Hitboxes",
                            Bot::get()->hitboxes().m_trailEnabled->inner());

                        if (Bot::get()->hitboxes().m_trailEnabled->inner()) {
                            tabby::checkbox(
                                "Player Hitbox Above Trail##Hitboxes",
                                Bot::get()->hitboxes().m_playerAbove->inner());
                            tabby::checkbox(
                                "Rotated Hitbox Above Trail##Hitboxes",
                                Bot::get()->hitboxes().m_rotatedAbove->inner());
                            tabby::drag("Trail Optimization Factor##Hitboxes",
                                        Bot::get()
                                            ->hitboxes()
                                            .m_optimizationFactor->inner(),
                                        0.f, 100.f, 0.05f, "{:.2f}");
                            slui::tooltip(
                                "Controls trail sampling resolution. Lower "
                                "values merge more nearby segments for better "
                                "performance; higher values preserve detail.");
                        }

                        tabby::drag("Width##Hitboxes",
                                    Bot::get()->hitboxes().m_width->inner(),
                                    0.0, 1.0, 0.02f, "{:.2f}");

                        tabby::dropdown(
                            "Hitbox##Selector", m_state.m_hitboxState,
                            m_state.m_hitboxState.selectedIndex, popupShaderFn);

                        {
                            ScopedInset inset;
                            auto& h =
                                m_state.m_hitboxCategories[m_state.m_hitboxState
                                                               .selectedIndex];

                            auto& category =
                                SLSettings::get()->hitboxes.categories[h];

                            tabby::checkbox("Enabled##SpecificHitbox",
                                            category.enabled);

                            animated(
                                "SpecificHitboxOptions", category.enabled,
                                [&]() {
                                    m_state.m_hitboxColorState.colors =
                                        category.colors;

                                    tabby::color("Color##SpecificHitbox",
                                                 m_state.m_hitboxColorState,
                                                 popupShaderFn);

                                    category.colors =
                                        m_state.m_hitboxColorState.colors;

                                    tabby::drag("Fill Opacity##SpecificHitbox",
                                                category.fillOpacity, 0.0, 1.0,
                                                0.01, "{:.2f}");
                                },
                                false);
                        }
                    });

                settingsCard(
                    "HitzonesCard", "Hitzones", nullptr, m_medium,
                    [&]() {
                        tabby::checkbox(
                            "Show On Death##Hitzones",
                            Bot::get()->hitzones().m_showOnDeath->inner());
                        tabby::checkbox(
                            "Accumulate Across Ticks##Hitzones",
                            Bot::get()->hitzones().m_accumulate->inner());
                        slui::tooltip(
                            "Accumulates the exact hitzone geometry from every "
                            "physics tick. Turning this off only shows the "
                            "current tick; saved history is kept.");

                        tabby::drag(
                            "Fill Opacity##Hitzones",
                            Bot::get()->hitzones().m_fillOpacity->inner(),
                            0.0, 1.0, 0.01, "{:.2f}");
                        tabby::drag(
                            "Border Opacity##Hitzones",
                            Bot::get()->hitzones().m_borderOpacity->inner(),
                            0.0, 1.0, 0.01, "{:.2f}");
                        tabby::drag(
                            "Border Width##Hitzones",
                            Bot::get()->hitzones().m_borderWidth->inner(),
                            0.0, 10.0, 0.1, "{:.2f}");

                        m_state.m_hitzoneSolidColorState.colors =
                            SLSettings::get()->hitzones.solidColor;
                        tabby::color("Solid Color##Hitzones",
                                     m_state.m_hitzoneSolidColorState,
                                     popupShaderFn);
                        SLSettings::get()->hitzones.solidColor =
                            m_state.m_hitzoneSolidColorState.colors;

                        m_state.m_hitzoneHazardColorState.colors =
                            SLSettings::get()->hitzones.hazardColor;
                        tabby::color("Hazard Color##Hitzones",
                                     m_state.m_hitzoneHazardColorState,
                                     popupShaderFn);
                        SLSettings::get()->hitzones.hazardColor =
                            m_state.m_hitzoneHazardColorState.colors;
                    });

                const bool hitzonesOn =
                    Bot::get()->hitzones().m_enabled->inner();
                if (tabby::button(hitzonesOn
                                      ? "Disable Hitzones##HitzonesToggle"
                                      : "Enable Hitzones##HitzonesToggle")
                        .pressed) {
                    Bot::get()->hitzones().m_enabled->inner() = !hitzonesOn;
                }

                // History is only removed by this button or when the game
                // closes, so it survives resets, deaths and level changes.
                if (tabby::button("Clear Saved Hitzones##Hitzones").pressed) {
                    Bot::get()->hitzones().clearHistory();
                }
                slui::tooltip(
                    "Hitzone history is only cleared by this button or when "
                    "the game closes. Resetting, dying, or changing levels "
                    "keeps it.");

                auto& layoutEnabled =
                    Bot::get()->updater().m_layoutMode->inner();
                if (settingsCard(
                        "LayoutCard", "Layout", &layoutEnabled, m_medium,
                        [&]() {
                            tabby::checkbox(
                                "Use Regular Background##LayoutMode",
                                Bot::get()->updater().m_useRegularBg->inner());

                            tabby::color("Background Color##LayoutMode",
                                         m_state.m_bgColorState, popupShaderFn);
                            tabby::color("Ground Color##LayoutMode",
                                         m_state.m_groundColorState,
                                         popupShaderFn);

                            SLSettings::get()->layoutBgColor =
                                m_state.m_bgColorState.colors;
                            SLSettings::get()->layoutGroundColor =
                                m_state.m_groundColorState.colors;
                        })) {
                    Bot::get()->updater().m_layoutMode->notifyChange();
                }

                auto& noclipEnabled = Bot::get()->updater().m_noclip->inner();
                settingsCard("NoclipCard", "Noclip", &noclipEnabled, m_medium,
                             [&]() {
                                 tabby::dropdown(
                                     "Player##Noclip", m_state.m_noclipState,
                                     *reinterpret_cast<int*>(
                                         &Bot::get()->updater().m_noclipType),
                                     popupShaderFn);
                             });

                auto& trajectoryEnabled =
                    Bot::get()->trajectory().m_state.m_enabled->inner();
                settingsCard(
                    "TrajectoryCard", "Trajectory", &trajectoryEnabled,
                    m_medium, [&]() {
                        tabby::drag(
                            "Width##Trajectory",
                            Bot::get()->trajectory().m_state.m_width->inner(),
                            0.0, 1.0, 0.01f, "{:.2f}");
                        tabby::drag(
                            "Length##Trajectory",
                            Bot::get()->trajectory().m_state.m_length->inner(),
                            0.0, 5.0, 0.01f, "{:.2f}s");

                        tabby::checkbox(
                            "Account For Moving Objects",
                            SLSettings::get()
                                ->trajectory.accountForMovingObjects);
                        slui::tooltip(
                            "Run the effect manager's move, rotate, scale and "
                            "follow steps inside the prediction so the "
                            "simulated path reacts to moving objects.");

                        tabby::dropdown("Trajectory##Selector",
                                        m_state.m_trajectoryState,
                                        m_state.m_trajectoryState.selectedIndex,
                                        popupShaderFn);
                        {
                            ScopedInset inset;
                            auto& t =
                                m_state.m_categories[m_state.m_trajectoryState
                                                         .selectedIndex];

                            auto& category =
                                SLSettings::get()->trajectory.categories[t];

                            tabby::checkbox("Enabled##SpecificTrajectory",
                                            category.enabled);

                            animated(
                                "SpecificTrajectoryOptions", category.enabled,
                                [&]() {
                                    m_state.m_trajectoryColorState.colors =
                                        category.colors;

                                    tabby::color("Color##SpecificTrajectory",
                                                 m_state.m_trajectoryColorState,
                                                 popupShaderFn);

                                    category.colors =
                                        m_state.m_trajectoryColorState.colors;
                                },
                                false);
                        }
                    });

                auto& backsteppingEnabled =
                    Bot::get()->updater().m_backwardsStepping->inner();
                if (settingsCard("BacksteppingCard", "Backstepping",
                                 &backsteppingEnabled, m_medium, [&]() {
                                     if (tabby::drag(
                                             "Steps",
                                             Bot::get()
                                                 ->practiceFix()
                                                 .m_maxStoredFrames->inner(),
                                             1u, 2400u)
                                             .changed) {
                                         Bot::get()
                                             ->practiceFix()
                                             .m_maxStoredFrames->notifyChange();
                                     }
                                 })) {
                    if (backsteppingEnabled &&
                        !Bot::get()->updater().m_lockDelta->inner()) {
                        slui::showYesNoModal({
                            .title = "Enable Lock Delta?",
                            .message =
                                "You have enabled backwards stepping, "
                                "but Lock Delta is disabled. Using "
                                "backwards stepping without Lock Delta "
                                "enabled will cause replay breaks and "
                                "inaccuracies.\n\nWould you like to "
                                "enable Lock Delta as well? Note that you may "
                                "also enable it manually in the Settings tab.",
                            .onYes =
                                []() {
                                    Bot::get()->updater().m_lockDelta->inner() =
                                        true;
                                    Bot::get()
                                        ->updater()
                                        .m_lockDelta->notifyChange();
                                },
                        });
                    }
                }

                // tabby::checkbox("Run Full Updates",
                // Bot::get()->updater().m_fullGamePrediction->inner());

                auto& autoclickerEnabled =
                    Bot::get()->autoclicker().m_enabled->inner();
                settingsCard(
                    "AutoclickerCard", "Autoclicker", &autoclickerEnabled,
                    m_medium, [&]() {
                        tabby::checkbox("Sync Both Players",
                                        m_state.m_syncBothAutoclicker);

                        if (!m_state.m_syncBothAutoclicker) {
                            tabby::dropdown(
                                "Player##Autoclicker",
                                m_state.m_autoclickerState,
                                m_state.m_autoclickerState.selectedIndex,
                                popupShaderFn);
                        }

                        Autoclicker::PlayerSettings& s =
                            (m_state.m_autoclickerState.selectedIndex == 0 ||
                             m_state.m_syncBothAutoclicker)
                                ? bot->autoclicker().m_player1
                                : bot->autoclicker().m_player2;

                        {
                            ScopedInset is;

                            tabby::checkbox("Enabled##Autoclickerplayer",
                                            s.m_enabled);

                            tabby::drag("Hold Time##Autoclicker", s.m_holdDelay,
                                        1u,
                                        std::numeric_limits<uint32_t>::max(),
                                        1.0f, "{} Tick(s)");

                            tabby::drag("Release Time##Autoclicker",
                                        s.m_releaseDelay, 1u,
                                        std::numeric_limits<uint32_t>::max(),
                                        1.0f, "{} Tick(s)");

                            tabby::checkbox("Swift Clicks", s.m_performSwifts);
                            slui::tooltip(
                                "Releases each click on the same frame instead "
                                "of "
                                "alternating between a press and release every "
                                "interval.",
                                tooltipShaderFn);

                            tabby::drag("Clicks Per Hold##Autoclicker",
                                        s.m_clicksPerHold, 1u,
                                        std::numeric_limits<uint32_t>::max(),
                                        1.0f, "{} Click(s)");
                            slui::tooltip(
                                "How many clicks to execute per hold action - "
                                "this will add an additional swift click on "
                                "the hold frame for each click above 1",
                                tooltipShaderFn);

                            if (m_state.m_syncBothAutoclicker) {
                                bot->autoclicker().m_player2(s);
                            }
                        }
                    });

                settingsCard("OtherCard", "Other", nullptr, m_medium, [&]() {
                    tabby::checkbox("Mirror Inputs",
                                    Bot::get()->replaySystem().m_mirrorInputs);
                    animated(
                        "MirrorOptions",
                        Bot::get()->replaySystem().m_mirrorInputs, [&]() {
                            tabby::checkbox(
                                "Mirror Inverted",
                                Bot::get()->replaySystem().m_mirrorInverted);
                        });

                    tabby::checkbox(
                        "Maintain Gravity",
                        Bot::get()->replaySystem().m_maintainGravity);

                    slui::tooltip(
                        "Perform wave inputs correctly, so that no matter the "
                        "gravity, the wave can always be controlled as if it "
                        "was normal gravity.",
                        tooltipShaderFn);

                    tabby::checkbox("No Mirror Portals",
                                    Bot::get()->updater().m_noMirror->inner());
                });
            });

            tabby::tab(m_state.m_currentTab, UIState::UITab::Prediction, [&]() {
                tabby::text("Prediction", m_bold);

                tabby::divider(false);

                tabby::text("Automation", m_medium);

                tabby::checkbox("Prevent Death",
                                Bot::get()->updater().m_preventDeath->inner());

                slui::tooltip(
                    "Prevent the player from dying, by pausing the game a "
                    "frame earlier. Requires backstepping to be enabled.",
                    tooltipShaderFn);

                tabby::checkbox(
                    "Use Trajectory##PD",
                    Bot::get()->updater().m_fullGamePrediction->inner());

                slui::tooltip(
                    "Whether to use trajectory to simulate forward to check if "
                    "the player has died instead of using backwards stepping "
                    "when the player has died.",
                    tooltipShaderFn);
                // tabby::checkbox("Auto Flip On Death",
                // Bot::get()->updater().m_autoFlipOnDeath->inner());

                tabby::divider();

                tabby::text("Simulation", m_medium);

                if (tabby::button("Find Best Frame").pressed) {
                    Bot::get()->updater().findBestFrameCandidate();
                }

                slui::tooltip(
                    "When performing the next input, find the frame the player "
                    "would survive the longest had the input been performed. "
                    "Requires backstepping to be enabled.",
                    tooltipShaderFn);

                tabby::drag(
                    "Threshold##Prediction",
                    Bot::get()->updater().m_acceptablePrediction->inner(), 0.0f,
                    1.0f, 0.01f, "{:.2f}");

                slui::tooltip(
                    "A fraction of your trajectory length to use for Find Best "
                    "Frame. When a frame that survives that long is found, go "
                    "to that frame and consider the best frame found.",
                    tooltipShaderFn);

                // tabby::divider();

                // tabby::text("Pathfinder", m_medium);

                // auto& pf = Bot::get()->pathfinder();

                // if (pf.isRunning()) {
                //     if (tabby::button("Stop##Pathfinder").pressed) {
                //         pf.stop();
                //     }

                //     tabby::checkbox("Preview##Pathfinder",
                //     pf.m_renderPreview);

                //     auto pl = PlayLayer::get();
                //     tabby::text(fmt::format("Progress: {} (Attempt {})",
                //     Bot::get()->updater().getFrame(), pl->m_attempts));
                // } else {
                //     if (tabby::button("Start##Pathfinder").pressed) {
                //         pf.start();
                //     }
                // }
            });

            tabby::tab(m_state.m_currentTab, UIState::UITab::Edit, [&]() {
                tabby::text("Edit", m_bold);

                tabby::divider(false);

                auto& replay = Bot::get()->replaySystem().m_actionAtom;
                auto& inputs = replay.m_actions;
                int inputIndex = Bot::get()->replaySystem().getInputIndex();

                if (inputs.empty()) {
                    tabby::text("No replay loaded.");
                }

                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(inputs.size()));

                const bool scrollToInput = inputIndex >= 0 &&
                                           inputIndex < (int)inputs.size() &&
                                           m_state.m_editIndex != inputIndex;
                if (scrollToInput) {
                    m_state.m_editIndex = inputIndex;
                    clipper.IncludeItemsByIndex(inputIndex, inputIndex + 1);
                }

                bool inputsChanged = false;
                while (clipper.Step() && !inputsChanged) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd;
                         i++) {
                        auto& input = inputs[i];
                        if (i == inputIndex) {
                            if (scrollToInput) {
                                ImGui::SetScrollHereY();
                            }

                            ImGui::PushStyleColor(ImGuiCol_Text,
                                                  {1.0, 0.5, 0.5, 1.0});
                        }

                        if (tabby::drag(
                                std::string("Frame##") + std::to_string(i),
                                input.m_frame, 0ull,
                                std::numeric_limits<uint64_t>::max())
                                .changed) {
                            Bot::get()->updater().m_tps->notifyChange();
                            input.recalculateDelta(
                                i == 0 ? 0 : inputs[i - 1].m_frame);
                            if ((int)inputs.size() > i + 1) {
                                inputs[i + 1].recalculateDelta(
                                    inputs[i].m_frame);
                            }
                        }

                        tabby::fraction(
                            2.0,
                            [&]() {
                                if (tabby::checkbox(std::string("Holding##") +
                                                        std::to_string(i),
                                                    input.m_holding)
                                        .pressed) {
                                    Bot::get()->updater().m_tps->notifyChange();
                                }

                                tabby::spacer(16.0);

                                if (tabby::checkbox(std::string("Player 2##") +
                                                        std::to_string(i),
                                                    input.m_player2)
                                        .pressed) {
                                    Bot::get()->updater().m_tps->notifyChange();
                                }

                                if (tabby::button(std::string("Add Below##") +
                                                  std::to_string(i))
                                        .pressed) {
                                    if (i + 1 == (int)inputs.size()) {
                                        inputs.push_back(slc::v3::Action(
                                            inputs[i].m_frame, 0,
                                            slc::v3::Action::ActionType::Jump,
                                            !inputs[i].m_holding,
                                            inputs[i].m_player2));
                                    } else {
                                        inputs.insert(inputs.begin() + i + 1,
                                                      slc::v3::Action(
                                                          inputs[i].m_frame, 0,
                                                          slc::v3::Action::
                                                              ActionType::Jump,
                                                          !inputs[i].m_holding,
                                                          inputs[i].m_player2));
                                    }
                                    inputsChanged = true;
                                }

                                tabby::spacer(16.0);

                                if (tabby::button(std::string("Remove##") +
                                                  std::to_string(i))
                                        .pressed) {
                                    inputs.erase(inputs.begin() + i);
                                    if ((int)inputs.size() > i) {
                                        inputs[i].recalculateDelta(
                                            i == 0 ? 0 : inputs[i - 1].m_frame);
                                    }

                                    if ((int)inputs.size() > i + 1) {
                                        inputs[i + 1].recalculateDelta(
                                            inputs[i].m_frame);
                                    }
                                    inputsChanged = true;
                                }
                            },
                            16.0);

                        tabby::divider();

                        if (i == inputIndex) {
                            ImGui::PopStyleColor();
                        }

                        if (inputsChanged) {
                            break;
                        }
                    }
                }
            });

            tabby::tab(m_state.m_currentTab, UIState::UITab::Render, [&]() {
                tabby::text("Render", m_bold);

                tabby::divider(false);

                auto renderer = Renderer::get();

                if (!renderer->isFFmpegLoaded()) {
                    tabby::text("FFmpeg not loaded.");

                    if (m_ffmpegDownloadProgress < 0.0 &&
                        tabby::button("Download").pressed) {
                        m_ffmpegDownloadProgress = 0.0;
                        geode::log::info("Downloading FFmpeg...");
                        auto req = web::WebRequest();

                        req.onProgress([&](const web::WebProgress& prog) {
                            if (prog.downloadTotal() == 0) {
                                return;
                            }

                            m_ffmpegDownloadProgress =
                                static_cast<double>(prog.downloaded()) /
                                static_cast<double>(prog.downloadTotal());
                        });

                        m_webListener.spawn(
                            req.get(ffmpegUrl), [&](web::WebResponse resp) {
                                const auto data = resp.data();

                                geode::log::info("Verifying checksum...");
                                uint64_t hash = fnv1aHash(data);
                                Sha256 sha = geode::sha256(data);

                                if (hash != hashes::FFMPEG_FNV1A_EXPECTED &&
                                    sha != hashes::FFMPEG_SHA256_EXPECTED) {
                                    geode::log::error(
                                        "Invalid checksum! Aborting FFmpeg "
                                        "loader");
                                    m_ffmpegDownloadProgress = -1.0;
                                    return;
                                }

                                geode::log::info(
                                    "Checksum valid! Unzipping...");
                                auto unzipResult =
                                    geode::utils::file::Unzip::create(data);
                                if (unzipResult.isErr()) {
                                    return;
                                }

                                auto unzip = std::move(unzipResult.unwrap());
                                auto ffmpegDir =
                                    Mod::get()->getTempDir() / "ffmpeg";
                                if (unzip.extractAllTo(ffmpegDir).isErr()) {
                                    return;
                                }

                                auto libDir = Mod::get()->getPersistentDir() /
                                              "libraries";
                                geode::log::info(
                                    "Copying dlls from temp dir `{}`...",
                                    ffmpegDir);

                                (void)asp::fs::createDir(libDir);

                                for (const auto& entry :
                                     asp::fs::iterdir(ffmpegDir).unwrap()) {
                                    if (entry.path().extension() == ".dll") {
                                        if (auto e = asp::fs::copy(
                                                entry,
                                                libDir /
                                                    entry.path().filename(),
                                                std::filesystem::copy_options::
                                                    overwrite_existing);
                                            e.isErr()) {
                                            geode::log::error(
                                                "failed to copy file: {}",
                                                e.unwrapErr().message());
                                        }
                                    }
                                }

                                if (auto e = asp::fs::removeAll(ffmpegDir);
                                    e.isErr()) {
                                    geode::log::error(
                                        "failed to delete temp ffmpeg dir: {}",
                                        e.unwrapErr().message());
                                }

                                Renderer::get()->loadFFmpeg();
                                m_ffmpegDownloadProgress = -1.0;
                            });
                    }

                    if (m_ffmpegDownloadProgress >= 0.95) {
                        tabby::text(
                            "Loading FFmpeg! Please do not close your game...");
                    } else if (m_ffmpegDownloadProgress >= 0.0) {
                        tabby::text(
                            fmt::format("Downloading FFmpeg... {:.1f}%",
                                        m_ffmpegDownloadProgress * 100.0));
                    }

                    return;
                }

                settingsCard(
                    "RenderOtherCard", "Controls", nullptr, m_medium, [&]() {
                        if (renderer->m_autoVideoName->inner()) {
                            tabby::input_text(
                                "Video Template", "Template",
                                Renderer::get()->m_videoNameTemplate->inner());

                            slui::tooltip(
                                "Template to automatically generate "
                                "video titles.\n\nAvailable templates:"
                                "\n\%rand\% - a random number."
                                "\n\%name\% - The level name."
                                "\n\%difficulty\% - The difficulty "
                                "of the level."
                                "\n\%id\% - The ID of the level."
                                "\n\%creator\% - The name of the "
                                "creator of the level.",
                                tooltipShaderFn);
                        } else {
                            tabby::input_text(
                                "Video Name", "Video",
                                Renderer::get()->m_settings.m_outputPath);
                        }

                        tabby::checkbox("Auto Video Name",
                                        renderer->m_autoVideoName->inner());

                        if (renderer->isRecording()) {
                            if (tabby::button("Stop").pressed) {
                                renderer->signalStop();
                            }
                        } else if (!PlayLayer::get() &&
                                   !renderer->m_shouldStart) {
                            if (tabby::button("Start").pressed) {
                                renderer->queueStart();
                            }
                        } else if (renderer->m_shouldStart) {
                            tabby::text("Waiting to enter level...");
                        } else if (PlayLayer::get() &&
                                   tabby::button("Start Here").pressed) {
                            auto res = renderer->start();
                            if (res.isErr()) {
                                geode::log::error(
                                    "Failed to start renderer: {}",
                                    res.unwrapErr());
                                slui::showInfoModal({
                                    .title = "Renderer Error",
                                    .message = fmt::format(
                                        "Failed to start recording: {}",
                                        res.unwrapErr()),
                                });
                            }
                        }

                        tabby::checkbox("Auto Exit on Finish",
                                        renderer->m_autoExitLevel->inner());
                        slui::tooltip(
                            "Automatically leave the level when "
                            "finishing a render.",
                            tooltipShaderFn);
                    });

                settingsCard(
                    "RenderPresetsCard", "Presets", nullptr, m_medium, [&]() {
                        tabby::input_text_autocomplete(
                            "Preset Name", "Preset", m_state.m_presetName,
                            m_state.m_presetAutocomplete, popupShaderFn);

                        tabby::fraction(2.0, [&]() {
                            if (tabby::button("Load##Preset").pressed) {
                                auto path = Mod::get()->getPersistentDir() /
                                            "presets" /
                                            (m_state.m_presetName + ".json");
                                if (asp::fs::exists(path)) {
                                    renderer->loadSettings(path);
                                } else {
                                    geode::log::error(
                                        "Preset file does not exist: "
                                        "{}",
                                        geode::utils::string::pathToString(
                                            path.string()));
                                }
                            }

                            tabby::same_line();

                            if (tabby::button("Save##Preset").pressed) {
                                auto path = Mod::get()->getPersistentDir() /
                                            "presets" /
                                            (m_state.m_presetName + ".json");
                                renderer->saveSettings(path);
                            }
                        });
                    });

                settingsCard(
                    "RenderVideoCard", "Video", nullptr, m_medium, [&]() {
                        tabby::fraction(
                            2.0,
                            [&]() {
                                tabby::drag("Width",
                                            renderer->m_settings.m_width, 1,
                                            10000, 1.0f);
                                tabby::spacer(16.0);
                                tabby::drag("Height",
                                            renderer->m_settings.m_height, 1,
                                            10000, 1.0f);

                                tabby::drag("FPS", renderer->m_settings.m_fps,
                                            1, 1000, 1.0f);
                                tabby::spacer(16.0);
                                if (tabby::drag("Bitrate", m_state.m_bitrate,
                                                0.0, 10000.0, 1.0f, "{:g}Mbps")
                                        .changed) {
                                    renderer->m_settings.m_bitrate = std::round(
                                        m_state.m_bitrate * 1'000'000.0);
                                }
                            },
                            16.0);

                        tabby::input_text("Codec", "Codec",
                                          renderer->m_settings.m_codec);
                        tabby::input_text("Extension", "Extension",
                                          renderer->m_settings.m_extension);
                    });

                auto ar = AudioRecorder::get();
                settingsCard("RenderAudioCard", "Audio", nullptr, m_medium,
                             [&]() {
                                 tabby::checkbox("Audio Preview",
                                                 ar->m_audioPreview->inner());

                                 tabby::drag("Music Volume",
                                             renderer->m_settings.m_musicVolume,
                                             0.0, 1.0, 0.01, "{:.2f}");
                                 tabby::drag("SFX Volume",
                                             renderer->m_settings.m_sfxVolume,
                                             0.0, 1.0, 0.01, "{:.2f}");
                             });

                settingsCard(
                    "RenderFadeCard", "Timing", nullptr, m_medium, [&]() {
                        tabby::drag("Fade In Time",
                                    renderer->m_settings.m_fadeInTime, 0.0,
                                    60.0, 0.1, "{:.1f}s");
                        if (tabby::drag("Fade Out Time",
                                        renderer->m_settings.m_fadeOutTime, 0.0,
                                        60.0, 0.1, "{:.1f}s")
                                .changed) {
                            renderer->m_settings.m_afterEndTime = std::max(
                                (float)renderer->m_settings.m_fadeOutTime,
                                renderer->m_settings.m_afterEndTime);
                        }

                        if (tabby::drag("After End Time",
                                        renderer->m_settings.m_afterEndTime,
                                        0.0f, 10000.0f, 1.0f, "{:.2f}s")
                                .changed) {
                            renderer->m_settings.m_fadeOutTime = std::min(
                                renderer->m_settings.m_fadeOutTime,
                                (double)renderer->m_settings.m_afterEndTime);
                        }
                    });

                settingsCard(
                    "RenderAdvancedCard", "Advanced", nullptr, m_medium, [&]() {
                        tabby::checkbox(
                            "SSB Fix", Bot::get()->updater().m_ssbFix->inner());
                        slui::tooltip(
                            "Compensates audio timing for the game's scroll "
                            "speed bug while rendering.",
                            tooltipShaderFn);

                        if (m_state.m_showExperimentalFeatures) {
                            tabby::drag("Visual FPS",
                                        renderer->m_settings.m_visualFps, 1,
                                        100000, 1.0f);
                        }
                        tabby::checkbox(
                            "Record 1st Attempt Pause",
                            renderer->m_settings.m_firstAttemptPause);

                        tabby::checkbox(
                            "Show Labels While Rendering",
                            bot->updater().m_showLabelsWhileRendering->inner());

                        tabby::input_text("FFmpeg Args", "-preset slow ...",
                                          renderer->m_settings.m_renderArgs);
                    });
            });

            // tabby::tab(m_state.m_currentTab, UIState::UITab::Scripts, [&]() {
            //     tabby::text("Scripts", m_bold);

            //     tabby::divider(false);

            //     tabby::text("Script Manager", m_medium);

            //     tabby::input_text_autocomplete(
            //         "Script Name", "Script", m_state.m_scriptName,
            //         m_state.m_scriptAutocomplete, [&]() {
            //             renderBlurBg(12.0f, 1.5f,
            //             m_state.m_useShader->inner(),
            //                          m_state.m_opacity->inner());
            //         });

            //     if (tabby::button("Load").pressed) {
            //         auto path = Mod::get()->getPersistentDir() / "scripts" /
            //                     (m_state.m_scriptName + ".lua");
            //         if (std::filesystem::exists(path)) {
            //             Bot::get()->scripts().loadScript(path);
            //         } else {
            //             geode::log::error("Script file does not exist: {}",
            //                               path.string());
            //         }
            //     }

            //     tabby::divider();

            //     auto& scripts = Bot::get()->scripts();
            //     for (int i = 0; i < scripts.m_scripts.size(); i++) {
            //         auto& script = scripts.m_scripts[i];
            //         tabby::text(script.m_name);
            //         tabby::checkbox("Enabled##Script" + std::to_string(i),
            //                         script.m_enabled);

            //         tabby::divider();
            //     }
            // });

            tabby::tab(m_state.m_currentTab, UIState::UITab::Keybinds, [&]() {
                auto* b = SLBindingManager::get();
                tabby::text("Keybinds", m_bold);

                tabby::divider(false);

                tabby::input_text("Search##Keybinds", "Search Keybinds...",
                                  m_state.m_keybindSearch);

                if (tabby::button("Add Binding").pressed) {
                    RawKeybind raw{
                        .m_key = 0,
                        .m_modifiers = 0,
                        .m_valueTag = "updater.frame_advance",
                        .m_type = KeybindType::Toggle,
                        .m_active = true,
                        .m_value = "1",
                    };

                    if (auto keybind = b->makeKeybind(raw)) {
                        m_state.m_keybindSearch.clear();
                        b->registerKeybind(keybind);
                        b->startEdit(keybind.get());
                    }
                }

                slui::tooltip(
                    "Creates a keybind for Frame Advance and immediately "
                    "starts listening. You can choose a different action "
                    "below.",
                    tooltipShaderFn);

                tabby::divider();

                std::vector<std::shared_ptr<KeybindControl>> keybinds;
                std::unordered_set<KeybindControl*> liveKeybinds;
                for (const auto& bucket :
                     b->getKeybinds() | std::views::values) {
                    for (const auto& keybind : bucket) {
                        if (!keybind) continue;
                        liveKeybinds.insert(keybind.get());
                        if (keybindMatches(*keybind, m_state.m_keybindSearch)) {
                            keybinds.push_back(keybind);
                        }
                    }
                }

                if (!m_state.m_keybindOrderInitialized) {
                    std::ranges::sort(keybinds, [](const auto& left,
                                                   const auto& right) {
                        return std::tuple(keybindActionName(left->getTag()),
                                          left->getKeyLabel()) <
                               std::tuple(keybindActionName(right->getTag()),
                                          right->getKeyLabel());
                    });
                    for (const auto& keybind : keybinds) {
                        m_state.m_keybindEdits[keybind.get()].order =
                            ++m_state.m_nextKeybindOrder;
                    }
                    m_state.m_keybindOrderInitialized = true;
                } else {
                    for (const auto& keybind : keybinds) {
                        auto& edit = m_state.m_keybindEdits[keybind.get()];
                        if (edit.order == 0) {
                            edit.order = ++m_state.m_nextKeybindOrder;
                        }
                    }
                    std::ranges::sort(
                        keybinds, [this](const auto& left, const auto& right) {
                            return m_state.m_keybindEdits.at(left.get()).order <
                                   m_state.m_keybindEdits.at(right.get()).order;
                        });
                }

                const auto bindableActions = b->getBindableValueTags();
                m_state.m_keybindEditGeneration++;
                std::string currentGroup;

                if (keybinds.empty()) {
                    ImGui::PushStyleColor(
                        ImGuiCol_Text,
                        ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    tabby::text(m_state.m_keybindSearch.empty()
                                    ? "No keybinds yet. Add one to get started."
                                    : "No keybinds match your search.");
                    ImGui::PopStyleColor();
                }

                for (const auto& kb : keybinds) {
                    auto& edit = m_state.m_keybindEdits[kb.get()];
                    const int id = static_cast<int>(edit.order);
                    edit.seen = m_state.m_keybindEditGeneration;
                    if (!edit.initialized) {
                        edit.tag = kb->getTag();
                        edit.value = kb->toString();
                        edit.initialized = true;
                    } else if (m_state.m_editedKeybindValue != kb.get()) {
                        edit.value = kb->toString();
                    }

                    ImGui::PushID(id);
                    const std::string actionName =
                        keybindActionName(kb->getTag());
                    settingsCard(
                        "KeybindCard", actionName.c_str(), nullptr, m_medium,
                        [&]() {
                            const bool isEditing = b->isEditing(kb.get());
                            const std::string captureLabel =
                                fmt::format("{}##Capture{}",
                                            isEditing ? "Press a shortcut..."
                                                      : kb->getKeyLabel(),
                                            id);

                            if (isEditing) {
                                ImGui::PushStyleColor(
                                    ImGuiCol_Button,
                                    ImGui::GetStyleColorVec4(
                                        ImGuiCol_ButtonActive));
                            }
                            if (tabby::button(captureLabel).pressed) {
                                if (isEditing) {
                                    b->stopEdit();
                                } else {
                                    b->startEdit(kb.get());
                                }
                            }
                            if (isEditing) ImGui::PopStyleColor();
                            slui::tooltip(
                                "Click, then press a key combination. Escape "
                                "cancels; Backspace or Delete clears the "
                                "shortcut.",
                                tooltipShaderFn);

                            const bool tagValidBeforeInput =
                                b->hasValue(edit.tag);
                            const std::string tagBeforeInput = edit.tag;
                            edit.actionAutocomplete.suggestions =
                                edit.tag == kb->getTag()
                                    ? bindableActions
                                    : filterKeybindActions(bindableActions,
                                                           edit.tag);
                            if (!tagValidBeforeInput) {
                                ImGui::PushStyleColor(
                                    ImGuiCol_Text,
                                    ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
                            }
                            auto actionState = tabby::input_text_autocomplete(
                                fmt::format("Action##Action{}", id),
                                "Search Actions...", edit.tag,
                                edit.actionAutocomplete, popupShaderFn);
                            const bool actionCommitted =
                                ImGui::IsItemDeactivatedAfterEdit();
                            if (!tagValidBeforeInput) ImGui::PopStyleColor();
                            const bool tagValidAfterInput =
                                b->hasValue(edit.tag);
                            const bool suggestionClicked =
                                edit.tag != tagBeforeInput &&
                                tagValidAfterInput;
                            const bool suggestionAcceptedWithKeyboard =
                                tagValidAfterInput &&
                                (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                                 ImGui::IsKeyPressed(ImGuiKey_KeypadEnter,
                                                     false));
                            slui::tooltip(
                                "The Silicate setting or action controlled by "
                                "this "
                                "keybind. Start typing to see available "
                                "actions.",
                                tooltipShaderFn);

                            if (actionState.held) {
                                m_state.m_editedKeybindTag = kb.get();
                            } else if (m_state.m_editedKeybindTag == kb.get()) {
                                m_state.m_editedKeybindTag = nullptr;
                            }

                            if (tagValidAfterInput &&
                                edit.tag != kb->getTag() &&
                                (actionCommitted || suggestionClicked ||
                                 suggestionAcceptedWithKeyboard ||
                                 !actionState.held)) {
                                m_state.m_pendingRetag = kb.get();
                                m_state.m_pendingRetagTag = edit.tag;
                            }

                            int behavior = static_cast<int>(kb->getType());
                            if (tabby::dropdown(
                                    fmt::format("Behavior##Behavior{}", id),
                                    edit.behaviorDropdown, behavior,
                                    popupShaderFn)
                                    .changed) {
                                b->cancelHeldKeybind(kb.get());
                                kb->setType(static_cast<KeybindType>(behavior));
                            }
                            slui::tooltip(
                                "While held restores the old value on release. "
                                "Toggle switches between the assigned and "
                                "previous "
                                "value. Set once only applies the assigned "
                                "value.",
                                tooltipShaderFn);

                            if (kb->getTypeKey() == keybind::typeKey<bool>()) {
                                bool assignedValue =
                                    edit.value != "0" && edit.value != "false";
                                if (tabby::checkbox(
                                        fmt::format(
                                            "Assigned Value##Assigned{}", id),
                                        assignedValue)
                                        .changed) {
                                    edit.value = assignedValue ? "1" : "0";
                                    kb->fromString(edit.value);
                                }
                                slui::tooltip(
                                    "Choose whether this shortcut assigns the "
                                    "action to on or off.",
                                    tooltipShaderFn);
                            } else {
                                const bool valueValid =
                                    kb->canParse(edit.value);
                                if (!valueValid) {
                                    ImGui::PushStyleColor(
                                        ImGuiCol_Text,
                                        ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
                                }
                                auto valueState = tabby::input_text(
                                    fmt::format("Value##Value{}", id), "Value",
                                    edit.value);
                                if (!valueValid) ImGui::PopStyleColor();
                                slui::tooltip(
                                    "The value assigned to the action when "
                                    "this "
                                    "shortcut activates.",
                                    tooltipShaderFn);

                                if (valueState.held) {
                                    m_state.m_editedKeybindValue = kb.get();
                                } else if (m_state.m_editedKeybindValue ==
                                           kb.get()) {
                                    m_state.m_editedKeybindValue = nullptr;
                                }
                                if (valueState.changed)
                                    kb->fromString(edit.value);
                            }

                            tabby::fraction(2.0, [&]() {
                                bool active = kb->isActive();
                                if (tabby::checkbox(
                                        fmt::format("Enabled##Enabled{}", id),
                                        active)
                                        .changed) {
                                    kb->setActive(active);
                                }

                                tabby::same_line();
                                if (tabby::button(
                                        fmt::format("Remove##Remove{}", id))
                                        .pressed) {
                                    m_state.m_pendingKeybindRemoval = kb.get();
                                }
                            });
                        });
                    ImGui::PopID();
                }

                if (m_state.m_pendingKeybindRemoval != nullptr) {
                    auto* removed = m_state.m_pendingKeybindRemoval;
                    if (m_state.m_pendingRetag == removed) {
                        m_state.m_pendingRetag = nullptr;
                        m_state.m_pendingRetagTag.clear();
                    }
                    b->removeKeybind(removed);
                    m_state.m_keybindEdits.erase(removed);
                    if (m_state.m_editedKeybindTag == removed)
                        m_state.m_editedKeybindTag = nullptr;
                    if (m_state.m_editedKeybindValue == removed)
                        m_state.m_editedKeybindValue = nullptr;
                    m_state.m_pendingKeybindRemoval = nullptr;
                }

                if (m_state.m_pendingRetag != nullptr) {
                    auto* previous = m_state.m_pendingRetag;
                    if (auto* replacement = b->retagKeybind(
                            previous, m_state.m_pendingRetagTag)) {
                        liveKeybinds.erase(previous);
                        liveKeybinds.insert(replacement);
                        auto editNode =
                            m_state.m_keybindEdits.extract(previous);
                        if (!editNode.empty()) {
                            auto replacementEdit = std::move(editNode.mapped());
                            replacementEdit.tag = replacement->getTag();
                            replacementEdit.value = replacement->toString();
                            replacementEdit.actionAutocomplete = {};
                            replacementEdit.seen =
                                m_state.m_keybindEditGeneration;
                            m_state.m_keybindEdits.emplace(
                                replacement, std::move(replacementEdit));
                        }

                        if (m_state.m_editedKeybindTag == previous) {
                            m_state.m_editedKeybindTag = replacement;
                        }

                        if (m_state.m_editedKeybindValue == previous) {
                            m_state.m_editedKeybindValue = replacement;
                        }
                    }

                    m_state.m_pendingRetag = nullptr;
                    m_state.m_pendingRetagTag.clear();
                }

                std::erase_if(m_state.m_keybindEdits, [this, &liveKeybinds](
                                                          const auto& item) {
                    if (liveKeybinds.contains(item.first)) {
                        return false;
                    }

                    if (m_state.m_editedKeybindTag == item.first) {
                        m_state.m_editedKeybindTag = nullptr;
                    }

                    if (m_state.m_editedKeybindValue == item.first) {
                        m_state.m_editedKeybindValue = nullptr;
                    }

                    return true;
                });
            });

            tabby::tab(m_state.m_currentTab, UIState::UITab::Settings, [&]() {
                tabby::text("Settings", m_bold);

                tabby::divider(false);

                tabby::text("Interface", m_medium);

                if (tabby::button("Open Silicate Folder").pressed) {
                    geode::utils::file::openFolder(
                        Mod::get()->getPersistentDir());
                }

                tabby::checkbox("Use Glass Shader",
                                m_state.m_useShader->inner());

                if (tabby::dropdown("Theme", m_state.m_themeState,
                                    m_state.m_themeState.selectedIndex,
                                    popupShaderFn)
                        .changed) {
                    SLSettings::get()->theme =
                        m_state.m_themeState.selectedIndex;
                    m_theme = &s_themes[m_state.m_themeState.selectedIndex];
                    m_theme->apply();
                }

                if (tabby::checkbox("Play Animations",
                                    m_state.m_playAnimations->inner())
                        .pressed) {
                    m_state.m_playAnimations->notifyChange();
                }

                if (tabby::drag("Animation Speed",
                                m_state.m_animationSpeed->inner(), 0.1f, 3.0f,
                                0.01f, "{:.2f}")
                        .changed) {
                    m_state.m_animationSpeed->notifyChange();
                }

                if (tabby::drag("UI Scale", m_state.m_uiScale->inner(), 0.1f,
                                10.0f, 0.01f, "{:.2f}x")
                        .changed) {
                    m_state.m_uiScale->notifyChange();
                }

                // if (m_state.m_restartGameInfo) {
                //     ImGui::PushStyleColor(ImGuiCol_Text,
                //                           ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
                //     tabby::text(
                //         "It's recommended to restart the game after "
                //         "changing "
                //         "UI Scale.");
                //     ImGui::PopStyleColor();
                // }

                if (tabby::drag("UI Opacity", m_state.m_opacity->inner(), 0.1f,
                                1.0f, 0.01f, "{:.2f}")
                        .changed) {
                    m_state.m_opacity->notifyChange();
                }

                if (tabby::checkbox("Ask For Overwrite Confirmation",
                                    SLSettings::get()->confirmSaveOverwrite)
                        .pressed) {
                    if (!SLSettings::get()->confirmSaveOverwrite) {
                        slui::showYesNoModal({
                            .title = "Disable Overwrite Confirmation?",
                            .message =
                                "Are you sure you'd like to disable overwrite "
                                "confirmation? Silicate will no longer ask "
                                "you to confirm saving replays when "
                                "overwriting them. Only enable this if you "
                                "know what you're doing - you may lose your "
                                "replay this way!",
                            .onNo =
                                []() {
                                    SLSettings::get()->confirmSaveOverwrite =
                                        true;
                                },
                        });
                    }
                }

                tabby::divider();

                tabby::text("Keybinds", m_medium);

                auto settings = SLSettings::get();
                tabby::drag("Repeat Frequency##Keybinds",
                            settings->keybindRepeatFrequency, 1.0, 1000.0, 1.0f,
                            "{:.0f} RPS");
                slui::tooltip(
                    "How often held Toggle and Override keybinds activate "
                    "after the hold delay.",
                    tooltipShaderFn);

                tabby::drag("Hold Trigger Delay##Keybinds",
                            settings->keybindHoldDelayMs, 0.0, 5000.0, 10.0f,
                            "{:.0f}ms");
                slui::tooltip(
                    "How long a Toggle or Override key must be held before it "
                    "starts repeating. A normal press still activates once "
                    "immediately.",
                    tooltipShaderFn);

                tabby::divider();

                tabby::text("Backups", m_medium);

                tabby::checkbox(
                    "Auto-Save At Level End",
                    Bot::get()->replaySystem().m_autosaveAtLevelEnd->inner());

                slui::tooltip(
                    "Whether to save the replay you're recording when "
                    "completing the level. Note that if a replay like that "
                    "already exists, it makes a backup instead.",
                    tooltipShaderFn);

                auto& rs = Bot::get()->replaySystem();
                if (tabby::checkbox("Auto-Backup",
                                    Bot::get()
                                        ->replaySystem()
                                        .m_autosaveAtInterval->inner())
                        .pressed) {
                    rs.m_autosaveAtInterval->notifyChange();
                }

                animated("AutoBackupOptions", rs.m_autosaveAtInterval->inner(),
                         [&]() {
                             if (tabby::drag("Auto-Backup Interval",
                                             rs.m_autosaveInterval->inner(),
                                             1.0, 3600.0, 1.0, "{:.0f}s")
                                     .changed) {
                                 rs.m_autosaveInterval->notifyChange();
                             }
                         });

                tabby::divider();

                tabby::text("Labels", m_medium);

                tabby::checkbox("Display Labels",
                                Bot::get()->labels().m_globalEnabled->inner());

                animated(
                    "GlobalLabelOptions",
                    Bot::get()->labels().m_globalEnabled->inner(),
                    [&]() {
                        tabby::dropdown("Label##Selector", m_state.m_labelState,
                                        m_state.m_labelState.selectedIndex,
                                        popupShaderFn);

                        ScopedInset inset;
                        auto& label =
                            Bot::get()
                                ->labels()
                                .m_labels[m_state.m_labelState.selectedIndex]
                                .m_config;

                        if (tabby::checkbox("Enabled##Label", label.m_enabled)
                                .pressed) {
                            bot->labels().m_requiresRefresh = true;
                        }

                        animated(
                            "SpecificLabelOptions", label.m_enabled,
                            [&]() {
                                if (tabby::drag("Opacity##Label",
                                                label.m_opacity, 0.0f, 1.0f,
                                                0.01f, "{:.2f}x")
                                        .changed) {
                                    bot->labels().m_requiresRefresh = true;
                                }

                                if (tabby::drag("Size##Label", label.m_scale,
                                                0.0f, 100.0f, 1.0f, "{:.2f}x")
                                        .changed) {
                                    bot->labels().m_requiresRefresh = true;
                                }

                                if (tabby::dropdown(
                                        "Font##Label",
                                        m_state.m_labelFontsState,
                                        *reinterpret_cast<int*>(&label.m_font),
                                        popupShaderFn)
                                        .changed) {
                                    bot->labels().m_requiresRefresh = true;
                                }

                                if (tabby::dropdown(
                                        "Position##Label",
                                        m_state.m_labelPositionsState,
                                        *reinterpret_cast<int*>(
                                            &label.m_anchor),
                                        popupShaderFn)
                                        .changed) {
                                    bot->labels().m_requiresRefresh = true;
                                }
                            },
                            false);
                    },
                    false);

                tabby::divider();

                tabby::text("Playback", m_medium);

                tabby::checkbox("Block Inputs During Playback",
                                bot->replaySystem().m_ignoreInputs->inner());
                tabby::checkbox("Audio Speedhack",
                                bot->updater().m_speedhackAudio->inner());

                tabby::divider();

                tabby::text("Updater", m_medium);

                if (tabby::checkbox("Lock Delta",
                                    bot->updater().m_lockDelta->inner())
                        .pressed) {
                    if (!bot->updater().m_lockDelta->inner()) {
                        slui::showInfoModal({
                            .title = "Warning",
                            .message =
                                "You are disabling Lock Delta. This may "
                                "cause physics to run incorrectly, skip "
                                "frames, or cause other accuracy "
                                "issues. Disable this option with care. "
                                "Disabling this option also causes backwards "
                                "stepping to drastically lose accuracy.",
                        });
                    }
                }
                slui::tooltip(
                    "Runs physics with a fixed delta time to improve replay "
                    "consistency.",
                    tooltipShaderFn);

                tabby::checkbox("Real Time",
                                bot->updater().m_realTime->inner());

                slui::tooltip(
                    "Removes any UPR restrictions and allows the game to have "
                    "as many physics ticks as it wants per frame. This ensures "
                    "no slowdown, although for laggier levels will show "
                    "visible lag.",
                    tooltipShaderFn);

                animated(
                    "NonRealtimeOptions", !bot->updater().m_realTime->inner(),
                    [&]() {
                        tabby::checkbox("Dynamic UPR",
                                        bot->updater().m_dynamicUpr->inner());
                        slui::tooltip(
                            "Automatically adjusts the update limit per visual "
                            "frame to maintain the target FPS.",
                            tooltipShaderFn);

                        animated(
                            "DynamicUprOptions",
                            bot->updater().m_dynamicUpr->inner(),
                            [&]() {
                                tabby::drag("Target FPS",
                                            bot->updater().m_fpsTarget->inner(),
                                            1.0, 480.0, 1.0f, "{:.0f} FPS");
                            },
                            false);
                        animated(
                            "StaticUprOptions",
                            !bot->updater().m_dynamicUpr->inner(),
                            [&]() {
                                tabby::drag("Max UPR",
                                            bot->updater().m_maxUPR->inner(),
                                            1u, 1000000u, 1.0f);

                                slui::tooltip(
                                    "How many physics ticks are allowed to run "
                                    "per visual frame.",
                                    tooltipShaderFn);

                                tabby::checkbox(
                                    "Use Visual Updates",
                                    bot->updater().m_useVisualUpdates->inner());

                                slui::tooltip(
                                    "Multiplies Max UPR with the expected "
                                    "number of ticks per visual frame for your "
                                    "current TPS. A max UPR of 1 with this "
                                    "option on will result in real time "
                                    "playback, minus lag.",
                                    tooltipShaderFn);
                            },
                            false);
                    },
                    false);

                tabby::divider();

                tabby::text("Miscellaneous", m_medium);

                tabby::checkbox(
                    "Use Alternate Input Hook",
                    bot->replaySystem().m_useAlternateHook->inner());
                slui::tooltip(
                    "Records inputs directly from the button handler. This "
                    "option is here for mod compatibility.",
                    tooltipShaderFn);

                if (tabby::button("Disable Bot").pressed) {
                    Bot::get()->m_enabled->inner() = false;
                    Bot::get()->m_enabled->notifyChange();
                    FMOD::ChannelGroup* master;
                    FMODAudioEngine::get()->m_system->getMasterChannelGroup(
                        &master);
                    master->setPitch(1.0);
                }

                slui::tooltip(
                    "Disable all of Silicate's features. Note that you must be "
                    "outside of a level for this button to do anything.",
                    tooltipShaderFn);

                tabby::checkbox("Experimental Features",
                                m_state.m_showExperimentalFeatures);
                slui::tooltip(
                    "Enables features that are not fully tested and may be "
                    "unstable.",
                    tooltipShaderFn);
            });
        });

        // window border
        ImGui::GetWindowDrawList()->AddRect(
            ImGui::GetWindowPos(),
            ImGui::GetWindowPos() + ImGui::GetWindowSize(),
            ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.1f)),
            24.0f * tabby::TabbyGlobalCfg::get().uiScale,
            ImDrawFlags_RoundCornersAll,
            2.5 * tabby::TabbyGlobalCfg::get().uiScale);

        slui::drawYesNoModal([this]() {
            renderBlurBg(16.0f, 1.5f, m_state.m_useShader->inner(), 0.0, false,
                         0.45f);
        });

        slui::drawInfoModal([this]() {
            renderBlurBg(16.0f, 1.5f, m_state.m_useShader->inner(), 0.0, false,
                         0.45f);
        });

        if (m_theme->m_applyAfter) {
            renderBlurBg(24.0f, 2.5f, true, m_state.m_opacity->inner(), true,
                         1.0f, true);
        }
    });

    ImGui::PopStyleVar();

#ifdef SILICATE_PROTECT
    VMProtectEnd();
#endif
}
