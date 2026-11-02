#pragma once

#include <string_view>

namespace slui {

struct TooltipOptions {
    float delay = 0.75f;
    float fadeDuration = 0.16f;
    float maxWidth = 300.0f;
};

// Attach a tooltip to the widget drawn immediately before this call.
void tooltip(std::string_view description,
             const std::function<void()>& renderBackground = {},
             TooltipOptions options = TooltipOptions{});

}  // namespace slui
