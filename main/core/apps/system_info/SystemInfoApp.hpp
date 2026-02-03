#pragma once

#include "core/apps/AppManager.hpp"
#include "core/services/system_info/SystemInfoService.hpp"
#include "lvgl.h"
#include <vector>

namespace System {
namespace Apps {

class SystemInfoApp : public App {
public:

	SystemInfoApp();
	virtual ~SystemInfoApp() = default;

	void onStart() override;
	void onResume() override;
	void onPause() override;
	void createUI(void* parent) override;
	void onStop() override;
	void update() override;

	std::string getPackageName() const override { return "com.flxos.systeminfo"; }
	std::string getAppName() const override { return "System Info"; }
	const void* getIcon() const override { return LV_SYMBOL_TINT; }

private:

	// UI Elements
	lv_obj_t* m_tabview;

	// System tab labels
	lv_obj_t* m_uptime_label;
	lv_obj_t* m_chip_label;
	lv_obj_t* m_idf_label;
	lv_obj_t* m_battery_label;
	std::vector<lv_obj_t*> m_cpu_bars;
	std::vector<lv_obj_t*> m_cpu_labels;

	// Memory tab labels
	lv_obj_t* m_heap_label;
	lv_obj_t* m_heap_bar;
	lv_obj_t* m_psram_label;
	lv_obj_t* m_psram_bar;

	// Storage
	lv_obj_t* m_storage_system_label;
	lv_obj_t* m_storage_system_bar;
	lv_obj_t* m_storage_data_label;
	lv_obj_t* m_storage_data_bar;

	// Network tab labels
	lv_obj_t* m_wifi_status_label;
	lv_obj_t* m_wifi_ssid_label;
	lv_obj_t* m_wifi_ip_label;
	lv_obj_t* m_wifi_mac_label;
	lv_obj_t* m_wifi_rssi_label;

	// Tasks tab
	lv_obj_t* m_tasks_table;

	uint32_t m_last_update = 0;

	// Helper methods
	void updateInfo();
	void createSystemTab(lv_obj_t* tab);
	void createMemoryTab(lv_obj_t* tab);
	void createNetworkTab(lv_obj_t* tab);
	void createTasksTab(lv_obj_t* tab);
};

} // namespace Apps
} // namespace System
