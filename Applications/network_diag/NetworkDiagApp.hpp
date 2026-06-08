#pragma once

#include "lvgl.h"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/connectivity/wifi/WiFiCredentialStore.hpp>
#include <mutex>
#include <string>
#include <vector>

namespace System::Apps {

using flx::apps::AppManifest;

struct ConnectionEvent {
	std::string ssid;
	std::string eventType; // "CONNECTED", "DISCONNECTED", "CONNECTING", "AUTH_FAILED", "NOT_FOUND"
	std::string timestamp;
};

class NetworkDiagApp : public flx::apps::App {
public:

	NetworkDiagApp();
	~NetworkDiagApp() override = default;

	bool onStart() override;
	bool onResume() override;
	void onPause() override;
	void createUI(void* parent) override;
	void onStop() override;
	void update() override;

	std::string getPackageName() const override { return "com.flxos.networkdiag"; }
	std::string getAppName() const override { return "Network Diag"; }
	const void* getIcon() const override { return LV_SYMBOL_WIFI; }

	static const AppManifest manifest;

	// Callbacks for events
	static void addHistoryEvent(const std::string& ssid, const std::string& type);

private:

	// UI Tabs
	lv_obj_t* m_tabview {nullptr};

	// Status Tab UI
	lv_obj_t* m_ssid_label {nullptr};
	lv_obj_t* m_ip_label {nullptr};
	lv_obj_t* m_rssi_label {nullptr};
	lv_obj_t* m_rssi_bar {nullptr};
	lv_obj_t* m_gateway_label {nullptr};
	lv_obj_t* m_dns_label {nullptr};
	lv_obj_t* m_mac_label {nullptr};
	lv_obj_t* m_channel_label {nullptr};

	// Saved Tab UI
	lv_obj_t* m_saved_list {nullptr};

	// Channels Tab UI
	lv_obj_t* m_channels_table {nullptr};

	// Tools Tab UI
	lv_obj_t* m_target_ta {nullptr}; // Text area for IP/host
	lv_obj_t* m_ping_btn {nullptr};
	lv_obj_t* m_dns_btn {nullptr};
	lv_obj_t* m_results_ta {nullptr}; // Results display text area

	// History Tab UI
	lv_obj_t* m_history_table {nullptr};

	// State
	uint32_t m_last_update = 0;
	bool m_ping_running = false;
	bool m_dns_running = false;
	std::string m_ping_target;
	std::string m_dns_target;
	std::vector<std::string> m_pending_results;
	std::mutex m_results_mutex;

	static std::vector<ConnectionEvent> s_history;
	static std::mutex s_history_mutex;

	// Helper UI builders
	void createStatusTab(lv_obj_t* tab);
	void createSavedTab(lv_obj_t* tab);
	void createChannelsTab(lv_obj_t* tab);
	void createToolsTab(lv_obj_t* tab);
	void createHistoryTab(lv_obj_t* tab);

	void updateStatusTab();
	void updateSavedTab();
	void updateChannelsTab();
	void updateHistoryTab();

	void logResult(const std::string& line);

	// Background Tasks
	static void pingTask(void* pvParameters);
	static void dnsTask(void* pvParameters);
};

} // namespace System::Apps
