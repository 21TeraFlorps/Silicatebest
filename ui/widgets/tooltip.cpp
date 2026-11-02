#include "tooltip.hpp"

#include <algorithm>
#include <tabby.hpp>
#include <unordered_map>

#include "imgui.h"

namespace {

struct TooltipState {
    float hoverDuration = 0.0f;
    float opacity = 0.0f;
    ImVec2 position{};
    int lastSeenFrame = 0;
};

std::unordered_map<ImGuiID, TooltipState> s_tooltipStates;
int s_lastCleanupFrame = 0;

void cleanUnusedStates(int frame) {
    if (frame - s_lastCleanupFrame < 300) {
        return;
    }

    std::erase_if(s_tooltipStates, [frame](const auto& entry) {
        return entry.second.opacity <= 0.0f &&
               frame - entry.second.lastSeenFrame > 300;
    });
    s_lastCleanupFrame = frame;
}

}  // namespace

namespace slui {

void tooltip(std::string_view description, const std::function<void()>& renderBackground, TooltipOptions options) {
    ImGuiID itemId = ImGui::GetItemID();
    if (itemId == 0) {
        itemId = ImGui::GetID(description.data(),
                              description.data() + description.size());
    }

    const int frame = ImGui::GetFrameCount();
    cleanUnusedStates(frame);

    auto& state = s_tooltipStates[itemId];
    state.lastSeenFrame = frame;

    const float deltaTime = ImGui::GetIO().DeltaTime;
    const bool hovered = ImGui::IsItemHovered(
        ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_NoNavOverride);

    if (hovered) {
        state.hoverDuration += deltaTime;
        const float scale = tabby::TabbyGlobalCfg::get().uiScale;
        const ImVec2 mousePosition = ImGui::GetMousePos();
        state.position = ImVec2(mousePosition.x + 18.0f * scale,
                                mousePosition.y + 14.0f * scale);
    } else {
        state.hoverDuration = 0.0f;
    }

    const float targetOpacity =
        hovered && state.hoverDuration >= options.delay ? 1.0f : 0.0f;
    if (options.fadeDuration <= 0.0f) {
        state.opacity = targetOpacity;
    } else {
        const float fadeStep = deltaTime / options.fadeDuration;
        if (state.opacity < targetOpacity) {
            state.opacity = std::min(targetOpacity, state.opacity + fadeStep);
        } else {
            state.opacity = std::max(targetOpacity, state.opacity - fadeStep);
        }
    }

    if (state.opacity <= 0.0f) {
        return;
    }

    const float easedOpacity =
        state.opacity * state.opacity * (3.0f - 2.0f * state.opacity);
    const float scale = tabby::TabbyGlobalCfg::get().uiScale;
    const ImGuiStyle& style = ImGui::GetStyle();

    ImGui::SetNextWindowPos(state.position);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, style.Alpha * easedOpacity);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(18.0f * scale, 12.0f * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f * scale);
    if (renderBackground) {
        ImGui::PushStyleColor(ImGuiCol_PopupBg,
                              ImVec4(0.055f, 0.06f, 0.075f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.88f, 0.92f, 1.0f, 0.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_PopupBg,
                              ImVec4(0.055f, 0.06f, 0.075f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.88f, 0.92f, 1.0f, 0.18f));
    }

    if (ImGui::BeginTooltip()) {
        if (renderBackground) renderBackground();

        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() +
                               options.maxWidth * scale);
        ImGui::TextUnformatted(description.data(),
                               description.data() + description.size());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

}  // namespace slui
