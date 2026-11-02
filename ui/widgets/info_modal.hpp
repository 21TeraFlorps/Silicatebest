#pragma once

#include <functional>
#include <string>

namespace slui {

struct InfoModalConfig {
    std::string title;
    std::string message;
    std::string okLabel = "OK";
};

void showInfoModal(InfoModalConfig config);
void hideInfoModal();

bool isInfoModalOpen();
void drawInfoModal(const std::function<void()>& renderBackground = {});

}  // namespace slui
