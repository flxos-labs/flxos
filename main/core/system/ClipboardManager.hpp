#pragma once
#include "esp_log.h"
#include <string>

namespace System {

enum class ClipboardOp { NONE,
						 COPY,
						 CUT };

struct ClipboardEntry {
	std::string path;
	bool isDir;
	ClipboardOp op;
};

class ClipboardManager {
public:

	static ClipboardManager& getInstance() {
		static ClipboardManager instance;
		return instance;
	}

	void set(const std::string& path, bool isDir, ClipboardOp op) {
		ESP_LOGI("Clipboard", "Setting clipboard: %s (isDir=%d, op=%d)", path.c_str(), isDir, (int)op);
		m_entry = {path, isDir, op};
	}

	const ClipboardEntry& get() const { return m_entry; }

	void clear() {
		ESP_LOGD("Clipboard", "Clearing clipboard");
		m_entry = {"", false, ClipboardOp::NONE};
	}

	bool hasContent() const { return m_entry.op != ClipboardOp::NONE; }

private:

	ClipboardManager() : m_entry({"", false, ClipboardOp::NONE}) {}
	ClipboardEntry m_entry;
};

} // namespace System
