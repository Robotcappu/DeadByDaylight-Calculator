#pragma once

#include <string>
#include <vector>

namespace DbDBloodpoints
{
    // Maximalpunkte für Progressbars (kategorieweite Caps)
    constexpr int MaxPointsPerCategory = 10000;

    struct ObjectiveEntry
    {
        std::string name;       // Name of the Objective
        float bloodpoints;        // Bloodpoints per Action
        int cap;                // Cap for the Objective. If there is no Cap, it returns 0
        std::string category;   // Category of the Objective
        std::string tooltip;    // Description
        bool noCap;             // returns if ther is no Bloodpoints Cal (true= no Cap, false = Cap)
    };

    // ============================
    // ========== Survivor =========
    // ============================

    // Some commented out bc they are not applying for all survivors

    inline const std::vector<ObjectiveEntry> SurvivorObjectives = {

        // ======== OBJECTIVES ========
        {"Repair Generator", 20.0f, 2000, "Objectives", "Repair Generators (20 BP per %, max 2000)", false},
        {"Open Exit Gate", 20.0f, 2000, "Objectives", "Open Exit Gates (20 BP per %, max 2000)", false},
        {"Chest Unlocking", 7.5f, 750, "Objectives", "Unlock Chests (7.5 BP per %, max 750)", false},
        {"Escape Hatch", 2500.0f, 0, "Objectives", "Escape through the Hatch (+2500 BP)", false},
        {"Invocation Participation", 15.0f, 1500, "Objectives", "Participate in Invocation (15 BP per %, max 1500)", false},
        {"Coop Action (Objectives)", 25.0f, 0, "Objectives", "Perform Coop Actions (+25 BP per tick, no cap)", true},
    
        // ======== ALTRUISM ==========
        {"Heal Survivor", 7.5f, 750, "Altruism", "Heal other Survivors (7.5 BP per %, max 750)", false},
        {"Safe Hook Rescue", 1250.0f, 0, "Altruism", "Safe Hook or Cage Rescue (+1250 BP per action, no cap)", true},
        {"Protection Hit", 200.0f, 0, "Altruism", "Take a Protection Hit (+200 BP per hit, no cap)", true},
        {"Fully Mended Others", 100.0f, 0, "Altruism", "Mend others from Deep Wound (+100 BP per action, no cap)", true},
        {"Coop Action (Altruism)", 25.0f, 0, "Altruism", "Perform Coop Actions in Altruism (+25 BP per tick, no cap)", true},
    
        // ======== BOLDNESS ==========
        {"Cleanse Totem", 1500.0f, 1500, "Boldness", "Cleanse Dull or Hex Totem (+1500 BP, max 1500)", true},
        {"Blessing", 1500.0f, 0, "Boldness", "Bless a Totem (+1500 BP, no cap)", true},
        {"Chase Time", 50.0f, 0, "Boldness", "Earn BP during Chases (50 BP per second, no cap)", true},
        {"Near Miss", 250.0f, 0, "Boldness", "Near Miss with Killer (+250 BP, no cap)", true},
        {"Escape Chase", 750.0f, 0, "Boldness", "Escape from Killer during Chase (+750 BP, no cap)", true},
        {"Pallet Stun", 500.0f, 0, "Boldness", "Stun Killer with Pallet (+500 BP per action, no cap)", true},
        {"Special Stun", 1000.0f, 0, "Boldness", "Special Stun (e.g. perks) (+1000 BP per action, no cap)", true},
        {"Vault During Chase", 100.0f, 1500, "Boldness", "Vault during Chase (+100 BP, max 1500)", false},
    
        // ======== SURVIVAL ==========
        {"Self Heal", 7.5f, 750, "Survival", "Self-Heal without Medkit (7.5 BP per %, max 750)", false},
        {"Claw Trap Escape", 1000.0f, 0, "Survival", "Escape while Claw Trap against Skull Merchant (+1000 BP)", false},
        {"Hook Escape", 1500.0f, 0, "Survival", "Self-Unhook (+1500 BP, no cap)", true},
        {"Killer Grasp Escape", 1250.0f, 0, "Survival", "Wiggle out of Killer's Grasp (+1250 BP, no cap)", true},
        {"Survive Trial", 7000.0f, 0, "Survival", "Escape the Trial (+7000 BP, no cap)", true},
        {"Recovery in Dying State", 3.16f, 316, "Survival", "Recover in Dying State (3.16 BP per %, max 316)", false}
    };
    

    // ============================
    // ========== Killer ==========
    // ============================

    inline const std::vector<ObjectiveEntry> KillerObjectives = {
        // --- Brutality ---
        {"Damage Survivor", 500, 8000, "Brutality", "Successfully hit a Survivor (+500 BP, max 8000)"},
        {"Break Pallet", 400, 4000, "Brutality", "Break a Pallet (+400 BP, max 4000)"},
        {"Break Generator", 400, 4000, "Brutality", "Damage a Generator (+400 BP, max 4000)"},

        // --- Deviousness ---
        {"Use Power", 600, 8000, "Deviousness", "Use Killer Power successfully (+600 BP, max 8000)"},
        {"Set Trap", 1200, 4000, "Deviousness", "Place a successful Trap (+1200 BP, max 4000)"},

        // --- Hunter ---
        {"Chase Survivor", 1000, 4000, "Hunter", "Chase a Survivor (+1000 BP, max 4000)"},
        {"Down Survivor", 500, 4000, "Hunter", "Down a Survivor (+500 BP, max 4000)"},
        {"Hook Survivor", 500, 4000, "Hunter", "Successfully hook a Survivor (+500 BP, max 4000)"},

        // --- Gatekeeper ---
        {"Generator Defense", 500, 8000, "Gatekeeper", "Defend Generators over time (+500 BP, max 8000)"}
    };
}