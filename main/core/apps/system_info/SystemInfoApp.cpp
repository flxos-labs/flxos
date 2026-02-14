#include "SystemInfoApp.hpp"
#include "../../ui/theming/layout_constants/LayoutConstants.hpp"
#include "../../ui/theming/ui_constants/UiConstants.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "core/services/device/DeviceProfileService.hpp"
#include "core/services/system_info/SystemInfoService.hpp"
#include "display/lv_display.h"
#include "esp_timer.h"
#include "font/lv_font.h"
#include "freertos/idf_additions.h"
#include "layouts/flex/lv_flex.h"
#include "misc/lv_anim.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_types.h"
#include "widgets/bar/lv_bar.h"
#include "widgets/label/lv_label.h"
#include "widgets/table/lv_table.h"
#include "widgets/tabview/lv_tabview.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <flx/core/Logger.hpp>
#include <string_view>

static constexpr std::string_view TAG = "SystemInfoApp";

static constexpr uint32_t UPDATE_INTERVAL_MS = 1000;

using namespace flx::apps;

namespace System::Apps {

const AppManifest SystemInfoApp::manifest = {
	.appId = "com.flxos.systeminfo",
	.appName = "System Info",
	.appIcon = LV_SYMBOL_TINT,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::SingleInstance,
	.location = AppLocation::internal(),
	.description = "System diagnostics and hardware information",
	.sortPriority = 30,
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<SystemInfoApp>(); }
};

SystemInfoApp::SystemInfoApp() {
	m_cpu_bars.clear();
	m_cpu_labels.clear();
}

bool SystemInfoApp::onStart() {
	Log::info(TAG, "App started");
	return true;
}

bool SystemInfoApp::onResume() {
	Log::debug(TAG, "App resumed, refreshing data");
	updateInfo();
	return true;
}

void SystemInfoApp::onPause() {
}

void SystemInfoApp::onStop() {
	m_tabview = nullptr;
	m_uptime_label = nullptr;
	m_chip_label = nullptr;
	m_idf_label = nullptr;
	m_battery_label = nullptr;
	m_heap_label = nullptr;
	m_heap_bar = nullptr;
	m_psram_label = nullptr;
	m_psram_bar = nullptr;
	m_storage_system_label = nullptr;
	m_storage_system_bar = nullptr;
	m_storage_data_label = nullptr;
	m_storage_data_bar = nullptr;
	m_wifi_status_label = nullptr;
	m_wifi_ssid_label = nullptr;
	m_wifi_ip_label = nullptr;
	m_wifi_mac_label = nullptr;
	m_wifi_rssi_label = nullptr;
	m_tasks_table = nullptr;
	m_cpu_bars.clear();
	m_cpu_labels.clear();
}

void SystemInfoApp::update() {
	if (isActive() && m_tabview) {
		uint32_t const now = esp_timer_get_time() / 1000; // ms
		if (now - m_last_update >= UPDATE_INTERVAL_MS) {
			updateInfo();
			m_last_update = now;
		}
	}
}

void SystemInfoApp::createUI(void* parent) {
	auto* container = (lv_obj_t*)parent;

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
	auto stats = flx::services::SystemInfoService::getInstance().getSystemStats();

	// FlxOS Version
	lv_obj_t* version_label = lv_label_create(tab);
	lv_label_set_text_fmt(version_label, "FlxOS: %s", stats.flxosVersion.c_str());

	// Build Date
	lv_obj_t* build_label = lv_label_create(tab);
	lv_label_set_text_fmt(build_label, "Build: %s", stats.buildDate.c_str());

	// Device Profile
	auto& profileService = flx::services::DeviceProfileService::getInstance();
	if (profileService.hasValidProfile()) {
		const auto& profile = profileService.getActiveProfile();

		lv_obj_t* separator = lv_obj_create(tab);
		lv_obj_set_size(separator, lv_pct(100), 1);
		lv_obj_set_style_bg_color(separator, lv_color_hex(0x888888), 0);
		lv_obj_set_style_bg_opa(separator, LV_OPA_50, 0);

		lv_obj_t* profile_header = lv_label_create(tab);
		lv_label_set_text(profile_header, "Device Profile");
		lv_obj_set_style_text_font(profile_header, &lv_font_montserrat_14, 0);

		lv_obj_t* board_label = lv_label_create(tab);
		lv_label_set_text_fmt(board_label, "Board: %s %s", profile.vendor.c_str(), profile.boardName.c_str());

		lv_obj_t* chip_info_label = lv_label_create(tab);
		lv_label_set_text_fmt(chip_info_label, "Target: %s", profile.chipTarget.c_str());

		lv_obj_t* display_info_label = lv_label_create(tab);
		lv_label_set_text_fmt(display_info_label, "Panel: %ux%u %s (%.1f\")", profile.display.width, profile.display.height, profile.display.driver.c_str(), profile.display.sizeInches);

		if (profile.touch.enabled) {
			lv_obj_t* touch_label = lv_label_create(tab);
			lv_label_set_text_fmt(touch_label, "Touch: %s via %s", profile.touch.driver.c_str(), profile.touch.bus.c_str());
		}

		// Feature flags
		std::string caps;
		if (profile.hasWifi()) caps += "WiFi ";
		if (profile.hasBluetooth()) caps += profile.connectivity.bleOnly ? "BLE " : "BT ";
		if (profile.hasPsram()) caps += "PSRAM ";
		if (profile.hasSdCard()) caps += "SD ";
		if (profile.hasBattery()) caps += "Battery ";
		if (profile.features.hasRgbLed) caps += "RGB ";

		lv_obj_t* caps_label = lv_label_create(tab);
		lv_label_set_text_fmt(caps_label, "Features: %s", caps.c_str());

		lv_obj_t* separator2 = lv_obj_create(tab);
		lv_obj_set_size(separator2, lv_pct(100), 1);
		lv_obj_set_style_bg_color(separator2, lv_color_hex(0x888888), 0);
		lv_obj_set_style_bg_opa(separator2, LV_OPA_50, 0);
	}

	// ESP-IDF Version
	m_idf_label = lv_label_create(tab);
	lv_label_set_text_fmt(m_idf_label, "ESP-IDF: %s", stats.idfVersion.c_str());

	// Boot Reason
	lv_obj_t* boot_label = lv_label_create(tab);
	lv_label_set_text_fmt(boot_label, "Boot: %s", stats.bootReason.c_str());

	// Chip Info
	m_chip_label = lv_label_create(tab);
	lv_label_set_text_fmt(m_chip_label, "Chip: %s (%d cores, rev %d)\nFreq: %lu MHz\nFeatures: %s", stats.chipModel.c_str(), stats.cores, stats.revision, stats.cpuFreqMhz, stats.features.c_str());

	// Display Info
	lv_obj_t* display_label = lv_label_create(tab);
	lv_label_set_text_fmt(display_label, "Display: %dx%d (%s)", stats.displayResX, stats.displayResY, stats.colorFormat.c_str());

	// CPU Load Header
	lv_obj_t* cpu_header = lv_label_create(tab);
	lv_label_set_text(cpu_header, "CPU Load:");
	lv_obj_set_style_margin_top(cpu_header, lv_dpx(10), 0);

	m_cpu_bars.clear();
	m_cpu_labels.clear();
	for (int i = 0; i < stats.cores; ++i) {
		lv_obj_t* row = lv_obj_create(tab);
		lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_pad_all(row, 0, 0);
		lv_obj_set_style_bg_opa(row, 0, 0);
		lv_obj_set_style_border_width(row, 0, 0);

		lv_obj_t* label = lv_label_create(row);
		lv_label_set_text_fmt(label, "Core %d: 0%%", i);
		lv_obj_set_width(label, lv_dpx(80));
		m_cpu_labels.push_back(label);

		lv_obj_t* bar = lv_bar_create(row);
		lv_obj_set_flex_grow(bar, 1);
		lv_obj_set_height(bar, lv_dpx(UiConstants::SIZE_BAR_HEIGHT));
		lv_bar_set_range(bar, 0, 100);
		m_cpu_bars.push_back(bar);
	}

	// Battery
	m_battery_label = lv_label_create(tab);
	lv_label_set_text(m_battery_label, "Battery: --");

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
	lv_obj_set_size(m_heap_bar, lv_pct(LayoutConstants::BAR_WIDTH_PCT), lv_dpx(UiConstants::SIZE_BAR_HEIGHT));
	lv_bar_set_range(m_heap_bar, 0, 100);
	lv_bar_set_value(m_heap_bar, 0, LV_ANIM_OFF);

	// PSRAM info
	m_psram_label = lv_label_create(tab);
	auto memStats = flx::services::SystemInfoService::getInstance().getMemoryStats();
	if (memStats.hasPsram) {
		lv_label_set_text_fmt(m_psram_label, "PSRAM: %s total", flx::services::SystemInfoService::formatBytes(memStats.totalPsram).c_str());

		lv_obj_t* psram_bar_label = lv_label_create(tab);
		lv_label_set_text(psram_bar_label, "PSRAM Usage:");

		m_psram_bar = lv_bar_create(tab);
		lv_obj_set_size(m_psram_bar, lv_pct(LayoutConstants::BAR_WIDTH_PCT), lv_dpx(UiConstants::SIZE_BAR_HEIGHT));
		lv_bar_set_range(m_psram_bar, 0, 100);
		lv_bar_set_value(m_psram_bar, 0, LV_ANIM_OFF);
	} else {
		lv_label_set_text(m_psram_label, "PSRAM: Not available");
		m_psram_bar = nullptr;
	}

	// Storage Section
	lv_obj_t* separator = lv_obj_create(tab);
	lv_obj_set_size(separator, lv_pct(100), 1);
	lv_obj_set_style_bg_color(separator, lv_color_hex(0x888888), 0);
	lv_obj_set_style_bg_opa(separator, LV_OPA_50, 0);

	lv_obj_t* storage_header = lv_label_create(tab);
	lv_label_set_text(storage_header, "Storage");
	lv_obj_set_style_text_font(storage_header, &lv_font_montserrat_14, 0); // Assuming font usage

	// System Partition
	m_storage_system_label = lv_label_create(tab);
	lv_label_set_text(m_storage_system_label, "System: --");
	m_storage_system_bar = lv_bar_create(tab);
	lv_obj_set_size(m_storage_system_bar, lv_pct(LayoutConstants::BAR_WIDTH_PCT), lv_dpx(UiConstants::SIZE_BAR_HEIGHT));
	lv_bar_set_range(m_storage_system_bar, 0, 100);

	// Data Partition
	m_storage_data_label = lv_label_create(tab);
	lv_label_set_text(m_storage_data_label, "Data: --");
	m_storage_data_bar = lv_bar_create(tab);
	lv_obj_set_size(m_storage_data_bar, lv_pct(LayoutConstants::BAR_WIDTH_PCT), lv_dpx(UiConstants::SIZE_BAR_HEIGHT));
	lv_bar_set_range(m_storage_data_bar, 0, 100);
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
	auto wifiStats = flx::services::SystemInfoService::getInstance().getWiFiStats();
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
	lv_table_set_column_count(m_tasks_table, 7);

	// Responsive column widths
	lv_obj_update_layout(tab);
	int32_t const screen_w = lv_obj_get_width(tab);
	// Use slightly less than full width to avoid potential scrollbar issues
	int32_t const w = screen_w - 5;

	lv_table_set_column_width(m_tasks_table, 0, (int32_t)(w * 0.08)); // #
	lv_table_set_column_width(m_tasks_table, 1, (int32_t)(w * 0.28)); // Name
	lv_table_set_column_width(m_tasks_table, 2, (int32_t)(w * 0.12)); // CPU%
	lv_table_set_column_width(m_tasks_table, 3, (int32_t)(w * 0.15)); // State
	lv_table_set_column_width(m_tasks_table, 4, (int32_t)(w * 0.12)); // Prio
	lv_table_set_column_width(m_tasks_table, 5, (int32_t)(w * 0.15)); // Stack
	lv_table_set_column_width(m_tasks_table, 6, (int32_t)(w * 0.10)); // Core

	lv_table_set_cell_value(m_tasks_table, 0, 0, "#");
	lv_table_set_cell_value(m_tasks_table, 0, 1, "Name");
	lv_table_set_cell_value(m_tasks_table, 0, 2, "CPU%");
	lv_table_set_cell_value(m_tasks_table, 0, 3, "State");
	lv_table_set_cell_value(m_tasks_table, 0, 4, "Prio");
	lv_table_set_cell_value(m_tasks_table, 0, 5, "Stack");
	lv_table_set_cell_value(m_tasks_table, 0, 6, "Core");
}

void SystemInfoApp::updateInfo() {
	Log::verbose(TAG, "Refreshing system stats...");
	// Get system stats
	auto& service = flx::services::SystemInfoService::getInstance(); // Use reference for convenience
	auto sysStats = service.getSystemStats();

	updateUptime(sysStats);
	updateBattery(service);

	// Get task list ONCE and reuse for CPU calculation and table display
	auto tasks = service.getTaskList();
	updateCpuUsage(tasks, sysStats.cores);

	updateHeap(service);
	updateStorage(service);
	updateWiFi(service);
	updateTaskList(tasks);
}

void SystemInfoApp::updateUptime(const flx::services::SystemStats& sysStats) {
	if (m_uptime_label) {
		int const uptime_s = sysStats.uptimeSeconds;
		int const h = uptime_s / 3600;
		int const m = (uptime_s % 3600) / 60;
		int const s = uptime_s % 60;
		lv_label_set_text_fmt(m_uptime_label, "Uptime: %02d:%02d:%02d", h, m, s);
	}
}

void SystemInfoApp::updateBattery(flx::services::SystemInfoService& service) {
	if (m_battery_label) {
		auto batStats = service.getBatteryStats();
		if (batStats.isConfigured) {
			lv_label_set_text_fmt(m_battery_label, "Battery: %d%% %s", batStats.level, batStats.isCharging ? "(Charging)" : "");
		} else {
			lv_label_set_text(m_battery_label, "Battery: Not configured");
		}
	}
}

void SystemInfoApp::updateCpuUsage(const std::vector<flx::services::TaskInfo>& tasks, int coreCount) {
	std::vector<float> core_usage(coreCount, 0.0f);
	for (const auto& task: tasks) {
		if (task.coreID >= 0 && task.coreID < (int)core_usage.size()) {
			// Don't count idle tasks as "usage" for the bar if we want it to represent load
			if (task.name.find("IDLE") == std::string::npos) {
				core_usage[task.coreID] += task.cpuUsagePercent;
			}
		} else if (task.coreID == -1 || task.coreID == (int)0x7fffffff /* tskNO_AFFINITY */) {
			// Distribute tasks with no affinity? For simplicity, ignore or split?
			// Let's just not count them for the total core-bar for now to avoid >100% confusion
		}
	}

	for (size_t i = 0; i < m_cpu_bars.size(); ++i) {
		int usage = (int)core_usage[i];
		usage = std::min(usage, 100);
		lv_bar_set_value(m_cpu_bars[i], usage, LV_ANIM_ON);
		lv_label_set_text_fmt(m_cpu_labels[i], "Core %d: %d%%", (int)i, usage);
	}
}

void SystemInfoApp::updateHeap(flx::services::SystemInfoService& service) {
	if (m_heap_label && m_heap_bar) {
		auto memStats = service.getMemoryStats();

		lv_label_set_text_fmt(m_heap_label, "Total: %s\nUsed: %s\nFree: %s\nMin Free: %s\nLargest Free Block: %s", service.formatBytes(memStats.totalHeap).c_str(), service.formatBytes(memStats.usedHeap).c_str(), service.formatBytes(memStats.freeHeap).c_str(), service.formatBytes(memStats.minFreeHeap).c_str(), service.formatBytes(memStats.largestFreeBlock).c_str());

		lv_bar_set_value(m_heap_bar, memStats.usagePercent, LV_ANIM_ON);

		if (memStats.hasPsram && m_psram_label && m_psram_bar) {
			lv_label_set_text_fmt(m_psram_label, "PSRAM Total: %s\nUsed: %s\nFree: %s", service.formatBytes(memStats.totalPsram).c_str(), service.formatBytes(memStats.usedPsram).c_str(), service.formatBytes(memStats.freePsram).c_str());
			lv_bar_set_value(m_psram_bar, memStats.usagePercentPsram, LV_ANIM_ON);
		}
	}
}

void SystemInfoApp::updateStorage(flx::services::SystemInfoService& service) {
	if (m_storage_system_label && m_storage_data_label) {
		auto storageStats = service.getStorageStats();
		for (const auto& stat: storageStats) {
			int usage = (stat.totalBytes > 0) ? (stat.usedBytes * 100 / stat.totalBytes) : 0;
			if (stat.name == "System") {
				lv_label_set_text_fmt(m_storage_system_label, "System: %s / %s", service.formatBytes(stat.usedBytes).c_str(), service.formatBytes(stat.totalBytes).c_str());
				lv_bar_set_value(m_storage_system_bar, usage, LV_ANIM_ON);
			} else if (stat.name == "Data") {
				lv_label_set_text_fmt(m_storage_data_label, "Data: %s / %s", service.formatBytes(stat.usedBytes).c_str(), service.formatBytes(stat.totalBytes).c_str());
				lv_bar_set_value(m_storage_data_bar, usage, LV_ANIM_ON);
			}
		}
	}
}

void SystemInfoApp::updateWiFi(flx::services::SystemInfoService& service) {
	if (m_wifi_status_label) {
		auto wifiStats = service.getWiFiStats();
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
}

void SystemInfoApp::updateTaskList(std::vector<flx::services::TaskInfo>& tasks) {
	if (m_tasks_table) {
		// Sort tasks by stack high water mark (descending) - sort a copy to not affect CPU calculation
		std::sort(tasks.begin(), tasks.end(), [](const flx::services::TaskInfo& a, const flx::services::TaskInfo& b) {
			return a.stackHighWaterMark > b.stackHighWaterMark;
		});

		lv_table_set_row_count(m_tasks_table, tasks.size() + 1);

		for (size_t i = 0; i < tasks.size(); ++i) {
			const auto& task = tasks[i];
			uint32_t row = i + 1;
			lv_table_set_cell_value_fmt(m_tasks_table, row, 0, "%d", (int)(i + 1));
			lv_table_set_cell_value(m_tasks_table, row, 1, task.name.c_str());
			lv_table_set_cell_value_fmt(m_tasks_table, row, 2, "%.1f%%", task.cpuUsagePercent);
			lv_table_set_cell_value(m_tasks_table, row, 3, task.state.c_str());
			lv_table_set_cell_value_fmt(m_tasks_table, row, 4, "%d", task.currentPriority);
			lv_table_set_cell_value_fmt(m_tasks_table, row, 5, "%u", (unsigned int)task.stackHighWaterMark);

			// Handle Core ID (check for tskNO_AFFINITY which is 2147483647)
			if (task.coreID == -1 || task.coreID == tskNO_AFFINITY) {
				lv_table_set_cell_value(m_tasks_table, row, 6, "Any");
			} else {
				lv_table_set_cell_value_fmt(m_tasks_table, row, 6, "%d", task.coreID);
			}
		}
	}
}

} // namespace System::Apps
