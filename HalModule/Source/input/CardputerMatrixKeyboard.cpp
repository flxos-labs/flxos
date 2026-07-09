#include <algorithm>
#include <driver/gpio.h>
#include <flx/core/Logger.hpp>
#include <flx/hal/input/CardputerMatrixKeyboard.hpp>

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

} // namespace

CardputerMatrixKeyboard::CardputerMatrixKeyboard() {
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

	if (!initLvglIndev()) {
		flx::Log::error("CardputerKeyboard", "Failed to create LVGL indev");
		this->setState(State::Error);
		return false;
	}

	flx::Log::info("CardputerKeyboard", "Cardputer Matrix Keyboard started");
	this->setState(State::Ready);
	return true;
}

bool CardputerMatrixKeyboard::stop() {
	deinitLvglIndev();
	this->setState(State::Stopped);
	return true;
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

} // namespace flx::hal::input
