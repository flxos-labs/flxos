#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/input/CardputerKeyboardCommon.hpp>
#include <flx/hal/input/IInputDevice.hpp>
#include <lvgl.h>
#include <mutex>
#include <vector>

namespace flx::hal::input {

class CardputerKeyboardBase : public DeviceBase<IKeyboardDevice> {
public:

	CardputerKeyboardBase();
	~CardputerKeyboardBase() override;

	// ── IInputDevice ──────────────────────────────────────────────────────
	lv_indev_t* getLvglIndev() const override;
	int subscribeKeyEvents(KeyEventCallback cb) override;
	void unsubscribeKeyEvents(int id) override;

protected:

	bool initLvglIndev();
	void deinitLvglIndev();

	void updateKeysState();
	void notifyObservers(const KeyEvent& event);
	void readInput(lv_indev_data_t* data);

	static void indevReadCb(lv_indev_t* indev, lv_indev_data_t* data);

	// To be implemented by subclasses
	virtual void updateKeyList() = 0;

	lv_indev_t* m_indev = nullptr;
	std::recursive_mutex m_mutex;
	std::vector<std::pair<int, KeyEventCallback>> m_observers;
	int m_nextObserverId = 1;

	std::vector<Point2D> m_keyListBuffer;
	std::vector<Point2D> m_keyValuesWithoutSpecialKeys;
	KeyState m_keysStateBuffer;
	uint32_t m_lastKeyPressed = 0;
	bool m_isPressed = false;
};

} // namespace flx::hal::input
