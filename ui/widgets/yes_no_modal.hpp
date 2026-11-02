#pragma once

#include <functional>
#include <string>

namespace slui {

struct YesNoModalConfig {
    std::string title;
    std::string message;
    std::string yesLabel = "Yes";
    std::string noLabel = "No";
    std::function<void()> onYes = {};
    std::function<void()> onNo = {};
};

// Shows one confirmation modal that hijacks the whole UI. Opening another replaces the
// pending modal. Escape dismisses the modal through the same path as "No".
void showYesNoModal(YesNoModalConfig config);

bool isYesNoModalOpen();
void drawYesNoModal(const std::function<void()>& renderBackground = {});

}  // namespace slui
