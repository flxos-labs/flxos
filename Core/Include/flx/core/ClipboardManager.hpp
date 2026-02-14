#pragma once
#include <flx/core/Singleton.hpp>
#include <string>

namespace flx {

enum class ClipboardOp { NONE,
						 COPY,
						 CUT };

struct ClipboardEntry {
	std::string path {};
	bool isDir {};
	ClipboardOp op;
};

class ClipboardManager : public Singleton<ClipboardManager> {
	friend class Singleton<ClipboardManager>;

public:

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

} // namespace flx
