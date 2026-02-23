#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>

namespace {
constexpr const char* TAG = "hwd:lilygo-t-hmi";
constexpr gpio_num_t kPowerEnablePin0 = GPIO_NUM_14;
constexpr gpio_num_t kPowerEnablePin1 = GPIO_NUM_10;

esp_err_t enable_power_pin(gpio_num_t pin) {
	gpio_config_t cfg = {};
	cfg.pin_bit_mask = 1ULL << pin;
	cfg.mode = GPIO_MODE_OUTPUT;
	const esp_err_t cfg_err = gpio_config(&cfg);
	if (cfg_err != ESP_OK) {
		ESP_LOGE(TAG, "gpio_config failed for pin %d: %s", pin, esp_err_to_name(cfg_err));
		return cfg_err;
	}

	const esp_err_t level_err = gpio_set_level(pin, 1);
	if (level_err != ESP_OK) {
		ESP_LOGE(TAG, "gpio_set_level failed for pin %d: %s", pin, esp_err_to_name(level_err));
		return level_err;
	}

	return ESP_OK;
}
} // namespace

extern "C" esp_err_t flx_profile_hwd_init() {
	ESP_LOGI(TAG, "Enabling T-HMI board power rails");
	const esp_err_t rail0_err = enable_power_pin(kPowerEnablePin0);
	if (rail0_err != ESP_OK) {
		return rail0_err;
	}

	const esp_err_t rail1_err = enable_power_pin(kPowerEnablePin1);
	if (rail1_err != ESP_OK) {
		return rail1_err;
	}

	return ESP_OK;
}
