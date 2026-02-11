#pragma once

#include "AppManifest.hpp"
#include "AppRegistry.hpp"
#include "Bundle.hpp"
#include <string>

namespace System::Apps {

/**
 * @brief Standard intent actions
 *
 * Intents declaratively describe what an app should do, similar to Android Intents.
 * Intents declaratively describe what an app should do, similar to Android Intents.
 */
namespace IntentAction {
constexpr const char* ACTION_MAIN = "MAIN"; // Launch app's main activity
constexpr const char* ACTION_VIEW = "VIEW"; // View data (e.g. open a file)
constexpr const char* ACTION_EDIT = "EDIT"; // Edit data
constexpr const char* ACTION_SEND = "SEND"; // Send/share data
constexpr const char* ACTION_PICK = "PICK"; // Pick an item and return it
} // namespace IntentAction

/**
 * @brief Describes what to launch and with what data
 *
 * Can target a specific app (via targetAppId) or be resolved to the best
 * matching app based on action + mimeType using the AppRegistry.
 */
struct Intent {
	std::string action = IntentAction::ACTION_MAIN;
	std::string data; // File path, URL, or other data reference
	std::string mimeType; // e.g. "text/plain", "image/png"
	Bundle extras; // Additional structured data
	std::string targetAppId; // Explicit target app (empty = resolve via registry)

	// Convenience constructors

	/** Launch a specific app by ID */
	static Intent forApp(const std::string& appId) {
		return Intent {IntentAction::ACTION_MAIN, "", "", {}, appId};
	}

	/** View a file or data item */
	static Intent view(const std::string& data, const std::string& mimeType = "") {
		return Intent {IntentAction::ACTION_VIEW, data, mimeType, {}, ""};
	}

	/** Edit a file or data item */
	static Intent edit(const std::string& data, const std::string& mimeType = "") {
		return Intent {IntentAction::ACTION_EDIT, data, mimeType, {}, ""};
	}

	/** Pick an item (expects result) */
	static Intent pick(const std::string& mimeType = "") {
		return Intent {IntentAction::ACTION_PICK, "", mimeType, {}, ""};
	}
};

/**
 * @brief Resolves an Intent to the best matching app manifest
 *
 * Resolution priority:
 * 1. If targetAppId is set → use that app directly
 * 2. If mimeType is set → search AppRegistry for apps supporting that MIME type
 * 3. Fallback → no match (returns nullopt)
 */
class IntentResolver {
public:

	/**
	 * Resolve the intent to a matching AppManifest.
	 * @return The best matching manifest, or nullopt if no app can handle it.
	 */
	static std::optional<AppManifest> resolve(const Intent& intent) {
		auto& registry = AppRegistry::getInstance();

		// 1. Explicit target
		if (!intent.targetAppId.empty()) {
			return registry.findById(intent.targetAppId);
		}

		// 2. MIME type matching
		if (!intent.mimeType.empty()) {
			auto matches = registry.getForMimeType(intent.mimeType);
			if (!matches.empty()) {
				return matches.front(); // Return highest-priority match
			}
		}

		// 3. No match
		return std::nullopt;
	}

	/**
	 * Get all apps that can handle an intent (for disambiguation dialogs).
	 */
	static std::vector<AppManifest> resolveAll(const Intent& intent) {
		auto& registry = AppRegistry::getInstance();

		if (!intent.targetAppId.empty()) {
			auto result = registry.findById(intent.targetAppId);
			if (result) return {*result};
			return {};
		}

		if (!intent.mimeType.empty()) {
			return registry.getForMimeType(intent.mimeType);
		}

		return {};
	}
};

} // namespace System::Apps
