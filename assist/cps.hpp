#pragma once

#include <queue>
#include <slc/slc.hpp>

class CPSCounter {
   public:
    struct CPSData {
        std::queue<double> m_timestampsP1;
        std::queue<double> m_timestampsP2;

        int m_maxCPSP1;
        int m_maxCPSP2;
    };

   private:
    CPSData m_data;

   public:
    int queryCPS(int player = 1);
    int queryMaxCPS(int player = 1);

    void clearActions();
    void pushAction(bool player2);

    void replaceTimestamps(const CPSData& data) { m_data = data; }
    void replaceTimestamps(CPSData&& data) { m_data = std::move(data); }
    CPSData const& getData() const { return m_data; }

    void update();
};
