#pragma once

#include <vector>

namespace flx::hal::input {

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

struct KeyValue_t {
	const char* value_first;
	const int value_num_first;
	const char* value_second;
	const int value_num_second;
};

// Global shared mapping matrix
extern const KeyValue_t CardputerKeyValueMap[4][14];

} // namespace flx::hal::input
