#pragma once

#include <flx/hal/i2c/II2cBus.hpp>
#include <flx/hal/input/CardputerKeyboardBase.hpp>
#include <memory>

namespace flx::hal::input {

/**
 * @brief M5Stack Cardputer Adv Keyboard Device.
 * Reads keyboard events using the TCA8418 I2C keyboard scanner.
 */
class CardputerAdvKeyboard : public CardputerKeyboardBase {
public:

	explicit CardputerAdvKeyboard(std::shared_ptr<flx::hal::i2c::II2cBus> i2cBus, uint8_t i2cAddr = 0x34);
	~CardputerAdvKeyboard() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "Cardputer Adv Keyboard"; }
	std::string_view getDescription() const override { return "Cardputer Adv 56-key keyboard driven by TCA8418 I2C chip"; }
	bool start() override;
	bool stop() override;

protected:

	void updateKeyList() override;

private:

	void remap(uint8_t& row, uint8_t& col);

	std::shared_ptr<flx::hal::i2c::II2cBus> m_i2cBus;
	uint8_t m_i2cAddr;
	bool m_pressedKeys[8][10] = {};
};

} // namespace flx::hal::input
