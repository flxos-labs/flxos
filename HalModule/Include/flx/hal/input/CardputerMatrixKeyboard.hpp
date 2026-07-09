#pragma once

#include <flx/hal/input/CardputerKeyboardBase.hpp>

namespace flx::hal::input {

/**
 * @brief M5Stack Cardputer Matrix Keyboard Device.
 * Scans 56 keys using a 3-to-8 demux columns and 7 row input pins.
 */
class CardputerMatrixKeyboard : public CardputerKeyboardBase {
public:

	CardputerMatrixKeyboard();
	~CardputerMatrixKeyboard() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "Cardputer Keyboard"; }
	std::string_view getDescription() const override { return "Cardputer 56-key matrix keyboard driven by demux scanning"; }
	bool start() override;
	bool stop() override;

protected:
	void updateKeyList() override;

private:
	void initHardware();
	void setDemuxOutput(uint8_t output);
	uint8_t readRowInputs();
};

} // namespace flx::hal::input
