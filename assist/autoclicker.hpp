#pragma once
#include <Geode/Geode.hpp>

#include "../settings/settings.hpp"
#include "../shared/value/value.hpp"

using namespace geode::prelude;

class Autoclicker {
   private:
       void update(PlayLayer* pl, bool player2);

   public:
    class PlayerSettings {
       public:
        uint32_t m_holdDelay = 1;
        uint32_t m_releaseDelay = 1;
        uint32_t m_clicksPerHold = 1;

        bool m_performSwifts = false;
        bool m_enabled = true;

        void operator()(const PlayerSettings& other) {
            m_holdDelay = other.m_holdDelay;
            m_releaseDelay = other.m_releaseDelay;
            m_clicksPerHold = other.m_clicksPerHold;
            m_performSwifts = other.m_performSwifts;
            m_enabled = other.m_enabled;
        }

        bool isClicking() const { return m_clicked; }

       private:
        uint64_t m_lastFrame = UINT64_MAX;
        bool m_clicked = false;

        friend class Autoclicker;
    };

    PlayerSettings m_player1{};
    PlayerSettings m_player2{};

    SLValuePtr<bool> m_enabled = SLValue<bool>::create(
        "autoclicker.enabled", &SLSettings::get()->autoclickerEnabled);

    void reset() {
        m_player1.m_lastFrame = UINT64_MAX;
        m_player2.m_lastFrame = UINT64_MAX;
    }

    void update(PlayLayer* pl);
    void update(PlayerObject* player, PlayerSettings& state, bool isP2, uint64_t tick, bool queueButtons = true);

    bool performPlayer1() { return m_player1.m_enabled; }
    bool performPlayer2() { return m_player2.m_enabled; }
};
