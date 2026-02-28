#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <atomic>
#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/gpio/IGpioController.hpp>
#include <mutex>
#include <string_view>
#include <unordered_map>

namespace flx::hal::gpio {

class EspGpioController : public DeviceBase<IGpioController> {
public:

	EspGpioController();
	~EspGpioController() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "ESP GPIO"; }
	std::string_view getDescription() const override { return "ESP-IDF GPIO Controller with ISR/Debounce"; }
	bool start() override;
	bool stop() override;
	Type getType() const override { return IGpioController::getType(); }

	// ── IGpioController ───────────────────────────────────────────────────
	bool configure(Pin pin, GpioMode mode, GpioPullMode pull = GpioPullMode::None) override;
	bool setLevel(Pin pin, bool level) override;
	bool getLevel(Pin pin) const override;
	int getPinCount() const override;

	bool attachInterrupt(Pin pin, GpioInterruptEdge edge, IsrCallback callback) override;
	bool detachInterrupt(Pin pin) override;
	bool configureDebounced(Pin pin, uint32_t debounceMs, IsrCallback callback) override;

private:

	struct PinConfig {
		EspGpioController* controller;
		Pin pin;
		IsrCallback callback;
		uint32_t debounceMs;
		TickType_t lastIsrTick;
		bool isDebounced;
	};

	static void IRAM_ATTR gpioIsrHandler(void* arg);
	static void debounceTaskRunner(void* arg);

	std::unordered_map<Pin, PinConfig*> m_pinConfigs;
	std::mutex m_mutex;
	bool m_isrServiceInstalled = false;

	QueueHandle_t m_debounceQueue = nullptr;
	TaskHandle_t m_debounceTaskHandle = nullptr;
	std::atomic<bool> m_taskRunning {false};
};

} // namespace flx::hal::gpio
