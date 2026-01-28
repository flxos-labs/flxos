#include "TimeManager.hpp"
#include "esp_log.h"
#include "esp_sntp.h"

static const char* TAG = "TimeManager";

namespace System {

TimeManager& TimeManager::getInstance() {
	static TimeManager instance;
	return instance;
}

static void time_sync_notification_cb(struct timeval* tv) {
	ESP_LOGI(TAG, "Notification of a time synchronization event");
	TimeManager::getInstance().updateSyncStatus(true);

	time_t now = 0;
	struct tm timeinfo = {};
	time(&now);
	localtime_r(&now, &timeinfo);
	ESP_LOGI(TAG, "Current time: %s", asctime(&timeinfo));
}

void TimeManager::init() {
	if (m_is_init)
		return;

	ESP_LOGI(TAG, "Initializing SNTP");

	esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	esp_sntp_setservername(1, "time.google.com");
	esp_sntp_setservername(2, "time.cloudflare.com");
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
	esp_sntp_init();

	// Set default timezone to India (IST)
	setenv("TZ", "IST-5:30", 1);
	tzset();

	m_is_init = true;
}

void TimeManager::deinit() {
	if (!m_is_init)
		return;

	ESP_LOGI(TAG, "Deinitializing SNTP");
	esp_sntp_stop();
	m_is_init = false;
	m_is_synced = false;
}

void TimeManager::syncTime() {
	if (!m_is_init) {
		init();
	}

	// Check if we are already synced
	if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
		m_is_synced = true;
	}
}

bool TimeManager::waitForSync(uint32_t timeout_ms) {
	if (!m_is_init)
		init();

	uint32_t waited = 0;
	const uint32_t interval = 100;

	while (!m_is_synced && waited < timeout_ms) {
		if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
			m_is_synced = true;
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(interval));
		waited += interval;
	}

	return m_is_synced;
}

void TimeManager::updateSyncStatus(bool synced) { m_is_synced = synced; }

void TimeManager::setTimeZone(const char* tz) {
	setenv("TZ", tz, 1);
	tzset();
	ESP_LOGI(TAG, "Timezone set to %s", tz);
}

} // namespace System
