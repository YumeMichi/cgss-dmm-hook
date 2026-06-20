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

        void read_bool(const rapidjson::Document& document, const char* key, bool& out) {
            if (!document.HasMember(key) || !document[key].IsBool()) {
                return;
            }
            out = document[key].GetBool();
        }
    }

    void load() {
        if (g_loaded) {
            return;
        }
        g_loaded = true;

        const auto config_path = get_config_path();
        if (config_path.empty()) {
            hook_log("[cgss-dmm-hook] failed to resolve config.json path");
            return;
        }

        std::ifstream config_stream(config_path);
        if (!config_stream.is_open()) {
            hook_log("[cgss-dmm-hook] config.json not found, using defaults");
            return;
        }

        rapidjson::IStreamWrapper wrapper(config_stream);
        rapidjson::Document document;
        document.ParseStream(wrapper);
        if (document.HasParseError() || !document.IsObject()) {
            hook_log("[cgss-dmm-hook] failed to parse config.json");
            return;
        }

        read_bool(document, "force_http", g_urls.force_http);
        read_string(document, "api_url", g_urls.api_url);
        read_string(document, "asset_url", g_urls.asset_url);

        hook_logf("[cgss-dmm-hook] force_http=%s", g_urls.force_http ? "true" : "false");

        if (!g_urls.api_url.empty()) {
            hook_logf("[cgss-dmm-hook] normalized api_url=%s", g_urls.api_url.c_str());
        }
        if (!g_urls.asset_url.empty()) {
            hook_logf("[cgss-dmm-hook] normalized asset_url=%s", g_urls.asset_url.c_str());
        }
    }

    const OverrideUrls& get() {
        return g_urls;
    }
}
