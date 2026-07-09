#include <cstring>
#include <flx/core/Logger.hpp>
#include <flx/hal/input/CardputerKeyboardBase.hpp>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace flx::hal::input {

CardputerKeyboardBase::CardputerKeyboardBase() {
	this->setState(State::Uninitialized);
}

CardputerKeyboardBase::~CardputerKeyboardBase() {
	deinitLvglIndev();
}

bool CardputerKeyboardBase::initLvglIndev() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_indev = lv_indev_create();
	if (!m_indev) {
		return false;
	}
	lv_indev_set_type(m_indev, LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_read_cb(m_indev, indevReadCb);
	lv_indev_set_user_data(m_indev, this);
	return true;
}

void CardputerKeyboardBase::deinitLvglIndev() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_indev) {
		lv_indev_delete(m_indev);
		m_indev = nullptr;
	}
}

lv_indev_t* CardputerKeyboardBase::getLvglIndev() const {
	return m_indev;
}

int CardputerKeyboardBase::subscribeKeyEvents(KeyEventCallback cb) {
	std::lock_guard<std::mutex> lock(m_mutex);
	int id = m_nextObserverId++;
	m_observers.push_back({id, cb});
	return id;
}

void CardputerKeyboardBase::unsubscribeKeyEvents(int id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
		if (it->first == id) {
			m_observers.erase(it);
			break;
		}
	}
}

void CardputerKeyboardBase::updateKeysState() {
	m_keysStateBuffer.reset();
	m_keyValuesWithoutSpecialKeys.clear();

	// Get special/modifier keys
	for (auto& i: m_keyListBuffer) {
		const char* val = CardputerKeyValueMap[i.y][i.x].value_first;
		if (std::strcmp(val, "tab") == 0) {
			m_keysStateBuffer.tab = true;
			continue;
		}
		if (std::strcmp(val, "fn") == 0) {
			m_keysStateBuffer.fn = true;
			continue;
		}
		if (std::strcmp(val, "shift") == 0) {
			m_keysStateBuffer.shift = true;
			continue;
		}
		if (std::strcmp(val, "ctrl") == 0) {
			m_keysStateBuffer.ctrl = true;
			continue;
		}
		if (std::strcmp(val, "opt") == 0) {
			m_keysStateBuffer.opt = true;
			continue;
		}
		if (std::strcmp(val, "alt") == 0) {
			m_keysStateBuffer.alt = true;
			continue;
		}
		if (std::strcmp(val, "del") == 0) {
			m_keysStateBuffer.del = true;
			m_keysStateBuffer.hidKey.push_back(0x2a); // KEY_BACKSPACE
			continue;
		}
		if (std::strcmp(val, "enter") == 0) {
			m_keysStateBuffer.enter = true;
			m_keysStateBuffer.hidKey.push_back(0x28); // KEY_ENTER
			continue;
		}
		if (std::strcmp(val, "space") == 0) {
			m_keysStateBuffer.space = true;
			m_keysStateBuffer.hidKey.push_back(0x2c); // KEY_SPACE
			continue;
		}

		m_keyValuesWithoutSpecialKeys.push_back(i);
	}

	// Deal with the rest of the keys
	for (auto& i: m_keyValuesWithoutSpecialKeys) {
		if (m_keysStateBuffer.ctrl || m_keysStateBuffer.shift) {
			m_keysStateBuffer.values.push_back(*CardputerKeyValueMap[i.y][i.x].value_second);
			m_keysStateBuffer.hidKey.push_back(CardputerKeyValueMap[i.y][i.x].value_num_second);
		} else {
			m_keysStateBuffer.values.push_back(*CardputerKeyValueMap[i.y][i.x].value_first);
			m_keysStateBuffer.hidKey.push_back(CardputerKeyValueMap[i.y][i.x].value_num_first);
		}
	}
}

void CardputerKeyboardBase::notifyObservers(const KeyEvent& event) {
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

void CardputerKeyboardBase::indevReadCb(lv_indev_t* indev, lv_indev_data_t* data) {
	auto* device = static_cast<CardputerKeyboardBase*>(lv_indev_get_user_data(indev));
	if (device) {
		device->readInput(data);
	}
}

void CardputerKeyboardBase::readInput(lv_indev_data_t* data) {
	updateKeyList();

	bool anyPressed = false;
	uint32_t currentKey = 0;

	if (!m_keyListBuffer.empty()) {
		updateKeysState();

		if (!m_keysStateBuffer.fn) {
			if (m_keysStateBuffer.enter) {
				currentKey = LV_KEY_ENTER;
				anyPressed = true;
			} else if (m_keysStateBuffer.space) {
				currentKey = ' ';
				anyPressed = true;
			} else if (m_keysStateBuffer.del) {
				currentKey = LV_KEY_BACKSPACE;
				anyPressed = true;
			} else if (m_keysStateBuffer.tab) {
				currentKey = m_keysStateBuffer.shift ? LV_KEY_PREV : LV_KEY_NEXT;
				anyPressed = true;
			} else if (!m_keysStateBuffer.values.empty()) {
				currentKey = static_cast<uint8_t>(m_keysStateBuffer.values[0]);
				anyPressed = true;
			}
		} else {
			// Fn modifier active
			if (m_keysStateBuffer.del) {
				currentKey = LV_KEY_DEL;
				anyPressed = true;
			} else if (!m_keysStateBuffer.values.empty()) {
				char ch = m_keysStateBuffer.values[0];
				if (ch == ';') { // Fn + ; -> Up
					currentKey = LV_KEY_UP;
					anyPressed = true;
				} else if (ch == '.') { // Fn + . -> Down
					currentKey = LV_KEY_DOWN;
					anyPressed = true;
				} else if (ch == ',') { // Fn + , -> Left
					currentKey = LV_KEY_LEFT;
					anyPressed = true;
				} else if (ch == '/') { // Fn + / -> Right
					currentKey = LV_KEY_RIGHT;
					anyPressed = true;
				} else if (ch == '`') { // Fn + ` -> Esc
					currentKey = LV_KEY_ESC;
					anyPressed = true;
				} else {
					// Fallback to normal character press with Fn
					currentKey = static_cast<uint8_t>(ch);
					anyPressed = true;
				}
			}
		}
	}

	if (anyPressed && (!m_isPressed || m_lastKeyPressed != currentKey)) {
		if (m_isPressed) {
			// Release the old key first before pressing the new key
			notifyObservers({m_lastKeyPressed, false, static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS)});
		}
		m_isPressed = true;
		m_lastKeyPressed = currentKey;
		notifyObservers({currentKey, true, static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS)});
	} else if (!anyPressed && m_isPressed) {
		m_isPressed = false;
		notifyObservers({m_lastKeyPressed, false, static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS)});
	}

	data->state = anyPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
	data->key = anyPressed ? currentKey : m_lastKeyPressed;
}

} // namespace flx::hal::input
