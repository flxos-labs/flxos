#pragma once

#include "lvgl.h"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/system/services/SystemInfoService.hpp>
using namespace flx::apps;
#include <vector>

namespace System::Apps {

class SystemInfoApp : public flx::apps::App {
public:

	SystemInfoApp();
	~SystemInfoApp() override = default;

	bool onStart() override;
	bool onResume() override;
	void onPause() override;
	void createUI(void* parent) override;
	void onStop() override;
	void update() override;

	std::string getPackageName() const override { return "com.flxos.systeminfo"; }
	std::string getAppName() const override { return "System Info"; }
	const void* getIcon() const override { return LV_SYMBOL_TINT; }

	static const AppManifest manifest;

private:

	// UI Elements
	lv_obj_t* m_tabview {nullptr};

	// System tab labels
	lv_obj_t* m_uptime_label {nullptr};
	lv_obj_t* m_chip_label {nullptr};
	lv_obj_t* m_idf_label {nullptr};
	lv_obj_t* m_battery_label {nullptr};
	std::vector<lv_obj_t*> m_cpu_bars {};
	std::vector<lv_obj_t*> m_cpu_labels {};

	// Memory tab labels
	lv_obj_t* m_heap_label {nullptr};
	lv_obj_t* m_heap_bar {nullptr};
	lv_obj_t* m_psram_label {nullptr};
	lv_obj_t* m_psram_bar {nullptr};

	// Storage
	lv_obj_t* m_storage_system_label {nullptr};
	lv_obj_t* m_storage_system_bar {nullptr};
	lv_obj_t* m_storage_data_label {nullptr};
	lv_obj_t* m_storage_data_bar {nullptr};

	// Network tab labels
	lv_obj_t* m_wifi_status_label {nullptr};
	lv_obj_t* m_wifi_ssid_label {nullptr};
	lv_obj_t* m_wifi_ip_label {nullptr};
	lv_obj_t* m_wifi_mac_label {nullptr};
	lv_obj_t* m_wifi_rssi_label {nullptr};

	// Tasks tab
	lv_obj_t* m_tasks_table {nullptr};

	uint32_t m_last_update = 0;

	// Helper methods
	void updateInfo();
	void updateUptime(const flx::services::SystemStats& sysStats);
	void updateBattery(flx::services::SystemInfoService& service);
	void updateCpuUsage(const std::vector<flx::services::TaskInfo>& tasks, int coreCount);
	void updateHeap(flx::services::SystemInfoService& service);
	void updateStorage(flx::services::SystemInfoService& service);
	void updateWiFi(flx::services::SystemInfoService& service);
	void updateTaskList(std::vector<flx::services::TaskInfo>& tasks);
	void createSystemTab(lv_obj_t* tab);
	void createMemoryTab(lv_obj_t* tab);
	void createNetworkTab(lv_obj_t* tab);
	void createTasksTab(lv_obj_t* tab);
};

} // namespace System::Apps
