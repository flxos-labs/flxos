#include <cstring>
#include <driver/gpio.h>
#include <flx/core/Logger.hpp>
#include <flx/hal/input/CardputerMatrixKeyboard.hpp>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace flx::hal::input {

namespace {

const std::vector<int> output_list = {8, 9, 11};
const std::vector<int> input_list = {13, 15, 3, 4, 5, 6, 7};

struct Chart_t {
	uint8_t value;
	uint8_t x_1;
	uint8_t x_2;
};

const Chart_t X_map_chart[7] = {
	{1, 0, 1},
	{2, 2, 3},
	{4, 4, 5},
	{8, 6, 7},
	{16, 8, 9},
	{32, 10, 11},
	{64, 12, 13}};

struct KeyValue_t {
	const char* value_first;
	const int value_num_first;
	const char* value_second;
	const int value_num_second;
};

// HID key codes mapped in standard keymap.h
#define KEY_BACKSPACE 0x2a
#define KEY_ENTER 0x28
#define KEY_SPACE 0x2c
#define KEY_TAB 0x2b
#define KEY_SEMICOLON 0x33
#define KEY_DOT 0x37
#define KEY_COMMA 0x36
#define KEY_SLASH 0x38
#define KEY_GRAVE 0x35
#define KEY_APOSTROPHE 0x34
#define KEY_MINUS 0x2d
#define KEY_EQUAL 0x2e
#define KEY_LEFTBRACE 0x2f
#define KEY_RIGHTBRACE 0x30
#define KEY_BACKSLASH 0x31

#define KEY_1 0x1e
#define KEY_2 0x1f
#define KEY_3 0x20
#define KEY_4 0x21
#define KEY_5 0x22
#define KEY_6 0x23
#define KEY_7 0x24
#define KEY_8 0x25
#define KEY_9 0x26
#define KEY_0 0x27

#define KEY_Q 0x14
#define KEY_W 0x1a
#define KEY_E 0x08
#define KEY_R 0x15
#define KEY_T 0x17
#define KEY_Y 0x1c
#define KEY_U 0x18
#define KEY_I 0x0c
#define KEY_O 0x12
#define KEY_P 0x13

#define KEY_A 0x04
#define KEY_S 0x16
#define KEY_D 0x07
#define KEY_F 0x09
#define KEY_G 0x0a
#define KEY_H 0x0b
#define KEY_J 0x0d
#define KEY_K 0x0e
#define KEY_L 0x0f

#define KEY_Z 0x1d
#define KEY_X 0x1b
#define KEY_C 0x06
#define KEY_V 0x19
#define KEY_B 0x05
#define KEY_N 0x11
#define KEY_M 0x10

#define KEY_LEFTCTRL 0xe0
#define KEY_LEFTALT 0xe2
#define KEY_KPASTERISK 0x55
#define KEY_KPLEFTPAREN 0xb6
#define KEY_KPRIGHTPAREN 0xb7
#define KEY_KPMINUS 0x56
#define KEY_KPPLUS 0x57
#define KEY_KPSLASH 0x54

const KeyValue_t _key_value_map[4][14] = {
	{{"`", KEY_GRAVE, "~", KEY_GRAVE},
		{"1", KEY_1, "!", KEY_1},
		{"2", KEY_2, "@", KEY_2},
		{"3", KEY_3, "#", KEY_3},
		{"4", KEY_4, "$", KEY_4},
		{"5", KEY_5, "%", KEY_5},
		{"6", KEY_6, "^", KEY_6},
		{"7", KEY_7, "&", KEY_7},
		{"8", KEY_8, "*", KEY_KPASTERISK},
		{"9", KEY_9, "(", KEY_KPLEFTPAREN},
		{"0", KEY_0, ")", KEY_KPRIGHTPAREN},
		{"-", KEY_MINUS, "_", KEY_KPMINUS},
		{"=", KEY_EQUAL, "+", KEY_KPPLUS},
		{"del", KEY_BACKSPACE, "del", KEY_BACKSPACE}},
	{{"tab", KEY_TAB, "tab", KEY_TAB},
		{"q", KEY_Q, "Q", KEY_Q},
		{"w", KEY_W, "W", KEY_W},
		{"e", KEY_E, "E", KEY_E},
		{"r", KEY_R, "R", KEY_R},
		{"t", KEY_T, "T", KEY_T},
		{"y", KEY_Y, "Y", KEY_Y},
		{"u", KEY_U, "U", KEY_U},
		{"i", KEY_I, "I", KEY_I},
		{"o", KEY_O, "O", KEY_O},
		{"p", KEY_P, "P", KEY_P},
		{"[", KEY_LEFTBRACE, "{", KEY_LEFTBRACE},
		{"]", KEY_RIGHTBRACE, "}", KEY_RIGHTBRACE},
		{"\\", KEY_BACKSLASH, "|", KEY_BACKSLASH}},
	{{"fn", 0, "fn", 0},
		{"shift", 0, "shift", 0},
		{"a", KEY_A, "A", KEY_A},
		{"s", KEY_S, "S", KEY_S},
		{"d", KEY_D, "D", KEY_D},
		{"f", KEY_F, "F", KEY_F},
		{"g", KEY_G, "G", KEY_G},
		{"h", KEY_H, "H", KEY_H},
		{"j", KEY_J, "J", KEY_J},
		{"k", KEY_K, "K", KEY_K},
		{"l", KEY_L, "L", KEY_L},
		{";", KEY_SEMICOLON, ":", KEY_SEMICOLON},
		{"'", KEY_APOSTROPHE, "\"", KEY_APOSTROPHE},
		{"enter", KEY_ENTER, "enter", KEY_ENTER}},
	{{"ctrl", KEY_LEFTCTRL, "ctrl", KEY_LEFTCTRL},
		{"opt", 0, "opt", 0},
		{"alt", KEY_LEFTALT, "alt", KEY_LEFTALT},
		{"z", KEY_Z, "Z", KEY_Z},
		{"x", KEY_X, "X", KEY_X},
		{"c", KEY_C, "C", KEY_C},
		{"v", KEY_V, "V", KEY_V},
		{"b", KEY_B, "B", KEY_B},
		{"n", KEY_N, "N", KEY_N},
		{"m", KEY_M, "M", KEY_M},
		{",", KEY_COMMA, "<", KEY_COMMA},
		{".", KEY_DOT, ">", KEY_DOT},
		{"/", KEY_KPSLASH, "?", KEY_KPSLASH},
		{"space", KEY_SPACE, "space", KEY_SPACE}}};

} // namespace

CardputerMatrixKeyboard::CardputerMatrixKeyboard() {
	this->setState(State::Uninitialized);
}

CardputerMatrixKeyboard::~CardputerMatrixKeyboard() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool CardputerMatrixKeyboard::start() {
	if (getState() == State::Ready) {
		flx::Log::warn("CardputerKeyboard", "start() called on already-running keyboard; ignoring");
		return true;
	}

	this->setState(State::Starting);

	initHardware();

	m_indev = lv_indev_create();
	if (!m_indev) {
		flx::Log::error("CardputerKeyboard", "Failed to create LVGL indev");
		this->setState(State::Error);
		return false;
	}

	lv_indev_set_type(m_indev, LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_read_cb(m_indev, indevReadCb);
	lv_indev_set_user_data(m_indev, this);

	flx::Log::info("CardputerKeyboard", "Cardputer Matrix Keyboard started");
	this->setState(State::Ready);
	return true;
}

bool CardputerMatrixKeyboard::stop() {
	if (m_indev) {
		lv_indev_delete(m_indev);
		m_indev = nullptr;
	}
	this->setState(State::Stopped);
	return true;
}

lv_indev_t* CardputerMatrixKeyboard::getLvglIndev() const {
	return m_indev;
}

int CardputerMatrixKeyboard::subscribeKeyEvents(KeyEventCallback cb) {
	std::lock_guard<std::mutex> lock(m_mutex);
	int id = m_nextObserverId++;
	m_observers.emplace_back(id, cb);
	return id;
}

void CardputerMatrixKeyboard::unsubscribeKeyEvents(int id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
		if (it->first == id) {
			m_observers.erase(it);
			break;
		}
	}
}

void CardputerMatrixKeyboard::initHardware() {
	for (auto i: output_list) {
		gpio_reset_pin(static_cast<gpio_num_t>(i));
		gpio_set_direction(static_cast<gpio_num_t>(i), GPIO_MODE_OUTPUT);
		gpio_set_pull_mode(static_cast<gpio_num_t>(i), GPIO_PULLUP_PULLDOWN);
		gpio_set_level(static_cast<gpio_num_t>(i), 0);
	}

	for (auto i: input_list) {
		gpio_reset_pin(static_cast<gpio_num_t>(i));
		gpio_set_direction(static_cast<gpio_num_t>(i), GPIO_MODE_INPUT);
		gpio_set_pull_mode(static_cast<gpio_num_t>(i), GPIO_PULLUP_ONLY);
	}

	setDemuxOutput(0);
}

void CardputerMatrixKeyboard::setDemuxOutput(uint8_t output) {
	output = output & 0B00000111;
	gpio_set_level(static_cast<gpio_num_t>(output_list[0]), (output >> 0) & 0x01);
	gpio_set_level(static_cast<gpio_num_t>(output_list[1]), (output >> 1) & 0x01);
	gpio_set_level(static_cast<gpio_num_t>(output_list[2]), (output >> 2) & 0x01);
}

uint8_t CardputerMatrixKeyboard::readRowInputs() {
	uint8_t buffer = 0x00;
	uint8_t pin_value = 0x00;

	for (int i = 0; i < 7; i++) {
		pin_value = (gpio_get_level(static_cast<gpio_num_t>(input_list[i])) == 1) ? 0x00 : 0x01;
		pin_value = pin_value << i;
		buffer = buffer | pin_value;
	}

	return buffer;
}

void CardputerMatrixKeyboard::updateKeyList() {
	m_keyListBuffer.clear();

	Point2D coor;
	uint8_t input_value = 0;

	for (int i = 0; i < 8; i++) {
		setDemuxOutput(i);
		input_value = readRowInputs();

		/* If key pressed */
		if (input_value) {
			/* Get X */
			for (int j = 0; j < 7; j++) {
				if (input_value & (0x01 << j)) {
					coor.x = (i > 3) ? X_map_chart[j].x_1 : X_map_chart[j].x_2;

					/* Get Y */
					coor.y = (i > 3) ? (i - 4) : i;

					/* Keep the same as picture */
					coor.y = -coor.y;
					coor.y = coor.y + 3;

					m_keyListBuffer.push_back(coor);
				}
			}
		}
	}
}

void CardputerMatrixKeyboard::updateKeysState() {
	m_keysStateBuffer.reset();
	m_keyValuesWithoutSpecialKeys.clear();

	// Get special keys
	for (auto& i: m_keyListBuffer) {
		const char* val = _key_value_map[i.y][i.x].value_first;
		if (strcmp(val, "tab") == 0) {
			m_keysStateBuffer.tab = true;
			continue;
		}
		if (strcmp(val, "fn") == 0) {
			m_keysStateBuffer.fn = true;
			continue;
		}
		if (strcmp(val, "shift") == 0) {
			m_keysStateBuffer.shift = true;
			continue;
		}
		if (strcmp(val, "ctrl") == 0) {
			m_keysStateBuffer.ctrl = true;
			continue;
		}
		if (strcmp(val, "opt") == 0) {
			m_keysStateBuffer.opt = true;
			continue;
		}
		if (strcmp(val, "alt") == 0) {
			m_keysStateBuffer.alt = true;
			continue;
		}
		if (strcmp(val, "del") == 0) {
			m_keysStateBuffer.del = true;
			m_keysStateBuffer.hidKey.push_back(KEY_BACKSPACE);
			continue;
		}
		if (strcmp(val, "enter") == 0) {
			m_keysStateBuffer.enter = true;
			m_keysStateBuffer.hidKey.push_back(KEY_ENTER);
			continue;
		}
		if (strcmp(val, "space") == 0) {
			m_keysStateBuffer.space = true;
			m_keysStateBuffer.hidKey.push_back(KEY_SPACE);
			continue;
		}

		m_keyValuesWithoutSpecialKeys.push_back(i);
	}

	// Deal with the rest
	for (auto& i: m_keyValuesWithoutSpecialKeys) {
		if (m_keysStateBuffer.ctrl || m_keysStateBuffer.shift) {
			m_keysStateBuffer.values.push_back(*_key_value_map[i.y][i.x].value_second);
			m_keysStateBuffer.hidKey.push_back(_key_value_map[i.y][i.x].value_num_second);
		} else {
			m_keysStateBuffer.values.push_back(*_key_value_map[i.y][i.x].value_first);
			m_keysStateBuffer.hidKey.push_back(_key_value_map[i.y][i.x].value_num_first);
		}
	}
}

void CardputerMatrixKeyboard::notifyObservers(const KeyEvent& event) {
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

void CardputerMatrixKeyboard::indevReadCb(lv_indev_t* indev, lv_indev_data_t* data) {
	auto* device = static_cast<CardputerMatrixKeyboard*>(lv_indev_get_user_data(indev));
	if (device) {
		device->readInput(data);
	}
}

void CardputerMatrixKeyboard::readInput(lv_indev_data_t* data) {
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
