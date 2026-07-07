#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/i2c/II2cBus.hpp>
#include <flx/hal/input/IInputDevice.hpp>
#include <lvgl.h>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace flx::hal::input {

/**
 * @brief M5Stack Cardputer Adv Keyboard Device.
 * Reads keyboard events using the TCA8418 I2C keyboard scanner.
 */
class CardputerAdvKeyboard : public DeviceBase<IKeyboardDevice> {
public:

	explicit CardputerAdvKeyboard(std::shared_ptr<flx::hal::i2c::II2cBus> i2cBus, uint8_t i2cAddr = 0x34);
	~CardputerAdvKeyboard() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "Cardputer Adv Keyboard"; }
	std::string_view getDescription() const override { return "Cardputer Adv 56-key keyboard driven by TCA8418 I2C chip"; }
	bool start() override;
	bool stop() override;

	// ── IInputDevice ──────────────────────────────────────────────────────
	lv_indev_t* getLvglIndev() const override;
	int subscribeKeyEvents(KeyEventCallback cb) override;
	void unsubscribeKeyEvents(int id) override;

private:

	struct Point2D {
		int x;
		int y;
	};

	struct KeyState {
		bool tab = false;
		bool fn = false;
		bool shift = false;
		bool ctrl = false;
		bool opt = false;
		bool alt = false;
		bool del = false;
		bool enter = false;
		bool space = false;
		std::vector<char> values;
		std::vector<int> hidKey;

		void reset() {
			tab = false;
			fn = false;
			shift = false;
			ctrl = false;
			opt = false;
			alt = false;
			del = false;
			enter = false;
			space = false;
			values.clear();
			hidKey.clear();
		}
	};

	void updateKeyList();
	void updateKeysState();
	void remap(uint8_t& row, uint8_t& col);

	std::shared_ptr<flx::hal::i2c::II2cBus> m_i2cBus;
	uint8_t m_i2cAddr;
	bool m_pressedKeys[8][10];

	lv_indev_t* m_indev = nullptr;
	std::mutex m_mutex;
	std::vector<std::pair<int, KeyEventCallback>> m_observers;
	int m_nextObserverId = 1;

	std::vector<Point2D> m_keyListBuffer;
	std::vector<Point2D> m_keyValuesWithoutSpecialKeys;
	KeyState m_keysStateBuffer;
	uint8_t m_lastKeySize = 0;
	uint32_t m_lastKeyPressed = 0;
	bool m_isPressed = false;

	static void indevReadCb(lv_indev_t* indev, lv_indev_data_t* data);
	void readInput(lv_indev_data_t* data);
	void notifyObservers(const KeyEvent& event);
};

} // namespace flx::hal::input
