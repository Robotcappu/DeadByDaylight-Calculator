#pragma once

#include <string>
#include <vector>

namespace DbDOfferings
 {
    struct OfferingsEntry {
        std::string name;       // Name of the Offering
        float multiplier;       // Multiplier of the OFfering
        std::string category;   // Category of the Offering
        std::string tooltip;    // Description
    };
    
    inline const std::vector<OfferingsEntry> offerings = {

        // ======== OBJECTIVES ========
        {"Screech Cobbler", 1.08f, "Event Offering", "Testoffering"}
    };
}