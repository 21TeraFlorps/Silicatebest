#include "info_modal.hpp"

#include <Geode/binding/FMODAudioEngine.hpp>
#include <tabby.hpp>
#include <utility>

#include "bot/bot.hpp"
#include "imgui.h"
#include "ui/manager.hpp"

namespace {

constexpr const char* POPUP_ID = "Silicate Confirmation##InfoModal";

struct ModalState {
    slui::InfoModalConfig config;
    bool open = false;
    bool requestOpen = false;
    bool pausedAudio = false;

    bool hide = false;
};

ModalState s_modal;

void pauseAudio() {
    auto* engine = FMODAudioEngine::get();
    if (engine == nullptr || engine->m_system == nullptr) return;

    FMOD::ChannelGroup* master = nullptr;
    if (engine->m_system->getMasterChannelGroup(&master) != FMOD_OK ||
        master == nullptr) {
        return;
    }

    bool alreadyPaused = false;
    if (master->getPaused(&alreadyPaused) != FMOD_OK || alreadyPaused) return;

    s_modal.pausedAudio = master->setPaused(true) == FMOD_OK;
}

void restoreAudio() {
    if (!s_modal.pausedAudio) return;

    auto* engine = FMODAudioEngine::get();
    if (engine == nullptr || engine->m_system == nullptr) return;

    FMOD::ChannelGroup* master = nullptr;
    if (engine->m_system->getMasterChannelGroup(&master) == FMOD_OK &&
        master != nullptr) {
        master->setPaused(false);
    }
}

}  // namespace

namespace slui {

void showInfoModal(InfoModalConfig config) {
    const bool wasOpen = s_modal.open;
    s_modal.config = std::move(config);
    s_modal.open = true;
    s_modal.requestOpen = true;
    s_modal.hide = false;
    if (!wasOpen) pauseAudio();
}

bool isInfoModalOpen() { return s_modal.open; }

void hideInfoModal() {
    s_modal.hide = true;
}

void drawInfoModal(const std::function<void()>& renderBackground) {
    if (!s_modal.open) return;

    if (s_modal.requestOpen) {
        ImGui::OpenPopup(POPUP_ID);
        s_modal.requestOpen = false;
    }

    const float scale = tabby::TabbyGlobalCfg::get().uiScale;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float width = 390.0f * scale;

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f),
                                        ImVec2(width, viewport->WorkSize.y));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg,
                          ImVec4(0.0f, 0.0f, 0.0f, 0.55f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,
                          renderBackground
                              ? ImVec4(0.055f, 0.06f, 0.075f, 0.0f)
                              : ImVec4(0.055f, 0.06f, 0.075f, 0.985f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.88f, 0.92f, 1.0f, 0.18f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(22.0f * scale, 20.0f * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 16.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(10.0f * scale, 14.0f * scale));

    bool closePopup = s_modal.hide;
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    if (renderBackground) flags |= ImGuiWindowFlags_NoBackground;

    if (ImGui::BeginPopupModal(POPUP_ID, nullptr, flags)) {
        if (renderBackground) renderBackground();

        {
            tabby::ScopedFont f(Bot::get()->ui().m_medium);

            ImGui::TextUnformatted(s_modal.config.title.c_str());
        }

        ImGui::Separator();

        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + width - 44.0f * scale);
        ImGui::TextUnformatted(s_modal.config.message.c_str());
        ImGui::PopTextWrapPos();

        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float buttonWidth = ImGui::GetContentRegionAvail().x - spacing;

        if (ImGui::Button(s_modal.config.okLabel.c_str(), ImVec2(buttonWidth, 0.0f))) {
            closePopup = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            closePopup = true;
        }

        if (closePopup) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(3);

    if (!closePopup) return;

    restoreAudio();
    s_modal = {};
}

}  // namespace slui
