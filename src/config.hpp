#pragma once

namespace config {
    struct OverrideUrls {
        bool force_http = true;
        std::string api_url;
        std::string asset_url;
        bool launch_borderless_helper = true;
    };

    void load();
    const OverrideUrls& get();
}
