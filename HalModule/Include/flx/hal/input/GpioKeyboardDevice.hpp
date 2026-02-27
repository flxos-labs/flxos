#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/input/IInputDevice.hpp>
#include <mutex>
#include <vector>
#include <lvgl.h>

namespace flx::hal::input {

/**
 * @brief Basic GPIO-based Keyboard Device.
 * Maps an array of GPIO pins to specific LVGL key codes.
 */
class GpioKeyboardDevice : public DeviceBase<IKeyboardDevice> {
public:
	GpioKeyboardDevice(const std::vector<int>& pins, const std::vector<uint32_t>& keyCodes);
	~GpioKeyboardDevice() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "GPIO Keyboard"; }
	std::string_view getDescription() const override { return "Basic GPIO-based keyboard mapped to LVGL"; }
	bool start() override;
	bool stop() override;
	// ── IInputDevice ──────────────────────────────────────────────────────
	lv_indev_t* getLvglIndev() const override;
	int subscribeKeyEvents(KeyEventCallback cb) override;
	void unsubscribeKeyEvents(int id) override;

private:
	std::vector<int> m_pins;
	std::vector<uint32_t> m_keyCodes;
	lv_indev_t* m_indev = nullptr;

	std::mutex m_mutex;
	std::vector<std::pair<int, KeyEventCallback>> m_observers;
	int m_nextObserverId = 1;

	uint32_t m_lastKeyPressed = 0;
	bool m_isPressed = false;

	static void indevReadCb(lv_indev_t* indev, lv_indev_data_t* data);
	void readInput(lv_indev_data_t* data);
	void notifyObservers(const KeyEvent& event);
};

} // namespace flx::hal::input
