#include <driver/gpio.h>
#include <flx/core/Logger.hpp>
#include <flx/hal/input/GpioKeyboardDevice.hpp>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace flx::hal::input {

static constexpr std::string_view TAG = "GpioKeyboard";

GpioKeyboardDevice::GpioKeyboardDevice(const std::vector<int>& pins, const std::vector<uint32_t>& keyCodes)
	: m_pins(pins), m_keyCodes(keyCodes) {
	this->setState(State::Uninitialized);
}

GpioKeyboardDevice::~GpioKeyboardDevice() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool GpioKeyboardDevice::start() {
	this->setState(State::Starting);

	if (m_pins.size() != m_keyCodes.size()) {
		flx::Log::error(TAG, "Mismatched pins and keycodes count");
		this->setState(State::Error);
		return false;
	}

	// Initialize GPIOs
	for (int pin: m_pins) {
		if (pin >= 0) {
			gpio_config_t io_conf = {};
			io_conf.intr_type = GPIO_INTR_DISABLE;
			io_conf.mode = GPIO_MODE_INPUT;
			io_conf.pin_bit_mask = (1ULL << pin);
			io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
			io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Assume pull-up, buttons pull to GND
			gpio_config(&io_conf);
		}
	}

	// Create LVGL input device
	m_indev = lv_indev_create();
	if (!m_indev) {
		flx::Log::error(TAG, "Failed to create LVGL indev");
		this->setState(State::Error);
		return false;
	}

	lv_indev_set_type(m_indev, LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_read_cb(m_indev, indevReadCb);
	lv_indev_set_user_data(m_indev, this);

	flx::Log::info(TAG, "Keyboard started with %zu keys", m_pins.size());
	this->setState(State::Ready);
	return true;
}

bool GpioKeyboardDevice::stop() {
	if (m_indev) {
		lv_indev_delete(m_indev);
		m_indev = nullptr;
	}
	this->setState(State::Stopped);
	return true;
}

lv_indev_t* GpioKeyboardDevice::getLvglIndev() const {
	return m_indev;
}

int GpioKeyboardDevice::subscribeKeyEvents(KeyEventCallback cb) {
	std::lock_guard<std::mutex> lock(m_mutex);
	int id = m_nextObserverId++;
	m_observers.emplace_back(id, cb);
	return id;
}

void GpioKeyboardDevice::unsubscribeKeyEvents(int id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
		if (it->first == id) {
			m_observers.erase(it);
			break;
		}
	}
}

void GpioKeyboardDevice::notifyObservers(const KeyEvent& event) {
	std::vector<KeyEventCallback> callbacks;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (const auto& observer: m_observers) {
			if (observer.second) callbacks.push_back(observer.second);
		}
	}
	for (const auto& cb: callbacks) {
		cb(event);
	}
}

void GpioKeyboardDevice::indevReadCb(lv_indev_t* indev, lv_indev_data_t* data) {
	auto* device = static_cast<GpioKeyboardDevice*>(lv_indev_get_user_data(indev));
	if (device) {
		device->readInput(data);
	}
}

void GpioKeyboardDevice::readInput(lv_indev_data_t* data) {
	bool anyPressed = false;
	uint32_t currentKey = 0;

	for (size_t i = 0; i < m_pins.size(); ++i) {
		if (m_pins[i] >= 0) {
			// Assuming active LOW (pulled to GND)
			if (gpio_get_level(static_cast<gpio_num_t>(m_pins[i])) == 0) {
				currentKey = m_keyCodes[i];
				anyPressed = true;
				break; // Only handle one key at a time for basic keypad
			}
		}
	}

	if (anyPressed && !m_isPressed) {
		// Just pressed
		m_isPressed = true;
		m_lastKeyPressed = currentKey;
		notifyObservers({currentKey, true, xTaskGetTickCount() * portTICK_PERIOD_MS});
	} else if (!anyPressed && m_isPressed) {
		// Just released
		m_isPressed = false;
		notifyObservers({m_lastKeyPressed, false, xTaskGetTickCount() * portTICK_PERIOD_MS});
	}

	data->state = anyPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
	data->key = anyPressed ? currentKey : m_lastKeyPressed;
}

} // namespace flx::hal::input
