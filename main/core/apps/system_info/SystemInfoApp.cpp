#include "SystemInfoApp.hpp"
#include "core/services/systeminfo/SystemInfoService.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include <sstream>

static const char* TAG = "SystemInfoApp";
static constexpr uint32_t UPDATE_INTERVAL_MS = 1000;

namespace System {
namespace Apps {

SystemInfoApp::SystemInfoApp()
	: m_tabview(nullptr), m_uptime_label(nullptr), m_chip_label(nullptr), m_idf_label(nullptr), m_heap_label(nullptr), m_heap_bar(nullptr), m_psram_label(nullptr), m_wifi_status_label(nullptr), m_wifi_ssid_label(nullptr), m_wifi_ip_label(nullptr), m_wifi_mac_label(nullptr), m_wifi_rssi_label(nullptr), m_tasks_list(nullptr), m_last_update(0) {}

void SystemInfoApp::onStart() {
	ESP_LOGI(TAG, "SystemInfoApp started");
}

void SystemInfoApp::onResume() {
	ESP_LOGI(TAG, "SystemInfoApp resumed");
	updateInfo();
}

void SystemInfoApp::onPause() {
	ESP_LOGI(TAG, "SystemInfoApp paused");
}

void SystemInfoApp::onStop() {
	ESP_LOGI(TAG, "SystemInfoApp stopped");
	m_tabview = nullptr;
	m_uptime_label = nullptr;
	m_chip_label = nullptr;
	m_idf_label = nullptr;
	m_heap_label = nullptr;
	m_heap_bar = nullptr;
	m_psram_label = nullptr;
	m_wifi_status_label = nullptr;
	m_wifi_ssid_label = nullptr;
	m_wifi_ip_label = nullptr;
	m_wifi_mac_label = nullptr;
	m_wifi_rssi_label = nullptr;
	m_tasks_list = nullptr;
}

void SystemInfoApp::update() {
	if (isActive() && m_tabview) {
		uint32_t now = esp_timer_get_time() / 1000; // ms
		if (now - m_last_update >= UPDATE_INTERVAL_MS) {
			updateInfo();
			m_last_update = now;
		}
	}
}

void SystemInfoApp::createUI(void* parent) {
	lv_obj_t* container = (lv_obj_t*)parent;

	// Create tabview
	m_tabview = lv_tabview_create(container);
	lv_obj_set_size(m_tabview, lv_pct(100), lv_pct(100));
	lv_tabview_set_tab_bar_position(m_tabview, LV_DIR_TOP);
	lv_tabview_set_tab_bar_size(m_tabview, lv_dpx(40));

	// Create tabs
	lv_obj_t* tab_system = lv_tabview_add_tab(m_tabview, "System");
	lv_obj_t* tab_memory = lv_tabview_add_tab(m_tabview, "Memory");
	lv_obj_t* tab_network = lv_tabview_add_tab(m_tabview, "Network");
	lv_obj_t* tab_tasks = lv_tabview_add_tab(m_tabview, "Tasks");

	createSystemTab(tab_system);
	createMemoryTab(tab_memory);
	createNetworkTab(tab_network);
	createTasksTab(tab_tasks);

	updateInfo();
}

void SystemInfoApp::createSystemTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(10), 0);
	lv_obj_set_style_pad_row(tab, lv_dpx(8), 0);

	// Get initial system stats
	auto stats = Services::SystemInfoService::getInstance().getSystemStats();

	// FlxOS Version
	lv_obj_t* version_label = lv_label_create(tab);
	lv_label_set_text_fmt(version_label, "FlxOS Version: %s", stats.flxosVersion.c_str());
	lv_obj_set_style_text_font(version_label, &lv_font_montserrat_14, 0);

	// ESP-IDF Version
	m_idf_label = lv_label_create(tab);
	lv_label_set_text_fmt(m_idf_label, "ESP-IDF: %s", stats.idfVersion.c_str());

	// Chip Info
	m_chip_label = lv_label_create(tab);
	lv_label_set_text_fmt(m_chip_label, "Chip: %s\nCores: %d\nRevision: %d\nFeatures: %s", stats.chipModel.c_str(), stats.cores, stats.revision, stats.features.c_str());

	// Uptime
	m_uptime_label = lv_label_create(tab);
	lv_label_set_text(m_uptime_label, "Uptime: --:--:--");
}

void SystemInfoApp::createMemoryTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(10), 0);
	lv_obj_set_style_pad_row(tab, lv_dpx(8), 0);

	// Heap info label
	m_heap_label = lv_label_create(tab);
	lv_label_set_text(m_heap_label, "Heap: Loading...");

	// Heap usage bar
	lv_obj_t* bar_label = lv_label_create(tab);
	lv_label_set_text(bar_label, "Heap Usage:");

	m_heap_bar = lv_bar_create(tab);
	lv_obj_set_size(m_heap_bar, lv_pct(90), lv_dpx(20));
	lv_bar_set_range(m_heap_bar, 0, 100);
	lv_bar_set_value(m_heap_bar, 0, LV_ANIM_OFF);

	// PSRAM info
	m_psram_label = lv_label_create(tab);
	auto memStats = Services::SystemInfoService::getInstance().getMemoryStats();
	if (memStats.hasPsram) {
		lv_label_set_text_fmt(m_psram_label, "PSRAM: %s total", Services::SystemInfoService::formatBytes(memStats.totalPsram).c_str());
	} else {
		lv_label_set_text(m_psram_label, "PSRAM: Not available");
	}
}

void SystemInfoApp::createNetworkTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(10), 0);
	lv_obj_set_style_pad_row(tab, lv_dpx(8), 0);

	m_wifi_status_label = lv_label_create(tab);
	lv_label_set_text(m_wifi_status_label, "WiFi: Checking...");

	m_wifi_ssid_label = lv_label_create(tab);
	lv_label_set_text(m_wifi_ssid_label, "SSID: --");

	m_wifi_ip_label = lv_label_create(tab);
	lv_label_set_text(m_wifi_ip_label, "IP Address: --");

	m_wifi_mac_label = lv_label_create(tab);
	auto wifiStats = Services::SystemInfoService::getInstance().getWiFiStats();
	lv_label_set_text_fmt(m_wifi_mac_label, "MAC: %02X:%02X:%02X:%02X:%02X:%02X", wifiStats.mac[0], wifiStats.mac[1], wifiStats.mac[2], wifiStats.mac[3], wifiStats.mac[4], wifiStats.mac[5]);

	m_wifi_rssi_label = lv_label_create(tab);
	lv_label_set_text(m_wifi_rssi_label, "Signal: --");
}

void SystemInfoApp::createTasksTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(10), 0);

	lv_obj_t* header = lv_label_create(tab);
	lv_label_set_text(header, "FreeRTOS Tasks:");
	lv_obj_set_style_text_font(header, &lv_font_montserrat_14, 0);

	m_tasks_list = lv_label_create(tab);
	lv_label_set_text(m_tasks_list, "Loading tasks...");
	lv_label_set_long_mode(m_tasks_list, LV_LABEL_LONG_SCROLL);
	lv_obj_set_width(m_tasks_list, lv_pct(95));
}

void SystemInfoApp::updateInfo() {
	// Get system stats
	auto sysStats = Services::SystemInfoService::getInstance().getSystemStats();

	// Update uptime
	if (m_uptime_label) {
		int uptime_s = sysStats.uptimeSeconds;
		int h = uptime_s / 3600;
		int m = (uptime_s % 3600) / 60;
		int s = uptime_s % 60;
		lv_label_set_text_fmt(m_uptime_label, "Uptime: %02d:%02d:%02d", h, m, s);
	}

	// Update heap info
	if (m_heap_label && m_heap_bar) {
		auto memStats = Services::SystemInfoService::getInstance().getMemoryStats();

		lv_label_set_text_fmt(m_heap_label, "Total: %s\nUsed: %s\nFree: %s\nMin Free: %s", Services::SystemInfoService::formatBytes(memStats.totalHeap).c_str(), Services::SystemInfoService::formatBytes(memStats.usedHeap).c_str(), Services::SystemInfoService::formatBytes(memStats.freeHeap).c_str(), Services::SystemInfoService::formatBytes(memStats.minFreeHeap).c_str());

		lv_bar_set_value(m_heap_bar, memStats.usagePercent, LV_ANIM_ON);
	}

	// Update WiFi info
	if (m_wifi_status_label) {
		auto wifiStats = Services::SystemInfoService::getInstance().getWiFiStats();
		lv_label_set_text_fmt(m_wifi_status_label, "WiFi: %s", wifiStats.connected ? "Connected" : "Disconnected");

		if (wifiStats.connected && m_wifi_ssid_label && m_wifi_ip_label && m_wifi_rssi_label) {
			lv_label_set_text_fmt(m_wifi_ssid_label, "SSID: %s", wifiStats.ssid.c_str());
			lv_label_set_text_fmt(m_wifi_ip_label, "IP Address: %s", wifiStats.ipAddress.c_str());
			lv_label_set_text_fmt(m_wifi_rssi_label, "Signal: %d dBm (%s)", wifiStats.rssi, wifiStats.signalStrength.c_str());
		} else {
			if (m_wifi_ssid_label) lv_label_set_text(m_wifi_ssid_label, "SSID: --");
			if (m_wifi_ip_label) lv_label_set_text(m_wifi_ip_label, "IP Address: --");
			if (m_wifi_rssi_label) lv_label_set_text(m_wifi_rssi_label, "Signal: --");
		}
	}

	// Update tasks list
	if (m_tasks_list) {
		auto tasks = Services::SystemInfoService::getInstance().getTaskList(10);
		std::stringstream ss;
		ss << "Active Tasks: " << tasks.size() << "\n\n";

		for (const auto& task: tasks) {
			ss << task.name << " (Stack: " << task.stackHighWaterMark << ")\n";
		}

		lv_label_set_text(m_tasks_list, ss.str().c_str());
	}
}


} // namespace Apps
} // namespace System
