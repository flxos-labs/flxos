#include <cstring>
#include <flx/core/Logger.hpp>
#include <flx/hal/input/CardputerAdvKeyboard.hpp>

namespace flx::hal::input {

CardputerAdvKeyboard::CardputerAdvKeyboard(std::shared_ptr<flx::hal::i2c::II2cBus> i2cBus, uint8_t i2cAddr)
	: m_i2cBus(i2cBus), m_i2cAddr(i2cAddr) {
}

CardputerAdvKeyboard::~CardputerAdvKeyboard() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool CardputerAdvKeyboard::start() {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	if (getState() == State::Ready) {
		flx::Log::warn("CardputerAdvKeyboard", "start() called on already-running keyboard; ignoring");
		return true;
	}

	this->setState(State::Starting);

	if (!m_i2cBus) {
		flx::Log::error("CardputerAdvKeyboard", "I2C bus not provided");
		this->setState(State::Error);
		return false;
	}

	// ── 1. Configure TCA8418 Keypad Matrix ───────────────────────────────
	// Configure row pins (ROW0 - ROW6)
	if (!m_i2cBus->writeRegister8(m_i2cAddr, 0x1D, 0x7F)) {
		flx::Log::error("CardputerAdvKeyboard", "Failed to write KP_GPIO1 (ROWs) register");
		this->setState(State::Error);
		return false;
	}
	// Configure column pins (COL0 - COL7)
	if (!m_i2cBus->writeRegister8(m_i2cAddr, 0x1E, 0xFF)) {
		flx::Log::error("CardputerAdvKeyboard", "Failed to write KP_GPIO2 (COLs) register");
		this->setState(State::Error);
		return false;
	}
	// Configure CFG register (enable Key Events Interrupt, disable overflow mode)
	if (!m_i2cBus->writeRegister8(m_i2cAddr, 0x01, 0x99)) {
		flx::Log::error("CardputerAdvKeyboard", "Failed to write CFG register");
		this->setState(State::Error);
		return false;
	}

	// ── 2. Flush TCA8418 Key Events FIFO ─────────────────────────────────
	uint8_t event = 0;
	while (m_i2cBus->readRegister8(m_i2cAddr, 0x04, event) && event != 0) {
		// Keep reading until FIFO is empty
	}

	// ── 3. Initialize LVGL Input Device ──────────────────────────────────
	if (!initLvglIndev()) {
		flx::Log::error("CardputerAdvKeyboard", "Failed to create LVGL input device");
		this->setState(State::Error);
		return false;
	}

	flx::Log::info("CardputerAdvKeyboard", "Cardputer Adv Keyboard (TCA8418) started successfully");
	this->setState(State::Ready);
	return true;
}

bool CardputerAdvKeyboard::stop() {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	deinitLvglIndev();
	this->setState(State::Stopped);
	return true;
}

void CardputerAdvKeyboard::remap(uint8_t& row, uint8_t& col) {
	// Col
	uint8_t coltemp = row * 2;
	if (col > 3) coltemp++;

	// Row
	uint8_t rowtemp = (col + 4) % 4;

	row = rowtemp;
	col = coltemp;
}

void CardputerAdvKeyboard::updateKeyList() {
	m_keyListBuffer.clear();

	// Read all pending events from the TCA8418 FIFO
	while (true) {
		uint8_t event = 0;
		if (!m_i2cBus->readRegister8(m_i2cAddr, 0x04, event) || event == 0) {
			break;
		}
		bool pressed = (event & 0x80);
		uint8_t raw_key = (event & 0x7F) - 1;
		uint8_t row = raw_key / 10;
		uint8_t col = raw_key % 10;
		if (row < 8 && col < 10) {
			m_pressedKeys[row][col] = pressed;
		}
	}

	// Build key list coordinates from the current pressed keys state
	Point2D coor;
	for (uint8_t r = 0; r < 7; r++) {
		for (uint8_t c = 0; c < 8; c++) {
			if (m_pressedKeys[r][c]) {
				uint8_t mapped_row = r;
				uint8_t mapped_col = c;
				remap(mapped_row, mapped_col);

				coor.y = mapped_row;
				coor.x = mapped_col;
				m_keyListBuffer.push_back(coor);
			}
		}
	}
}

} // namespace flx::hal::input
