#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/input/IInputDevice.hpp>
#include <lvgl.h>
#include <mutex>
#include <vector>
#include <string_view>

namespace flx::hal::input {

/**
 * @brief M5Stack Cardputer Matrix Keyboard Device.
 * Scans 56 keys using a 3-to-8 demux columns and 7 row input pins.
 */
class CardputerMatrixKeyboard : public DeviceBase<IKeyboardDevice> {
public:

	CardputerMatrixKeyboard();
	~CardputerMatrixKeyboard() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "Cardputer Keyboard"; }
	std::string_view getDescription() const override { return "Cardputer 56-key matrix keyboard driven by demux scanning"; }
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

	void initHardware();
	void setDemuxOutput(uint8_t output);
	uint8_t readRowInputs();
	void updateKeyList();
	void updateKeysState();

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
