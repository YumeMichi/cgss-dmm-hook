#include "stdinclude.hpp"

#include "config.hpp"

namespace config {
    namespace {
        OverrideUrls g_urls;
        bool g_loaded = false;

        std::string get_config_path() {
            char path[MAX_PATH] = {};
            DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
            if (len == 0 || len >= MAX_PATH) {
                return {};
            }

            char* last_sep = path;
            for (char* p = path; *p != '\0'; ++p) {
                if (*p == '\\' || *p == '/') {
                    last_sep = p;
                }
            }
            *last_sep = '\0';
            lstrcatA(path, "\\config.json");
            return path;
        }

        std::string normalize_base_url(std::string value) {
            if (value.empty()) {
                return value;
            }

            constexpr const char* kHttp = "http://";
            constexpr const char* kHttps = "https://";

            if (value.rfind(kHttp, 0) == 0) {
                value.erase(0, lstrlenA(kHttp));
            } else if (value.rfind(kHttps, 0) == 0) {
                value.erase(0, lstrlenA(kHttps));
            }

            while (!value.empty() && value.front() == '/') {
                value.erase(value.begin());
            }

            if (value.empty()) {
                return value;
            }

            if (value.back() != '/') {
                value.push_back('/');
            }

            return value;
        }

        void read_string(const rapidjson::Document& document, const char* key, std::string& out) {
            if (!document.HasMember(key) || !document[key].IsString()) {
                return;
            }
            out = normalize_base_url(document[key].GetString());
        }
    }

    void load() {
        if (g_loaded) {
            return;
        }
        g_loaded = true;

        const auto config_path = get_config_path();
        if (config_path.empty()) {
            hook_log("[cgss-http-hook] failed to resolve config.json path");
            return;
        }

        std::ifstream config_stream(config_path);
        if (!config_stream.is_open()) {
            hook_log("[cgss-http-hook] config.json not found, using defaults");
            return;
        }

        rapidjson::IStreamWrapper wrapper(config_stream);
        rapidjson::Document document;
        document.ParseStream(wrapper);
        if (document.HasParseError() || !document.IsObject()) {
            hook_log("[cgss-http-hook] failed to parse config.json");
            return;
        }

        read_string(document, "api_url", g_urls.api_url);
        read_string(document, "asset_url", g_urls.asset_url);

        if (!g_urls.api_url.empty()) {
            hook_logf("[cgss-http-hook] normalized api_url=%s", g_urls.api_url.c_str());
        }
        if (!g_urls.asset_url.empty()) {
            hook_logf("[cgss-http-hook] normalized asset_url=%s", g_urls.asset_url.c_str());
        }
    }

    const OverrideUrls& get() {
        return g_urls;
    }
}
