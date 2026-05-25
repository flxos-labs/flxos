#pragma once

#include <flx/core/EventBus.hpp>
#include <string>
#include <string_view>

namespace flx::connectivity {

/// Typed WiFi events published on EventBus topic "connectivity.wifi"
enum class WiFiEvent {
	RadioEnabled, ///< WiFi radio turned on
	RadioDisabled, ///< WiFi radio turned off
	ScanStarted, ///< Background scan initiated
	ScanFinished, ///< Background scan completed
	Connecting, ///< Attempting to connect to SSID
	Connected, ///< Got IP — fully connected
	Disconnected, ///< Lost connection (reason may be in bundle)
	AuthFailed, ///< Wrong password / auth failure
	NotFound, ///< SSID not found after retries
};

/// Helper to publish typed WiFi events to the EventBus.
/// All events carry "event" (int) and optionally "ssid" and "reason" fields.
class WiFiEvents {
public:

	static constexpr std::string_view TOPIC = "connectivity.wifi";

	static void publish(WiFiEvent event, const std::string& ssid = "", int reason = 0) {
		flx::core::Bundle data;
		data.putInt32("event", static_cast<int32_t>(event));
		if (!ssid.empty()) {
			data.putString("ssid", ssid);
		}
		if (reason != 0) {
			data.putInt32("reason", reason);
		}
		flx::core::EventBus::getInstance().publish(std::string(TOPIC), data);
	}
};

} // namespace flx::connectivity
