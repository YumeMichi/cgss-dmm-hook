#pragma once

namespace config {
    struct OverrideUrls {
        bool force_http = true;
        std::string api_url;
        std::string asset_url;
    };

    void load();
    const OverrideUrls& get();
}
