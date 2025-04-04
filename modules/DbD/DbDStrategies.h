// DbDStrategies.h
#pragma once
#include <string>
#include <vector>

namespace DbDStrategies
{
    struct StrategyEntry
    {
        std::string name;
        std::string tooltip;
        int altruismBonus;
        int boldnessBonus;
        int objectivesBonus;
        int survivalBonus;
        bool active = false;
    };

    inline std::vector<StrategyEntry> strategies = {
        {"BlightRushDodge", "Safe Unhook after Blight Rush dodge (10k Altruism)", 10000, 0, 0, 0}
    };
}
