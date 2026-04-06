#include <flx/flxapp/FlxAppManifest.hpp>

#include <cJSON.h>
#include <flx/core/Logger.hpp>
#include <font/lv_symbol_def.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace flx::flxapp {

namespace {

constexpr const char* TAG = "FlxAppManifest";

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string jsonStringOr(const cJSON* object, const char* key, const char* fallback = "") {
    if (object == nullptr) {
        return fallback;
    }

    const cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
        return item->valuestring;
    }

    return fallback;
}

int jsonIntOr(const cJSON* object, const char* key, int fallback = 0) {
    if (object == nullptr) {
        return fallback;
    }

    const cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsNumber(item)) {
        return item->valueint;
    }

    return fallback;
}

void appendStringArray(const cJSON* object, const char* key, std::vector<std::string>& out) {
    if (object == nullptr) {
        return;
    }

    const cJSON* array = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsArray(array)) {
        return;
    }

    const cJSON* item = nullptr;
    cJSON_ArrayForEach(item, array) {
        if (cJSON_IsString(item) && item->valuestring != nullptr) {
            out.emplace_back(item->valuestring);
        }
    }
}

uint16_t parseFlags(const cJSON* appObject) {
    uint16_t flags = flx::apps::AppFlags::None;

    const cJSON* hidden = cJSON_GetObjectItemCaseSensitive(appObject, "hidden");
    if (cJSON_IsBool(hidden) && cJSON_IsTrue(hidden)) {
        flags |= flx::apps::AppFlags::Hidden;
    }

    const cJSON* singleInstance = cJSON_GetObjectItemCaseSensitive(appObject, "single_instance");
    if (cJSON_IsBool(singleInstance) && cJSON_IsTrue(singleInstance)) {
        flags |= flx::apps::AppFlags::SingleInstance;
    }

    return flags;
}

flx::apps::AppCapability parseCapabilities(const cJSON* appObject) {
    const cJSON* capabilities = cJSON_GetObjectItemCaseSensitive(appObject, "capabilities");
    if (!cJSON_IsArray(capabilities)) {
        return flx::apps::AppCapability::Storage;
    }

    flx::apps::AppCapability parsed = flx::apps::AppCapability::None;
    const cJSON* item = nullptr;
    cJSON_ArrayForEach(item, capabilities) {
        if (!cJSON_IsString(item) || item->valuestring == nullptr) {
            continue;
        }

        std::string value = item->valuestring;
        if (value == "wifi") {
            parsed = parsed | flx::apps::AppCapability::WiFi;
        } else if (value == "bluetooth") {
            parsed = parsed | flx::apps::AppCapability::Bluetooth;
        } else if (value == "storage") {
            parsed = parsed | flx::apps::AppCapability::Storage;
        } else if (value == "camera") {
            parsed = parsed | flx::apps::AppCapability::Camera;
        } else if (value == "gpio") {
            parsed = parsed | flx::apps::AppCapability::GPIO;
        } else if (value == "i2c") {
            parsed = parsed | flx::apps::AppCapability::I2C;
        } else if (value == "spi") {
            parsed = parsed | flx::apps::AppCapability::SPI;
        } else if (value == "uart") {
            parsed = parsed | flx::apps::AppCapability::UART;
        }
    }

    return parsed;
}

} // namespace

std::optional<flx::apps::AppManifest> FlxAppManifest::loadFromFile(const std::string& path) {
    const std::string raw = readFile(path);
    if (raw.empty()) {
        Log::error(TAG, "Failed to read FlxApp file: %s", path.c_str());
        return std::nullopt;
    }

    cJSON* root = cJSON_Parse(raw.c_str());
    if (root == nullptr) {
        Log::error(TAG, "Failed to parse FlxApp document as JSON: %s", path.c_str());
        return std::nullopt;
    }

    const cJSON* appObject = cJSON_GetObjectItemCaseSensitive(root, "app");
    if (!cJSON_IsObject(appObject)) {
        Log::error(TAG, "Missing 'app' object in FlxApp: %s", path.c_str());
        cJSON_Delete(root);
        return std::nullopt;
    }

    flx::apps::AppManifest manifest;
    manifest.appId = jsonStringOr(appObject, "id");
    manifest.appName = jsonStringOr(appObject, "name");
    manifest.appVersionName = jsonStringOr(appObject, "version", "0.1.0");
    manifest.appVersionCode = static_cast<uint32_t>(jsonIntOr(appObject, "version_code", 1));
    manifest.description = jsonStringOr(appObject, "description");
    manifest.sortPriority = jsonIntOr(appObject, "sort_priority", 200);
    manifest.flags = parseFlags(appObject);
    manifest.capabilities = parseCapabilities(appObject);
    manifest.category = flx::apps::AppCategory::External;
    manifest.location = flx::apps::AppLocation::external(path);
    manifest.minHeapKb = static_cast<uint16_t>(jsonIntOr(appObject, "min_heap_kb", 0));

    static constexpr const char* defaultIcon = LV_SYMBOL_FILE;
    static constexpr const char* settingsIcon = LV_SYMBOL_LIST;
    const std::string icon = jsonStringOr(appObject, "icon");
    if (icon == "list") {
        manifest.appIcon = settingsIcon;
    } else {
        manifest.appIcon = defaultIcon;
    }

    appendStringArray(appObject, "required_services", manifest.requiredServices);
    appendStringArray(appObject, "mime_types", manifest.supportedMimeTypes);
    appendStringArray(appObject, "url_schemes", manifest.urlSchemes);

    cJSON_Delete(root);

    if (manifest.appId.empty() || manifest.appName.empty()) {
        Log::error(TAG, "FlxApp metadata missing id or name: %s", path.c_str());
        return std::nullopt;
    }

    return manifest;
}

} // namespace flx::flxapp
