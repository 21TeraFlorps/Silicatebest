#include "autoclicker.hpp"

#include "bot/bot.hpp"
#include "bot/updater.hpp"

void Autoclicker::update(PlayerObject* player, PlayerSettings& s, bool player2, uint64_t tick, bool queueButtons) {
        if (!PlayLayer::get()) {
            s.m_clicked = false;
            return;
        }

        if (!s.m_enabled || !m_enabled->inner()) {
            if (s.m_clicked) {
                if (queueButtons) {
                    player->m_gameLayer->queueButton(1, false, player2, 0.0);
                } else {
                    player->releaseButton(PlayerButton::Jump);
                }

                s.m_clicked = false;
            }

            return;
        }

        auto bot = Bot::get();
        if (!bot->isRecording()) {
            return;
        }

        if (tick == s.m_lastFrame) {
            return;
        }

        uint64_t nextEventTick =
            s.m_lastFrame + (s.m_clicked ? s.m_holdDelay : s.m_releaseDelay);

        if (tick >= nextEventTick || s.m_lastFrame == UINT64_MAX) {
            s.m_lastFrame = tick;

            auto* pl = player->m_gameLayer;

            if (!s.m_clicked) {
                if (queueButtons) {
                    pl->queueButton(1, true, player2, 0.0);
                } else {
                    player->pushButton(PlayerButton::Jump);
                }

                for (size_t i = 0; i < s.m_clicksPerHold - 1; i++) {
                    if (queueButtons) {
                        pl->queueButton(1, false, player2, 0.0);
                        pl->queueButton(1, true, player2, 0.0);
                    } else {
                        player->releaseButton(PlayerButton::Jump);
                        player->pushButton(PlayerButton::Jump);
                    }
                }

                s.m_clicked = true;
            } else {
                if (queueButtons) {
                    pl->queueButton(1, false, player2, 0.0);
                } else {
                    player->releaseButton(PlayerButton::Jump);
                }

                s.m_clicked = false;

                return;
            }

            if (s.m_performSwifts && s.m_clicked) {
                if (queueButtons) {
                    pl->queueButton(1, false, player2, 0.0);
                } else {
                    player->releaseButton(PlayerButton::Jump);
                }

                s.m_clicked = false;
            }
        }
}

void Autoclicker::update(PlayLayer* pl, bool player2) {
    PlayerSettings& s = player2 ? m_player2 : m_player1;
    PlayerObject* player = player2 ? pl->m_player2 : pl->m_player1;
    update(player, s, player2, Bot::get()->updater().getFrame(), true);
}

void Autoclicker::update(PlayLayer* pl) {
    update(pl, false);
    update(pl, true);
}
