#pragma once

namespace config {
    struct OverrideUrls {
        std::string api_url;
        std::string asset_url;
    };

    void load();
    const OverrideUrls& get();
}
