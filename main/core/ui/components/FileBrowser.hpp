#pragma once

#include "core/apps/settings/SettingsCommon.hpp"
#include "core/services/filesystem/FileSystemService.hpp"
#include "lvgl.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <string.h>
#include <vector>

namespace System::UI {

/**
 * FileBrowser - A screen-based file browser component for use in apps.
 * Creates a full page that integrates with app navigation using show/hide pattern.
 */
class FileBrowser {
public:

	using FileSelectedCallback = std::function<void(const std::string& vfsPath)>;
	using BackCallback = std::function<void()>;

	/**
	 * Constructor (integrated with app navigation)
	 */
	FileBrowser(lv_obj_t* parent, BackCallback onBack);

	~FileBrowser();

	// Non-copyable and non-movable: destructor frees LVGL objects and
	// callbacks capture `this`, so copies/moves would cause double-free
	// and dangling-pointer bugs.
	FileBrowser(const FileBrowser&) = delete;
	FileBrowser& operator=(const FileBrowser&) = delete;
	FileBrowser(FileBrowser&&) = delete;
	FileBrowser& operator=(FileBrowser&&) = delete;

	/**
	 * Show the file browser screen.
	 * @param forSave If true, shows filename input for save operations
	 * @param onFileSelected Callback when a file is selected
	 * @param defaultFilename Default filename for save mode
	 */
	void show(bool forSave, FileSelectedCallback onFileSelected, const std::string& defaultFilename = "untitled.txt");

	/**
	 * Hide the file browser screen.
	 */
	void hide();

	/**
	 * Destroy and cleanup resources.
	 */
	void destroy();

	/**
	 * Set the initial directory path.
	 */
	void setInitialPath(const std::string& path) { m_currentPath = path; }

	/**
	 * Set file extension filters (e.g., {".txt", ".md"})
	 */
	void setExtensions(const std::vector<std::string>& extensions) { m_extensions = extensions; }

private:

	lv_obj_t* m_parent {nullptr};
	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_list {nullptr};
	lv_obj_t* m_pathLabel {nullptr};
	lv_obj_t* m_filenameInput {nullptr};
	lv_obj_t* m_actionBtn {nullptr};

	BackCallback m_onBack;
	FileSelectedCallback m_onFileSelected;
	std::string m_currentPath {"A:/"};
	std::vector<std::string> m_extensions {};
	bool m_forSave {false};

	void createUI();
	void refreshList();
	void navigateUp();
	void enterDirectory(const std::string& name);
	void selectFile(const std::string& name);
	void confirmSelection();

	static bool hasExtension(const std::string& fileName, const std::string& ext);
};

// ============================================================================
// Implementation (header-only for simplicity like other UI components)
// ============================================================================

inline FileBrowser::FileBrowser(lv_obj_t* parent, BackCallback onBack)
	: m_parent(parent), m_onBack(std::move(onBack)) {}

inline FileBrowser::~FileBrowser() {
	destroy();
}

inline void FileBrowser::show(bool forSave, FileSelectedCallback onFileSelected, const std::string& defaultFilename) {
	m_forSave = forSave;
	m_onFileSelected = std::move(onFileSelected);

	if (m_container == nullptr) {
		createUI();
	}

	// Update UI for mode
	if (m_filenameInput) {
		if (forSave) {
			lv_obj_remove_flag(lv_obj_get_parent(m_filenameInput), LV_OBJ_FLAG_HIDDEN);
			lv_textarea_set_text(m_filenameInput, defaultFilename.c_str());
		} else {
			lv_obj_add_flag(lv_obj_get_parent(m_filenameInput), LV_OBJ_FLAG_HIDDEN);
		}
	}

	if (m_actionBtn) {
		lv_obj_t* label = lv_obj_get_child(m_actionBtn, 0);
		if (label) {
			lv_label_set_text(label, forSave ? "Save" : "Open");
		}
		if (forSave) {
			lv_obj_remove_flag(m_actionBtn, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_add_flag(m_actionBtn, LV_OBJ_FLAG_HIDDEN);
		}
	}

	lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	refreshList();
}

inline void FileBrowser::hide() {
	if (m_container) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

inline void FileBrowser::destroy() {
	if (m_container) {
		lv_obj_del(m_container);
		m_container = nullptr;
	}
	m_list = nullptr;
	m_pathLabel = nullptr;
	m_filenameInput = nullptr;
	m_actionBtn = nullptr;
}

inline void FileBrowser::createUI() {
	using namespace Apps::Settings;

	m_container = create_page_container(m_parent);

	// Header with back button
	lv_obj_t* backBtn = nullptr;
	lv_obj_t* header = create_header(m_container, "Browse", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	// Path label
	m_pathLabel = lv_label_create(header);
	lv_obj_set_style_pad_left(m_pathLabel, 8, 0);
	lv_obj_set_style_pad_right(m_pathLabel, 8, 0);
	lv_label_set_long_mode(m_pathLabel, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
	lv_obj_set_flex_grow(m_pathLabel, 1);

	// Spacer for right alignment
	lv_obj_t* spacer = lv_obj_create(header);
	lv_obj_set_size(spacer, 0, 0);
	lv_obj_set_flex_grow(spacer, 1);
	lv_obj_set_style_border_width(spacer, 0, 0);
	lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);

	// Action button in header (for save mode)
	m_actionBtn = lv_button_create(header);
	lv_obj_set_size(m_actionBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* actionLabel = lv_label_create(m_actionBtn);
	lv_label_set_text(actionLabel, "Save");
	lv_obj_add_event_cb(m_actionBtn, [](lv_event_t* e) {
		auto* browser = static_cast<FileBrowser*>(lv_event_get_user_data(e));
		browser->confirmSelection(); }, LV_EVENT_CLICKED, this);
	lv_obj_add_flag(m_actionBtn, LV_OBJ_FLAG_HIDDEN);

	// Filename input row (for save mode)
	lv_obj_t* inputRow = lv_obj_create(m_container);
	lv_obj_set_size(inputRow, lv_pct(100), 44);
	lv_obj_set_style_pad_all(inputRow, 4, 0);
	lv_obj_set_style_border_width(inputRow, 0, 0);
	lv_obj_set_style_bg_opa(inputRow, LV_OPA_TRANSP, 0);
	lv_obj_set_flex_flow(inputRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(inputRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_remove_flag(inputRow, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(inputRow, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t* fnLabel = lv_label_create(inputRow);
	lv_label_set_text(fnLabel, "Name:");
	lv_obj_set_style_pad_right(fnLabel, 8, 0);

	m_filenameInput = lv_textarea_create(inputRow);
	lv_obj_set_flex_grow(m_filenameInput, 1);
	lv_textarea_set_one_line(m_filenameInput, true);
	lv_textarea_set_text(m_filenameInput, "untitled.txt");

	// File list
	m_list = create_settings_list(m_container);
}

inline void FileBrowser::refreshList() {
	if (!m_list) return;

	lv_obj_clean(m_list);
	lv_label_set_text(m_pathLabel, m_currentPath.c_str());

	// Add parent directory option if not at root
	if (m_currentPath != "A:/") {
		lv_obj_t* parentBtn = lv_list_add_button(m_list, LV_SYMBOL_UP, "..");
		lv_obj_add_event_cb(parentBtn, [](lv_event_t* e) {
			auto* browser = static_cast<FileBrowser*>(lv_event_get_user_data(e));
			browser->navigateUp(); }, LV_EVENT_CLICKED, this);
	}

	auto entries = Services::FileSystemService::getInstance().listDirectory(m_currentPath);
	for (const auto& entry: entries) {
		// Apply extension filter if set
		if (!entry.isDirectory && !m_extensions.empty()) {
			bool match = false;
			for (const auto& ext: m_extensions) {
				if (hasExtension(entry.name, ext)) {
					match = true;
					break;
				}
			}
			if (!match) continue;
		}

		const char* icon = entry.isDirectory ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;
		lv_obj_t* btn = lv_list_add_button(m_list, icon, entry.name.c_str());

		// Store entry info
		struct EntryData {
			FileBrowser* browser;
			std::string name;
			bool isDir;
		};
		// Note: EntryData leaked if list cleaned without DELETE event handling?
		// The original code had DELETE event handler, making sure I keep it.
		auto* data = new EntryData {this, entry.name, entry.isDirectory};

		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
			auto* data = static_cast<EntryData*>(lv_event_get_user_data(e));
			if (data->isDir) {
				data->browser->enterDirectory(data->name);
			} else {
				data->browser->selectFile(data->name);
			} }, LV_EVENT_CLICKED, data);

		// Cleanup on button delete
		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
			if (lv_event_get_code(e) == LV_EVENT_DELETE) {
				delete static_cast<EntryData*>(lv_event_get_user_data(e));
			} }, LV_EVENT_DELETE, data);
	}
}

inline void FileBrowser::navigateUp() {
	if (m_currentPath == "A:/") return;

	size_t pos = m_currentPath.find_last_of('/');
	if (pos != std::string::npos) {
		if (pos == 2) {
			m_currentPath = "A:/";
		} else {
			m_currentPath = m_currentPath.substr(0, pos);
		}
	}
	refreshList();
}

inline void FileBrowser::enterDirectory(const std::string& name) {
	m_currentPath = Services::FileSystemService::buildPath(m_currentPath, name);
	refreshList();
}

inline void FileBrowser::selectFile(const std::string& name) {
	if (m_forSave && m_filenameInput) {
		// In save mode, clicking a file sets the filename
		lv_textarea_set_text(m_filenameInput, name.c_str());
	} else {
		// In open mode, clicking a file opens it
		std::string vfsPath = Services::FileSystemService::toVfsPath(
			Services::FileSystemService::buildPath(m_currentPath, name)
		);
		if (m_onFileSelected) {
			m_onFileSelected(vfsPath);
		}
	}
}

inline void FileBrowser::confirmSelection() {
	if (!m_filenameInput) return;

	const char* filename = lv_textarea_get_text(m_filenameInput);
	if (filename && strlen(filename) > 0) {
		std::string vfsPath = Services::FileSystemService::toVfsPath(
			Services::FileSystemService::buildPath(m_currentPath, filename)
		);
		if (m_onFileSelected) {
			m_onFileSelected(vfsPath);
		}
	}
}

inline bool FileBrowser::hasExtension(const std::string& fileName, const std::string& ext) {
	if (fileName.length() < ext.length()) return false;
	auto toLower = [](const std::string& s) {
		std::string lower = s;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
		return lower;
	};
	return toLower(fileName.substr(fileName.length() - ext.length())) == toLower(ext);
}

} // namespace System::UI
