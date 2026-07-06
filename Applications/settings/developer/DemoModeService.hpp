#pragma once

#include "lvgl.h"
#include <flx/core/Observable.hpp>
#include <flx/system/managers/PowerManager.hpp>
#include <flx/connectivity/ConnectivityManager.hpp>
#include <flx/system/managers/NotificationManager.hpp>

namespace System::Apps::Settings {

class DemoModeService {
public:
	static inline flx::Observable<int32_t> demoMode {0};
	static inline flx::Observable<int32_t> simulateBattery {1};
	static inline flx::Observable<int32_t> simulateWifi {1};
	static inline flx::Observable<int32_t> simulateBluetooth {1};
	static inline flx::Observable<int32_t> simulateHotspot {1};
	static inline flx::Observable<int32_t> simulateNotifications {1};
	static inline flx::Observable<int32_t> demoIntervalMs {1500};

	static void init() {
		static bool initialized = false;
		if (initialized) return;
		initialized = true;

		demoIntervalMs.subscribe([](const int32_t& ms) {
			if (s_active && s_timer) {
				lv_timer_set_period(s_timer, ms);
			}
		});

		simulateBattery.subscribe([](const int32_t& val) {
			updateSimulateBattery(val != 0);
		});
		simulateWifi.subscribe([](const int32_t& val) {
			updateSimulateWifi(val != 0);
		});
		simulateBluetooth.subscribe([](const int32_t& val) {
			updateSimulateBluetooth(val != 0);
		});
		simulateHotspot.subscribe([](const int32_t& val) {
			updateSimulateHotspot(val != 0);
		});
		simulateNotifications.subscribe([](const int32_t& val) {
			updateSimulateNotifications(val != 0);
		});

		demoMode.subscribe([](const int32_t& val) {
			if (val != 0) {
				start();
			} else {
				stop();
			}
		});
	}

	static void start() {
		if (s_active) return;
		s_active = true;

		auto& pm = flx::system::PowerManager::getInstance();
		auto& cm = flx::connectivity::ConnectivityManager::getInstance();
		auto& nm = flx::system::NotificationManager::getInstance();

		s_origBatteryLevel = pm.getBatteryLevelObservable().get();
		s_origIsCharging = pm.getIsChargingObservable().get();
		s_origIsConfigured = pm.getIsConfiguredObservable().get();
		s_origWifiConnected = cm.getWiFiConnectedObservable().get();
		s_origWifiEnabled = cm.getWiFiEnabledObservable().get();
		s_origBluetoothEnabled = cm.getBluetoothEnabledObservable().get();
		s_origHotspotEnabled = cm.getHotspotEnabledObservable().get();
		s_origHotspotClients = cm.getHotspotClientsObservable().get();
		s_origUnreadCount = nm.getUnreadCountObservable().get();

		s_step = 0;
		runStep();

		s_timer = lv_timer_create(
			[](lv_timer_t* /*t*/) {
				runStep();
			},
			demoIntervalMs.get(), nullptr);
	}

	static void stop() {
		if (!s_active) return;
		s_active = false;

		if (s_timer) {
			lv_timer_delete(s_timer);
			s_timer = nullptr;
		}

		auto& pm = flx::system::PowerManager::getInstance();
		auto& cm = flx::connectivity::ConnectivityManager::getInstance();
		auto& nm = flx::system::NotificationManager::getInstance();

		pm.getBatteryLevelObservable().set(s_origBatteryLevel);
		pm.getIsChargingObservable().set(s_origIsCharging);
		pm.getIsConfiguredObservable().set(s_origIsConfigured);
		cm.getWiFiConnectedObservable().set(s_origWifiConnected);
		cm.getWiFiEnabledObservable().set(s_origWifiEnabled);
		cm.getBluetoothEnabledObservable().set(s_origBluetoothEnabled);
		cm.getHotspotEnabledObservable().set(s_origHotspotEnabled);
		cm.getHotspotClientsObservable().set(s_origHotspotClients);
		nm.getUnreadCountObservable().set(s_origUnreadCount);
	}

	static bool isActive() {
		return s_active;
	}

private:
	static void updateSimulateBattery(bool enabled) {
		if (!s_active) return;
		auto& pm = flx::system::PowerManager::getInstance();
		if (enabled) {
			s_origBatteryLevel = pm.getBatteryLevelObservable().get();
			s_origIsCharging = pm.getIsChargingObservable().get();
			s_origIsConfigured = pm.getIsConfiguredObservable().get();
		} else {
			pm.getBatteryLevelObservable().set(s_origBatteryLevel);
			pm.getIsChargingObservable().set(s_origIsCharging);
			pm.getIsConfiguredObservable().set(s_origIsConfigured);
		}
	}

	static void updateSimulateWifi(bool enabled) {
		if (!s_active) return;
		auto& cm = flx::connectivity::ConnectivityManager::getInstance();
		if (enabled) {
			s_origWifiConnected = cm.getWiFiConnectedObservable().get();
			s_origWifiEnabled = cm.getWiFiEnabledObservable().get();
		} else {
			cm.getWiFiConnectedObservable().set(s_origWifiConnected);
			cm.getWiFiEnabledObservable().set(s_origWifiEnabled);
		}
	}

	static void updateSimulateBluetooth(bool enabled) {
		if (!s_active) return;
		auto& cm = flx::connectivity::ConnectivityManager::getInstance();
		if (enabled) {
			s_origBluetoothEnabled = cm.getBluetoothEnabledObservable().get();
		} else {
			cm.getBluetoothEnabledObservable().set(s_origBluetoothEnabled);
		}
	}

	static void updateSimulateHotspot(bool enabled) {
		if (!s_active) return;
		auto& cm = flx::connectivity::ConnectivityManager::getInstance();
		if (enabled) {
			s_origHotspotEnabled = cm.getHotspotEnabledObservable().get();
			s_origHotspotClients = cm.getHotspotClientsObservable().get();
		} else {
			cm.getHotspotEnabledObservable().set(s_origHotspotEnabled);
			cm.getHotspotClientsObservable().set(s_origHotspotClients);
		}
	}

	static void updateSimulateNotifications(bool enabled) {
		if (!s_active) return;
		auto& nm = flx::system::NotificationManager::getInstance();
		if (enabled) {
			s_origUnreadCount = nm.getUnreadCountObservable().get();
		} else {
			nm.getUnreadCountObservable().set(s_origUnreadCount);
		}
	}

	static void runStep() {
		auto& pm = flx::system::PowerManager::getInstance();
		auto& cm = flx::connectivity::ConnectivityManager::getInstance();
		auto& nm = flx::system::NotificationManager::getInstance();

		bool const simBattery = simulateBattery.get();
		bool const simWifi = simulateWifi.get();
		bool const simBluetooth = simulateBluetooth.get();
		bool const simHotspot = simulateHotspot.get();
		bool const simNotifications = simulateNotifications.get();

		if (simBattery) {
			pm.getIsConfiguredObservable().set(1);
		}

		switch (s_step) {
			case 0:
				if (simBattery) {
					pm.getBatteryLevelObservable().set(100);
					pm.getIsChargingObservable().set(0);
				}
				if (simWifi) {
					cm.getWiFiEnabledObservable().set(1);
					cm.getWiFiConnectedObservable().set(1);
				}
				if (simBluetooth) {
					cm.getBluetoothEnabledObservable().set(1);
				}
				if (simHotspot) {
					cm.getHotspotEnabledObservable().set(0);
					cm.getHotspotClientsObservable().set(0);
				}
				if (simNotifications) {
					nm.getUnreadCountObservable().set(0);
				}
				break;
			case 1:
				if (simBattery) {
					pm.getBatteryLevelObservable().set(55);
					pm.getIsChargingObservable().set(0);
				}
				if (simWifi) {
					cm.getWiFiEnabledObservable().set(1);
					cm.getWiFiConnectedObservable().set(1);
				}
				if (simBluetooth) {
					cm.getBluetoothEnabledObservable().set(1);
				}
				if (simHotspot) {
					cm.getHotspotEnabledObservable().set(0);
					cm.getHotspotClientsObservable().set(0);
				}
				if (simNotifications) {
					nm.getUnreadCountObservable().set(0);
				}
				break;
			case 2:
				if (simBattery) {
					pm.getBatteryLevelObservable().set(18);
					pm.getIsChargingObservable().set(0);
				}
				if (simWifi) {
					cm.getWiFiEnabledObservable().set(1);
					cm.getWiFiConnectedObservable().set(0);
				}
				if (simBluetooth) {
					cm.getBluetoothEnabledObservable().set(1);
				}
				if (simHotspot) {
					cm.getHotspotEnabledObservable().set(0);
					cm.getHotspotClientsObservable().set(0);
				}
				if (simNotifications) {
					nm.getUnreadCountObservable().set(2);
				}
				break;
			case 3:
				if (simBattery) {
					pm.getBatteryLevelObservable().set(8);
					pm.getIsChargingObservable().set(0);
				}
				if (simWifi) {
					cm.getWiFiEnabledObservable().set(0);
					cm.getWiFiConnectedObservable().set(0);
				}
				if (simBluetooth) {
					cm.getBluetoothEnabledObservable().set(0);
				}
				if (simHotspot) {
					cm.getHotspotEnabledObservable().set(0);
					cm.getHotspotClientsObservable().set(0);
				}
				if (simNotifications) {
					nm.getUnreadCountObservable().set(5);
				}
				break;
			case 4:
				if (simBattery) {
					pm.getBatteryLevelObservable().set(25);
					pm.getIsChargingObservable().set(1);
				}
				if (simWifi) {
					cm.getWiFiEnabledObservable().set(1);
					cm.getWiFiConnectedObservable().set(1);
				}
				if (simBluetooth) {
					cm.getBluetoothEnabledObservable().set(1);
				}
				if (simHotspot) {
					cm.getHotspotEnabledObservable().set(0);
					cm.getHotspotClientsObservable().set(0);
				}
				if (simNotifications) {
					nm.getUnreadCountObservable().set(0);
				}
				break;
			case 5:
				if (simBattery) {
					pm.getBatteryLevelObservable().set(45);
					pm.getIsChargingObservable().set(0);
				}
				if (simWifi) {
					cm.getWiFiEnabledObservable().set(1);
					cm.getWiFiConnectedObservable().set(0);
				}
				if (simBluetooth) {
					cm.getBluetoothEnabledObservable().set(1);
				}
				if (simHotspot) {
					cm.getHotspotEnabledObservable().set(1);
					cm.getHotspotClientsObservable().set(1);
				}
				if (simNotifications) {
					nm.getUnreadCountObservable().set(1);
				}
				break;
		}

		s_step = (s_step + 1) % 6;
	}

	static inline bool s_active = false;
	static inline int s_step = 0;
	static inline lv_timer_t* s_timer = nullptr;

	static inline int32_t s_origBatteryLevel = 100;
	static inline int32_t s_origIsCharging = 0;
	static inline int32_t s_origIsConfigured = 0;
	static inline int32_t s_origWifiConnected = 0;
	static inline int32_t s_origWifiEnabled = 0;
	static inline int32_t s_origBluetoothEnabled = 0;
	static inline int32_t s_origHotspotEnabled = 0;
	static inline int32_t s_origHotspotClients = 0;
	static inline int32_t s_origUnreadCount = 0;
};

} // namespace System::Apps::Settings
