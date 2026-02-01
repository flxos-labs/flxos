#include "SystemInfoApp.hpp"
#include "../../ui/theming/UiConstants/UiConstants.hpp"
#include "core/common/Logger.hpp"
#include "core/services/systeminfo/SystemInfoService.hpp"
#include "esp_timer.h"
#include <string_view>

static constexpr std::string_view TAG = "SystemInfoApp";

static constexpr uint32_t UPDATE_INTERVAL_MS = 1000;

namespace System {
namespace Apps {

SystemInfoApp::SystemInfoApp()
	: m_tabview(nullptr), m_uptime_label(nullptr), m_chip_label(nullptr), m_idf_label(nullptr), m_heap_label(nullptr), m_heap_bar(nullptr), m_psram_label(nullptr), m_psram_bar(nullptr), m_wifi_status_label(nullptr), m_wifi_ssid_label(nullptr), m_wifi_ip_label(nullptr), m_wifi_mac_label(nullptr), m_wifi_rssi_label(nullptr), m_tasks_table(nullptr), m_last_update(0) {}

void SystemInfoApp::onStart() {
	Log::info(TAG, "App started");
}

void SystemInfoApp::onResume() {
	Log::debug(TAG, "App resumed, refreshing data");
	updateInfo();
}

void SystemInfoApp::onPause() {
}

void SystemInfoApp::onStop() {
	m_tabview = nullptr;
	m_uptime_label = nullptr;
	m_chip_label = nullptr;
	m_idf_label = nullptr;
	m_heap_label = nullptr;
	m_heap_bar = nullptr;
	m_psram_label = nullptr;
	m_psram_bar = nullptr;
	m_wifi_status_label = nullptr;
	m_wifi_ssid_label = nullptr;
	m_wifi_ip_label = nullptr;
	m_wifi_mac_label = nullptr;
	m_wifi_rssi_label = nullptr;
	m_tasks_table = nullptr;
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
	lv_tabview_set_tab_bar_size(m_tabview, lv_dpx(UiConstants::SIZE_TAB_BAR));

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
	lv_obj_set_style_pad_all(tab, lv_dpx(UiConstants::PAD_LARGE), 0);
	lv_obj_set_style_pad_row(tab, lv_dpx(UiConstants::PAD_DEFAULT), 0);

	// Get initial system stats
	auto stats = Services::SystemInfoService::getInstance().getSystemStats();

	// FlxOS Version
	lv_obj_t* version_label = lv_label_create(tab);
	lv_label_set_text_fmt(version_label, "FlxOS Version: %s", stats.flxosVersion.c_str());

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
	lv_obj_set_style_pad_all(tab, lv_dpx(UiConstants::PAD_LARGE), 0);
	lv_obj_set_style_pad_row(tab, lv_dpx(UiConstants::PAD_DEFAULT), 0);

	// Heap info label
	m_heap_label = lv_label_create(tab);
	lv_label_set_text(m_heap_label, "Heap: Loading...");

	// Heap usage bar
	lv_obj_t* bar_label = lv_label_create(tab);
	lv_label_set_text(bar_label, "Heap Usage:");

	m_heap_bar = lv_bar_create(tab);
	lv_obj_set_size(m_heap_bar, lv_pct(90), lv_dpx(UiConstants::SIZE_BAR_HEIGHT));
	lv_bar_set_range(m_heap_bar, 0, 100);
	lv_bar_set_value(m_heap_bar, 0, LV_ANIM_OFF);

	// PSRAM info
	m_psram_label = lv_label_create(tab);
	auto memStats = Services::SystemInfoService::getInstance().getMemoryStats();
	if (memStats.hasPsram) {
		lv_label_set_text_fmt(m_psram_label, "PSRAM: %s total", Services::SystemInfoService::formatBytes(memStats.totalPsram).c_str());

		lv_obj_t* psram_bar_label = lv_label_create(tab);
		lv_label_set_text(psram_bar_label, "PSRAM Usage:");

		m_psram_bar = lv_bar_create(tab);
		lv_obj_set_size(m_psram_bar, lv_pct(90), lv_dpx(UiConstants::SIZE_BAR_HEIGHT));
		lv_bar_set_range(m_psram_bar, 0, 100);
		lv_bar_set_value(m_psram_bar, 0, LV_ANIM_OFF);
	} else {
		lv_label_set_text(m_psram_label, "PSRAM: Not available");
		m_psram_bar = nullptr;
	}
}

void SystemInfoApp::createNetworkTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(UiConstants::PAD_LARGE), 0);
	lv_obj_set_style_pad_row(tab, lv_dpx(UiConstants::PAD_DEFAULT), 0);

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
	lv_obj_set_style_pad_all(tab, 0, 0);

	m_tasks_table = lv_table_create(tab);
	lv_obj_set_style_pad_all(m_tasks_table, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(m_tasks_table, 0, LV_PART_ITEMS);
	lv_obj_set_width(m_tasks_table, lv_pct(100));
	lv_table_set_column_count(m_tasks_table, 5);

	// Responsive column widths
	lv_obj_update_layout(tab);
	int32_t screen_w = lv_obj_get_width(tab);
	// Use slightly less than full width to avoid potential scrollbar issues
	int32_t w = screen_w - 5;

	lv_table_set_column_width(m_tasks_table, 0, w * 0.32); // Name
	lv_table_set_column_width(m_tasks_table, 1, w * 0.22); // State
	lv_table_set_column_width(m_tasks_table, 2, w * 0.13); // Prio
	lv_table_set_column_width(m_tasks_table, 3, w * 0.20); // Stack
	lv_table_set_column_width(m_tasks_table, 4, w * 0.13); // Core

	lv_table_set_cell_value(m_tasks_table, 0, 0, "Name");
	lv_table_set_cell_value(m_tasks_table, 0, 1, "State");
	lv_table_set_cell_value(m_tasks_table, 0, 2, "Prio");
	lv_table_set_cell_value(m_tasks_table, 0, 3, "Stack");
	lv_table_set_cell_value(m_tasks_table, 0, 4, "Core");
}

void SystemInfoApp::updateInfo() {
	Log::verbose(TAG, "Refreshing system stats...");
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

		if (memStats.hasPsram && m_psram_label && m_psram_bar) {
			lv_label_set_text_fmt(m_psram_label, "PSRAM Total: %s\nUsed: %s\nFree: %s", Services::SystemInfoService::formatBytes(memStats.totalPsram).c_str(), Services::SystemInfoService::formatBytes(memStats.usedPsram).c_str(), Services::SystemInfoService::formatBytes(memStats.freePsram).c_str());
			lv_bar_set_value(m_psram_bar, memStats.usagePercentPsram, LV_ANIM_ON);
		}
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

	// Update tasks table
	if (m_tasks_table) {
		auto tasks = Services::SystemInfoService::getInstance().getTaskList();
		lv_table_set_row_count(m_tasks_table, tasks.size() + 1);

		for (size_t i = 0; i < tasks.size(); ++i) {
			const auto& task = tasks[i];
			uint32_t row = i + 1;
			lv_table_set_cell_value(m_tasks_table, row, 0, task.name.c_str());
			lv_table_set_cell_value(m_tasks_table, row, 1, task.state.c_str());
			lv_table_set_cell_value_fmt(m_tasks_table, row, 2, "%d", task.currentPriority);
			lv_table_set_cell_value_fmt(m_tasks_table, row, 3, "%u", (unsigned int)task.stackHighWaterMark);
			lv_table_set_cell_value_fmt(m_tasks_table, row, 4, "%d", (task.coreID == -1) ? -1 : task.coreID);
		}
	}
}

} // namespace Apps
} // namespace System
