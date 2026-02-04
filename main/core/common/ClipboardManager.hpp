#pragma once
#include <string>

namespace System {

enum class ClipboardOp { NONE,
						 COPY,
						 CUT };

struct ClipboardEntry {
	std::string path {};
	bool isDir {};
	ClipboardOp op;
};

class ClipboardManager {
public:

	static ClipboardManager& getInstance() {
		static ClipboardManager instance;
		return instance;
	}

	void set(const std::string& path, bool isDir, ClipboardOp op) {
		m_entry = {path, isDir, op};
	}

	const ClipboardEntry& get() const { return m_entry; }

	void clear() {
		m_entry = {"", false, ClipboardOp::NONE};
	}

	bool hasContent() const { return m_entry.op != ClipboardOp::NONE; }

private:

	ClipboardManager() : m_entry({"", false, ClipboardOp::NONE}) {}
	ClipboardEntry m_entry;
};

} // namespace System
