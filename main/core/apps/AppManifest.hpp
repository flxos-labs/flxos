#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace System::Apps {

// Forward declaration
class App;

/**
 * @brief Categories for organizing apps in the launcher and registry
 */
enum class AppCategory : uint8_t {
	System, // Core system apps (Settings, SystemInfo)
	Settings, // Settings-related apps
	Tools, // Utility apps (Calculator, Stopwatch)
	User, // User-installed internal apps
	External // Dynamically loaded from SD card / ELF
};

/**
 * @brief Capability flags declaring what system resources an app needs
 * Capability flags declaring what system resources an app needs
 */
enum class AppCapability : uint16_t {
	None = 0,
	WiFi = 1 << 0,
	Bluetooth = 1 << 1,
	Storage = 1 << 2,
	Camera = 1 << 3,
	GPIO = 1 << 4,
	I2C = 1 << 5,
	SPI = 1 << 6,
	UART = 1 << 7,
};

// Bitwise operators for AppCapability
inline AppCapability operator|(AppCapability a, AppCapability b) {
	return static_cast<AppCapability>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
inline AppCapability operator&(AppCapability a, AppCapability b) {
	return static_cast<AppCapability>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}
inline bool hasCapability(AppCapability caps, AppCapability check) {
	return (static_cast<uint16_t>(caps) & static_cast<uint16_t>(check)) != 0;
}

/**
 * @brief Flags controlling app behavior
 */
struct AppFlags {
	static constexpr uint16_t None = 0;
	static constexpr uint16_t HideStatusBar = 1 << 0; // Hide status bar when app is active
	static constexpr uint16_t Hidden = 1 << 1; // Don't show in launcher
	static constexpr uint16_t SingleInstance = 1 << 2; // Only one instance allowed
	static constexpr uint16_t Background = 1 << 3; // Can run in background
};

/**
 * @brief Describes where an app is located (compiled-in or external)
 */
struct AppLocation {
	std::string path; // Empty = internal (compiled into firmware)

	bool isInternal() const { return path.empty(); }
	bool isExternal() const { return !path.empty(); }

	static AppLocation internal() { return AppLocation {""}; }
	static AppLocation external(const std::string& p) { return AppLocation {p}; }
};

/**
 * @brief App manifest describing an application's identity, capabilities, and metadata
 *
 * Enhanced with:
 * - Capability/permission system
 * - App dependencies on services
 * - Memory hints for resource management
 * - MIME type and URL scheme handlers for intent routing
 * - Sort priority for launcher ordering
 * - Rich description for App Hub display
 */
struct AppManifest {
	// === Core Identity ===
	std::string appId; // Unique ID, e.g. "com.flxos.settings"
	std::string appName; // Human-readable name, e.g. "Settings"
	const char* appIcon = nullptr; // LVGL symbol, e.g. LV_SYMBOL_SETTINGS
	std::string appVersionName; // Semantic version, e.g. "1.1.0"
	uint32_t appVersionCode = 1; // Incrementing version number

	// === Classification ===
	AppCategory category = AppCategory::User;
	uint16_t flags = AppFlags::None;
	AppLocation location = AppLocation::internal();

	// === Rich Metadata ===
	std::string description; // App description for App Hub
	int sortPriority = 100; // Lower = shown first in launcher (0 = top)

	// === Permissions & Resources ===
	AppCapability capabilities = AppCapability::None;
	uint16_t minHeapKb = 0; // Minimum heap needed (0 = no hint)
	uint16_t stackSizeKb = 0; // Recommended task stack (0 = default)

	// === Dependencies & Intents ===
	std::vector<std::string> requiredServices; // Required services, e.g. {"wifi"}
	std::vector<std::string> supportedMimeTypes; // e.g. {"text/*", "image/png"}
	std::vector<std::string> urlSchemes; // e.g. {"flxos://settings"}

	// === Factory ===
	std::function<std::shared_ptr<App>()> createApp;
};

} // namespace System::Apps
