#include "TimeManager.hpp"
#include "core/common/Logger.hpp"
#include "esp_sntp.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <cstdint>
#include <string_view>

static constexpr std::string_view TAG = "TimeManager";

namespace System {

TimeManager& TimeManager::getInstance() {
	static TimeManager instance;
	return instance;
}

static void time_sync_notification_cb(struct timeval* /*tv*/) {
	TimeManager::getInstance().updateSyncStatus(true);

	time_t now = 0;
	struct tm timeinfo = {};
	time(&now);
	localtime_r(&now, &timeinfo);
}

void TimeManager::init() {
	if (m_is_init) {
		return;
	}

	// setCompileTime(); // Moved to after tzset to correct offset

	esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	esp_sntp_setservername(1, "time.google.com");
	esp_sntp_setservername(2, "time.cloudflare.com");
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
	Log::info(TAG, "Initializing SNTP...");
	esp_sntp_init();

	// Set default timezone to India (IST)
	setenv("TZ", "IST-5:30", 1);
	tzset();

	// Set time to compile time if current time is invalid (older than build time)
	// This must be called AFTER tzset so mktime knows the local time offset
	setCompileTime();

	m_is_init = true;
}

void TimeManager::deinit() {
	if (!m_is_init) {
		return;
	}

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
	if (!m_is_init) {
		init();
	}

	uint32_t waited = 0;
	const uint32_t INTERVAL = 100;

	while (!m_is_synced && waited < timeout_ms) {
		if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
			m_is_synced = true;
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(INTERVAL));
		waited += INTERVAL;
	}

	if (!m_is_synced) {
	}

	return m_is_synced;
}

void TimeManager::updateSyncStatus(bool synced) {
	if (synced != m_is_synced) {
		Log::info(TAG, "Time sync status changed: %s", synced ? "SYNCED" : "NOT SYNCED");
	}
	m_is_synced = synced;
}

void TimeManager::setTimeZone(const char* tz) {
	setenv("TZ", tz, 1);
	tzset();
}

void TimeManager::setCompileTime() {
	char s_month[5];
	int day, year, hour, minute, second;
	struct tm t = {0};
	static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

	sscanf(__DATE__, "%s %d %d", s_month, &day, &year);
	sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

	// Calculate month index (0-11)
	const char* month_ptr = strstr(month_names, s_month);
	if (month_ptr) {
		t.tm_mon = (month_ptr - month_names) / 3;
	} else {
		t.tm_mon = 0; // Default to Jan if parsing fails
	}

	t.tm_mday = day;
	t.tm_year = year - 1900;
	t.tm_hour = hour;
	t.tm_min = minute;
	t.tm_sec = second;
	t.tm_isdst = -1;

	time_t buildTime = mktime(&t);
	time_t now;
	time(&now);

	if (now < buildTime) {
		struct timeval tv = {.tv_sec = buildTime, .tv_usec = 0};
		settimeofday(&tv, NULL);
		// Log the time we just set to verify
		char buf[64];
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
		Log::info(TAG, "System time was invalid. Set to compile time: %s", buf);
	}
}

} // namespace System
