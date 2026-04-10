#include <flx/flxapp/FlxAppManifest.hpp>

#include <flx/core/Logger.hpp>
#include <flx/core/Value.hpp>
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

std::string stringOr(const flx::core::FlxValueView& object, const char* key, const char* fallback = "") {
    if (key == nullptr) {
        return fallback;
    }
    return object.child(key).asString(fallback);
}

int intOr(const flx::core::FlxValueView& object, const char* key, int fallback = 0) {
    if (key == nullptr) {
        return fallback;
    }
    return static_cast<int>(object.child(key).asInt64(fallback));
}

void appendStringArray(const flx::core::FlxValueView& object, const char* key,
                       std::vector<std::string>& out) {
    if (key == nullptr) {
        return;
    }
    const flx::core::FlxValueView array = object.child(key);
    if (!array.valid() || !array.isSeq()) {
        return;
    }
    array.forEachChild([&out](const flx::core::FlxValueView& item) {
        if (item.hasValue() && !item.isNull()) {
            out.emplace_back(item.asString());
        }
    });
}

uint16_t parseFlags(const flx::core::FlxValueView& appObject) {
    uint16_t flags = flx::apps::AppFlags::None;
    if (appObject.child("hidden").asBool()) {
        flags |= flx::apps::AppFlags::Hidden;
    }
    if (appObject.child("single_instance").asBool()) {
        flags |= flx::apps::AppFlags::SingleInstance;
    }
    return flags;
}

flx::apps::AppCapability parseCapabilities(const flx::core::FlxValueView& appObject) {
    const flx::core::FlxValueView capabilities = appObject.child("capabilities");
    if (!capabilities.valid() || !capabilities.isSeq()) {
        return flx::apps::AppCapability::None;
    }
    flx::apps::AppCapability parsed = flx::apps::AppCapability::None;
    capabilities.forEachChild([&parsed](const flx::core::FlxValueView& item) {
        if (!item.hasValue() || item.isNull()) {
            return;
        }
        const std::string value = item.asString();
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
    });
    return parsed;
}

std::optional<flx::apps::AppManifest> buildManifest(const flx::core::FlxValueView& root,
                                                     const std::string& path) {
    const flx::core::FlxValueView appObject = root.child("app");
    if (!appObject.valid() || !appObject.isMap()) {
        Log::error(TAG, "Missing 'app' object in FlxApp: %s", path.c_str());
        return std::nullopt;
    }

    flx::apps::AppManifest manifest;
    manifest.appId          = stringOr(appObject, "id");
    manifest.appName        = stringOr(appObject, "name");
    manifest.appVersionName = stringOr(appObject, "version", "0.1.0");
    manifest.appVersionCode = static_cast<uint32_t>(intOr(appObject, "version_code", 1));
    manifest.description    = stringOr(appObject, "description");
    manifest.sortPriority   = intOr(appObject, "sort_priority", 200);
    manifest.flags          = parseFlags(appObject);
    manifest.capabilities   = parseCapabilities(appObject);
    manifest.category       = flx::apps::AppCategory::External;
    manifest.location       = flx::apps::AppLocation::external(path);
    manifest.minHeapKb      = static_cast<uint16_t>(intOr(appObject, "min_heap_kb", 0));

    static constexpr const char* defaultIcon  = LV_SYMBOL_FILE;
    static constexpr const char* settingsIcon = LV_SYMBOL_LIST;
    const std::string icon = stringOr(appObject, "icon");
    manifest.appIcon = (icon == "list") ? settingsIcon : defaultIcon;

    appendStringArray(appObject, "required_services",   manifest.requiredServices);
    appendStringArray(appObject, "mime_types",          manifest.supportedMimeTypes);
    appendStringArray(appObject, "url_schemes",         manifest.urlSchemes);

    if (manifest.appId.empty() || manifest.appName.empty()) {
        Log::error(TAG, "FlxApp metadata missing id or name: %s", path.c_str());
        return std::nullopt;
    }

    return manifest;
}

} // namespace

std::optional<flx::apps::AppManifest> FlxAppManifest::loadFromFile(const std::string& path) {
    const std::string raw = readFile(path);
    if (raw.empty()) {
        Log::error(TAG, "Failed to read FlxApp file: %s", path.c_str());
        return std::nullopt;
    }

    auto document = flx::core::FlxValueDocument::parseAuto(std::move(raw));
    if (!document) {
        Log::error(TAG, "Failed to parse FlxApp manifest: %s", path.c_str());
        return std::nullopt;
    }

    return buildManifest(document->root(), path);
}

} // namespace flx::flxapp
