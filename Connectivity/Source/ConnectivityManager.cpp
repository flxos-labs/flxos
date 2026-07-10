#include "flx/connectivity/ConnectivityManager.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"

#if FLXOS_WIFI_ENABLED
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "flx/connectivity/hotspot/HotspotManager.hpp"
#include "flx/connectivity/wifi/WiFiCredentialStore.hpp"
#include "flx/connectivity/wifi/WiFiManager.hpp"
#include "flx/connectivity/wifi/WiFiProvisioning.hpp"
#endif

#if FLXOS_BLUETOOTH_ENABLED
#include "flx/connectivity/bluetooth/BluetoothManager.hpp"
#endif
#include <cstdint>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <string_view>

static constexpr std::string_view TAG = "Connectivity";

namespace flx::connectivity {

const flx::services::ServiceManifest ConnectivityManager::serviceManifest = {
	.serviceId = "com.flxos.connectivity",
	.serviceName = "Connectivity",
	.dependencies = {"com.flxos.settings"},
	.priority = 30,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::WiFi | flx::services::ServiceCapability::Bluetooth,
	.description = "WiFi, Hotspot, and Bluetooth connectivity",
};

bool ConnectivityManager::onStart() {
	Log::info(TAG, "Initializing networking stack...");
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

#if FLXOS_WIFI_ENABLED
	esp_netif_create_default_wifi_sta();
	esp_netif_create_default_wifi_ap();
	wifi_init_config_t const cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	// Initialize sub-managers with observable references
	WiFiManager::getInstance().init(&m_wifi_connected_subject, &m_wifi_ssid_subject, &m_wifi_ip_subject, &m_wifi_status_subject);
	HotspotManager::getInstance().init(&m_hotspot_enabled_subject, &m_hotspot_clients_subject);
#endif

#if FLXOS_BLUETOOTH_ENABLED
	BluetoothManager::getInstance().init(&m_bluetooth_enabled_subject);
#endif

	// Register config settings for persistence
	flx::system::SettingsManager::getInstance().registerSetting("hs_ssid", m_hotspot_ssid_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_pass", m_hotspot_password_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_chan", m_hotspot_channel_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_max", m_hotspot_max_conn_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_hide", m_hotspot_hidden_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_auth", m_hotspot_auth_subject);
	flx::system::SettingsManager::getInstance().registerSetting("wifi_enabled", m_wifi_enabled_subject);
	flx::system::SettingsManager::getInstance().registerSetting("wifi_scan_int", m_wifi_scan_interval_subject);
	flx::system::SettingsManager::getInstance().registerSetting("wifi_ssid", m_saved_wifi_ssid_subject);
	flx::system::SettingsManager::getInstance().registerSetting("wifi_pass", m_saved_wifi_password_subject);

#if FLXOS_WIFI_ENABLED
	ESP_ERROR_CHECK(setWifiMode(WIFI_MODE_NULL));
#endif
	Log::info(TAG, "Connectivity service started");

#if FLXOS_WIFI_ENABLED
	// ── Migration: move old flat wifi_ssid/wifi_pass into the credential store ──
	{
		std::string old_ssid = m_saved_wifi_ssid_subject.get();
		std::string old_pass = m_saved_wifi_password_subject.get();
		if (!old_ssid.empty() && !WiFiCredentialStore::getInstance().contains(old_ssid)) {
			Log::info(TAG, "Migrating legacy saved network '%s' into credential store", old_ssid.c_str());
			WiFiCredential cred;
			cred.ssid = old_ssid;
			cred.password = old_pass;
			cred.autoConnect = true;
			cred.priority = 0;
			esp_err_t const save_err = WiFiCredentialStore::getInstance().save(cred);
			if (save_err == ESP_OK) {
				// Only erase legacy keys once we know the store accepted them;
				// a transient storage failure must not destroy the user's credentials.
				m_saved_wifi_ssid_subject.set("");
				m_saved_wifi_password_subject.set("");
			} else {
				Log::warn(TAG, "Migration save failed (%d) — legacy keys preserved", save_err);
			}
		}
	}

	// SD card / boot-media provisioning
	bool const newly_provisioned = WiFiProvisioning::importFromBootMedia();

	// Restore previous state of Wi-Fi (wifi_enabled)
	if (newly_provisioned || m_wifi_enabled_subject.get() != 0) {
		Log::info(TAG, "Restoring Wi-Fi previous enabled state (or newly provisioned)...");
		setWiFiEnabled(true);
		WiFiManager::getInstance().connectBestKnownNetwork();
	} else {
		Log::info(TAG, "Restoring Wi-Fi previous disabled state...");
		setWiFiEnabled(false);
	}

	// Initialize scan timer
	esp_timer_create_args_t scan_timer_args = {};
	scan_timer_args.callback = [](void* arg) {
		auto* self = static_cast<ConnectivityManager*>(arg);
		if (self->isWiFiEnabled() && !self->isWiFiConnected() && !WiFiManager::getInstance().isConnected()) {
			Log::info(TAG, "Periodic scan timer fired. Auto-connecting to best known network.");
			self->connectBestKnownNetwork();
		}
	};
	scan_timer_args.arg = this;
	scan_timer_args.name = "wifi_periodic_scan";
	esp_timer_create(&scan_timer_args, &m_scan_timer);

	// Subscribe to changes to update timer dynamically
	m_wifi_enabled_subject.subscribe([this](int32_t) {
		updateScanTimer();
	});
	m_wifi_scan_interval_subject.subscribe([this](int32_t) {
		updateScanTimer();
	});

	updateScanTimer();

	// Start hotspot usage timer
	HotspotManager::getInstance().startUsageTimer();
#endif

	return true;
}

void ConnectivityManager::onStop() {
	Log::info(TAG, "Connectivity service stopping...");
#if FLXOS_WIFI_ENABLED
	// Disconnect WiFi if connected
	if (isWiFiConnected()) {
		WiFiManager::getInstance().disconnect();
	}
	// Stop hotspot if running
	if (isHotspotEnabled()) {
		HotspotManager::getInstance().stop();
	}
	if (m_scan_timer) {
		esp_timer_stop(m_scan_timer);
		esp_timer_delete(m_scan_timer);
		m_scan_timer = nullptr;
	}
#endif
	Log::info(TAG, "Connectivity service stopped");
}

esp_err_t ConnectivityManager::setWifiMode(wifi_mode_t mode, bool auto_start) {
#if FLXOS_WIFI_ENABLED
	std::lock_guard<std::recursive_mutex> lock(m_wifi_mutex);

	wifi_mode_t current_mode;
	esp_err_t err = esp_wifi_get_mode(&current_mode);
	if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
		return err;
	}

	if (err != ESP_ERR_WIFI_NOT_INIT && current_mode == mode) {
		if (auto_start) {
			return esp_wifi_start(); // Ensure it's started
		}
		return ESP_OK;
	}

	Log::info(TAG, "Setting WiFi mode: %d", (int)mode);
	err = esp_wifi_set_mode(mode);
	if (err != ESP_OK) {
		Log::error("Connectivity", "Failed to set WiFi mode: %d (0x%x)", (int)mode, err);
		return err;
	}

	if (auto_start) {
		return esp_wifi_start();
	}
	return ESP_OK;
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t ConnectivityManager::connectWiFi(const char* ssid, const char* password, bool remember) {
#if FLXOS_WIFI_ENABLED
	esp_err_t err = WiFiManager::getInstance().connect(ssid, password);
	if (err == ESP_OK && remember && ssid) {
		// Save to multi-network credential store
		WiFiCredential cred;
		cred.ssid = ssid;
		cred.password = password ? password : "";
		cred.autoConnect = true;
		WiFiCredentialStore::getInstance().save(cred);
		// Also update legacy flat strings for UI backward compat
		saveWiFiCredentials(ssid, password);
	}
	return err;
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t ConnectivityManager::disconnectWiFi() {
#if FLXOS_WIFI_ENABLED
	return WiFiManager::getInstance().disconnect();
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

bool ConnectivityManager::isWiFiConnected() {
#if FLXOS_WIFI_ENABLED
	return WiFiManager::getInstance().isConnected();
#else
	return false;
#endif
}

esp_err_t ConnectivityManager::scanWiFi(WiFiManager::ScanCallback callback) {
#if FLXOS_WIFI_ENABLED
	return WiFiManager::getInstance().scan(callback);
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t ConnectivityManager::setWiFiEnabled(bool enabled) {
#if FLXOS_WIFI_ENABLED
	Log::info(TAG, "WiFi enabled set to: %s", enabled ? "TRUE" : "FALSE");
	esp_err_t const err = WiFiManager::getInstance().setEnabled(enabled);
	if (err == ESP_OK) {
		m_wifi_enabled_subject.set(enabled ? 1 : 0);
	}
	return err;
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

bool ConnectivityManager::isWiFiEnabled() {
#if FLXOS_WIFI_ENABLED
	return WiFiManager::getInstance().isEnabled();
#else
	return false;
#endif
}

esp_err_t ConnectivityManager::startHotspot(const char* s, const char* p, int c, int m, bool h, wifi_auth_mode_t auth, int8_t tx) {
#if FLXOS_WIFI_ENABLED
	Log::info(TAG, "Starting Hotspot (SSID: %s)", s);
	return HotspotManager::getInstance().start(s, p, c, m, h, auth, tx);
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t ConnectivityManager::stopHotspot() {
#if FLXOS_WIFI_ENABLED
	Log::info(TAG, "Stopping Hotspot");
	return HotspotManager::getInstance().stop();
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

bool ConnectivityManager::isHotspotEnabled() {
#if FLXOS_WIFI_ENABLED
	return HotspotManager::getInstance().isEnabled();
#else
	return false;
#endif
}

std::vector<HotspotManager::ClientInfo>
ConnectivityManager::getHotspotClientsList() const {
#if FLXOS_WIFI_ENABLED
	return HotspotManager::getInstance().getConnectedClients();
#else
	return {};
#endif
}

esp_err_t ConnectivityManager::enableBluetooth(bool e) {
#if FLXOS_BLUETOOTH_ENABLED
	return BluetoothManager::getInstance().enable(e);
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

bool ConnectivityManager::isBluetoothEnabled() {
#if FLXOS_BLUETOOTH_ENABLED
	return BluetoothManager::getInstance().isEnabled();
#else
	return false;
#endif
}

void ConnectivityManager::saveWiFiCredentials(const char* ssid, const char* password) {
#if FLXOS_WIFI_ENABLED
	m_saved_wifi_ssid_subject.set(ssid ? ssid : "");
	m_saved_wifi_password_subject.set(password ? password : "");
	Log::info(TAG, "WiFi credentials saved for: %s", ssid);
#endif
}

void ConnectivityManager::clearSavedWiFiCredentials() {
#if FLXOS_WIFI_ENABLED
	m_saved_wifi_ssid_subject.set("");
	m_saved_wifi_password_subject.set("");
	Log::info(TAG, "Saved WiFi credentials cleared");
#endif
}

bool ConnectivityManager::hasSavedWiFiCredentials() const {
#if FLXOS_WIFI_ENABLED
	// Check credential store first (preferred), fall back to legacy flat string
	if (WiFiCredentialStore::getInstance().count() > 0) {
		return true;
	}
	std::string ssid = m_saved_wifi_ssid_subject.get();
	return !ssid.empty();
#else
	return false;
#endif
}

std::string ConnectivityManager::getSavedWiFiSsid() const {
#if FLXOS_WIFI_ENABLED
	// After migration the legacy subject is empty; serve from the store instead.
	const std::string legacy = m_saved_wifi_ssid_subject.get();
	if (!legacy.empty()) {
		return legacy;
	}
	const auto all = WiFiCredentialStore::getInstance().loadAll();
	for (const auto& cred: all) {
		if (cred.autoConnect) {
			return cred.ssid;
		}
	}
#endif
	return {};
}

std::string ConnectivityManager::getSavedWiFiPassword() const {
#if FLXOS_WIFI_ENABLED
	const std::string legacy_ssid = m_saved_wifi_ssid_subject.get();
	if (!legacy_ssid.empty()) {
		return m_saved_wifi_password_subject.get();
	}
	const auto all = WiFiCredentialStore::getInstance().loadAll();
	for (const auto& cred: all) {
		if (cred.autoConnect) {
			return cred.password;
		}
	}
#endif
	return {};
}

// ──── Multi-network credential store delegation ────

esp_err_t ConnectivityManager::saveWiFiNetwork(const WiFiCredential& cred) {
#if FLXOS_WIFI_ENABLED
	return WiFiCredentialStore::getInstance().save(cred);
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

bool ConnectivityManager::loadWiFiNetwork(const std::string& ssid, WiFiCredential& out) const {
#if FLXOS_WIFI_ENABLED
	return WiFiCredentialStore::getInstance().load(ssid, out);
#else
	return false;
#endif
}

esp_err_t ConnectivityManager::removeWiFiNetwork(const std::string& ssid) {
#if FLXOS_WIFI_ENABLED
	esp_err_t err = WiFiCredentialStore::getInstance().remove(ssid);
	// If this was also the legacy saved SSID, clear it too
	if (m_saved_wifi_ssid_subject.get() == ssid) {
		m_saved_wifi_ssid_subject.set("");
		m_saved_wifi_password_subject.set("");
	}
	return err;
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

std::vector<WiFiCredential> ConnectivityManager::getSavedNetworks() const {
#if FLXOS_WIFI_ENABLED
	return WiFiCredentialStore::getInstance().loadAll();
#else
	return {};
#endif
}

esp_err_t ConnectivityManager::connectBestKnownNetwork() {
#if FLXOS_WIFI_ENABLED
	return WiFiManager::getInstance().connectBestKnownNetwork();
#else
	return ESP_ERR_NOT_SUPPORTED;
#endif
}

void ConnectivityManager::updateScanTimer() {
#if FLXOS_WIFI_ENABLED
	if (m_scan_timer) {
		esp_timer_stop(m_scan_timer);
	}

	int32_t interval_sec = m_wifi_scan_interval_subject.get();
	bool wifi_enabled = (m_wifi_enabled_subject.get() != 0);

	if (wifi_enabled && interval_sec > 0) {
		Log::info(TAG, "Starting periodic auto-connect scan timer (interval: %d s)", (int)interval_sec);
		esp_timer_start_periodic(m_scan_timer, static_cast<uint64_t>(interval_sec) * 1000000ULL);
	} else {
		Log::info(TAG, "Periodic auto-connect scan timer disabled");
	}
#endif
}

} // namespace flx::connectivity
