#include "flx/connectivity/ConnectivityManager.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "flx/connectivity/bluetooth/BluetoothManager.hpp"
#include "flx/connectivity/hotspot/HotspotManager.hpp"
#include "flx/connectivity/wifi/WiFiCredentialStore.hpp"
#include "flx/connectivity/wifi/WiFiManager.hpp"
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
	esp_netif_create_default_wifi_sta();
	esp_netif_create_default_wifi_ap();
	wifi_init_config_t const cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	// Initialize sub-managers with observable references
	WiFiManager::getInstance().init(&m_wifi_connected_subject, &m_wifi_ssid_subject, &m_wifi_ip_subject, &m_wifi_status_subject);
	HotspotManager::getInstance().init(&m_hotspot_enabled_subject, &m_hotspot_clients_subject);
	BluetoothManager::getInstance().init(&m_bluetooth_enabled_subject);

	// Register config settings for persistence
	flx::system::SettingsManager::getInstance().registerSetting("hs_ssid", m_hotspot_ssid_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_pass", m_hotspot_password_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_chan", m_hotspot_channel_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_max", m_hotspot_max_conn_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_hide", m_hotspot_hidden_subject);
	flx::system::SettingsManager::getInstance().registerSetting("hs_auth", m_hotspot_auth_subject);
	flx::system::SettingsManager::getInstance().registerSetting("wifi_autostart", m_wifi_autostart_subject);
	flx::system::SettingsManager::getInstance().registerSetting("wifi_scan_int", m_wifi_scan_interval_subject);
	flx::system::SettingsManager::getInstance().registerSetting("wifi_ssid", m_saved_wifi_ssid_subject);
	flx::system::SettingsManager::getInstance().registerSetting("wifi_pass", m_saved_wifi_password_subject);

	ESP_ERROR_CHECK(setWifiMode(WIFI_MODE_NULL));
	Log::info(TAG, "Connectivity service started");

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
			WiFiCredentialStore::getInstance().save(cred);
			// Clear legacy keys so they don't re-migrate on next boot
			m_saved_wifi_ssid_subject.set("");
			m_saved_wifi_password_subject.set("");
		}
	}

	// Auto-start WiFi and connect to best known network if enabled
	if (m_wifi_autostart_subject.get() != 0) {
		Log::info(TAG, "Auto-starting WiFi...");
		setWiFiEnabled(true);
		WiFiManager::getInstance().connectBestKnownNetwork();
	}

	// Start hotspot usage timer
	HotspotManager::getInstance().startUsageTimer();

	return true;
}

void ConnectivityManager::onStop() {
	Log::info(TAG, "Connectivity service stopping...");
	// Disconnect WiFi if connected
	if (isWiFiConnected()) {
		WiFiManager::getInstance().disconnect();
	}
	// Stop hotspot if running
	if (isHotspotEnabled()) {
		HotspotManager::getInstance().stop();
	}
	Log::info(TAG, "Connectivity service stopped");
}

esp_err_t ConnectivityManager::setWifiMode(wifi_mode_t mode, bool auto_start) {
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
}

esp_err_t ConnectivityManager::connectWiFi(const char* ssid, const char* password, bool remember) {
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
}
esp_err_t ConnectivityManager::disconnectWiFi() {
	return WiFiManager::getInstance().disconnect();
}
bool ConnectivityManager::isWiFiConnected() {
	return WiFiManager::getInstance().isConnected();
}
esp_err_t ConnectivityManager::scanWiFi(WiFiManager::ScanCallback callback) {
	return WiFiManager::getInstance().scan(callback);
}
esp_err_t ConnectivityManager::setWiFiEnabled(bool enabled) {
	Log::info(TAG, "WiFi enabled set to: %s", enabled ? "TRUE" : "FALSE");
	esp_err_t const err = WiFiManager::getInstance().setEnabled(enabled);
	if (err == ESP_OK) {
	}
	return err;
}
bool ConnectivityManager::isWiFiEnabled() {
	return WiFiManager::getInstance().isEnabled();
}
esp_err_t ConnectivityManager::startHotspot(const char* s, const char* p, int c, int m, bool h, wifi_auth_mode_t auth, int8_t tx) {
	Log::info(TAG, "Starting Hotspot (SSID: %s)", s);
	return HotspotManager::getInstance().start(s, p, c, m, h, auth, tx);
}
esp_err_t ConnectivityManager::stopHotspot() {
	Log::info(TAG, "Stopping Hotspot");
	return HotspotManager::getInstance().stop();
}
bool ConnectivityManager::isHotspotEnabled() {
	return HotspotManager::getInstance().isEnabled();
}
std::vector<HotspotManager::ClientInfo>
ConnectivityManager::getHotspotClientsList() const {
	return HotspotManager::getInstance().getConnectedClients();
}
esp_err_t ConnectivityManager::enableBluetooth(bool e) {
	return BluetoothManager::getInstance().enable(e);
}
bool ConnectivityManager::isBluetoothEnabled() {
	return BluetoothManager::getInstance().isEnabled();
}

void ConnectivityManager::saveWiFiCredentials(const char* ssid, const char* password) {
	m_saved_wifi_ssid_subject.set(ssid ? ssid : "");
	m_saved_wifi_password_subject.set(password ? password : "");
	Log::info(TAG, "WiFi credentials saved for: %s", ssid);
}

void ConnectivityManager::clearSavedWiFiCredentials() {
	m_saved_wifi_ssid_subject.set("");
	m_saved_wifi_password_subject.set("");
	Log::info(TAG, "Saved WiFi credentials cleared");
}

bool ConnectivityManager::hasSavedWiFiCredentials() const {
	// Check credential store first (preferred), fall back to legacy flat string
	if (WiFiCredentialStore::getInstance().count() > 0) {
		return true;
	}
	std::string ssid = m_saved_wifi_ssid_subject.get();
	return !ssid.empty();
}

// ──── Multi-network credential store delegation ────

esp_err_t ConnectivityManager::saveWiFiNetwork(const WiFiCredential& cred) {
	return WiFiCredentialStore::getInstance().save(cred);
}

bool ConnectivityManager::loadWiFiNetwork(const std::string& ssid, WiFiCredential& out) const {
	return WiFiCredentialStore::getInstance().load(ssid, out);
}

esp_err_t ConnectivityManager::removeWiFiNetwork(const std::string& ssid) {
	esp_err_t err = WiFiCredentialStore::getInstance().remove(ssid);
	// If this was also the legacy saved SSID, clear it too
	if (m_saved_wifi_ssid_subject.get() == ssid) {
		m_saved_wifi_ssid_subject.set("");
		m_saved_wifi_password_subject.set("");
	}
	return err;
}

std::vector<WiFiCredential> ConnectivityManager::getSavedNetworks() const {
	return WiFiCredentialStore::getInstance().loadAll();
}

esp_err_t ConnectivityManager::connectBestKnownNetwork() {
	return WiFiManager::getInstance().connectBestKnownNetwork();
}

} // namespace flx::connectivity
