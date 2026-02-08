#pragma once

#include "core/apps/settings/SettingsCommon.hpp"
#include "core/services/filesystem/FileSystemService.hpp"
#include "core/ui/theming/layout_constants/LayoutConstants.hpp"
#include "lvgl.h"
#include <functional>
#include <stack>
#include <string.h>
#include <vector>

namespace System::UI {

/**
 * FileBrowser - A versatile file browser component supporting both full-screen
 * and modal overlay modes. Combines features from the legacy FileChooser.
 * 
 * Full-screen mode: Integrates with app navigation using show/hide pattern
 * Modal mode: Displays as an overlay dialog with self-management
 */
class FileBrowser {
public:

	using FileSelectedCallback = std::function<void(const std::string& vfsPath)>;
	using BackCallback = std::function<void()>;

	/**
	 * Constructor for full-screen mode (integrated with app navigation)
	 */
	FileBrowser(lv_obj_t* parent, BackCallback onBack);

	~FileBrowser() = default;

	/**
	 * Show the file browser screen (full-screen mode).
	 * @param forSave If true, shows filename input for save operations
	 * @param onFileSelected Callback when a file is selected
	 * @param defaultFilename Default filename for save mode
	 */
	void show(bool forSave, FileSelectedCallback onFileSelected, const std::string& defaultFilename = "untitled.txt");

	/**
	 * Hide the file browser screen (full-screen mode).
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

	/**
	 * Static helper to show a modal file chooser dialog (legacy FileChooser API)
	 * @param callback Callback with selected file path
	 * @param extensions Optional file extension filters
	 * @param initialPath Starting directory path
	 */
	static void showModal(FileSelectedCallback callback, const std::vector<std::string>& extensions = {}, const char* initialPath = "A:/");

private:

	lv_obj_t* m_parent {nullptr};
	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_list {nullptr};
	lv_obj_t* m_pathLabel {nullptr};
	lv_obj_t* m_filenameInput {nullptr};
	lv_obj_t* m_actionBtn {nullptr};
	lv_obj_t* m_backBtn {nullptr};

	BackCallback m_onBack;
	FileSelectedCallback m_onFileSelected;
	std::string m_currentPath {"A:/data"};
	std::vector<std::string> m_extensions {};
	std::stack<std::string> m_history {};
	bool m_forSave {false};
	bool m_isModal {false};

	// Constructor for modal mode
	FileBrowser(FileSelectedCallback callback, const std::vector<std::string>& extensions, const char* path);

	void createUI();
	void createModalUI();
	void refreshList();
	void navigateUp();
	void goBack();
	void enterDirectory(const std::string& name);
	void selectFile(const std::string& name);
	void confirmSelection();
	void closeModal();

	static bool hasExtension(const std::string& fileName, const std::string& ext);
};

// ============================================================================
// Implementation (header-only for simplicity like other UI components)
// ============================================================================

// Full-screen mode constructor
inline FileBrowser::FileBrowser(lv_obj_t* parent, BackCallback onBack)
	: m_parent(parent), m_onBack(std::move(onBack)), m_isModal(false) {}

// Modal mode constructor
inline FileBrowser::FileBrowser(FileSelectedCallback callback, const std::vector<std::string>& extensions, const char* path)
	: m_parent(nullptr), m_onFileSelected(std::move(callback)), m_currentPath(path),
	  m_extensions(extensions), m_isModal(true) {}

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
	m_container = nullptr;
	m_list = nullptr;
	m_pathLabel = nullptr;
	m_filenameInput = nullptr;
	m_actionBtn = nullptr;
	m_backBtn = nullptr;
}

inline void FileBrowser::showModal(FileSelectedCallback callback, const std::vector<std::string>& extensions, const char* initialPath) {
	auto* chooser = new FileBrowser(callback, extensions, initialPath);
	chooser->createModalUI();
}

inline void FileBrowser::createUI() {
	using namespace Apps::Settings;

	m_container = create_page_container(m_parent);

	// Header with back button
	lv_obj_t* header = create_header(m_container, "Browse", &m_backBtn);
	add_back_button_event_cb(m_backBtn, &m_onBack);

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

inline void FileBrowser::createModalUI() {
	m_container = lv_obj_create(lv_layer_top());
	lv_obj_set_size(m_container, lv_pct(LayoutConstants::MODAL_WIDTH_PCT), lv_pct(LayoutConstants::FILE_DIALOG_HEIGHT_PCT));
	lv_obj_center(m_container);
	lv_obj_set_flex_flow(m_container, LV_FLEX_FLOW_COLUMN);

	// Header
	lv_obj_t* header = lv_obj_create(m_container);
	lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
	lv_obj_set_style_pad_all(header, 0, 0);
	lv_obj_set_style_border_width(header, 0, 0);
	lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);

	m_backBtn = lv_button_create(header);
	lv_obj_set_size(m_backBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* backLabel = lv_label_create(m_backBtn);
	lv_label_set_text(backLabel, LV_SYMBOL_LEFT);
	lv_obj_add_event_cb(m_backBtn, [](lv_event_t* e) {
		auto* self = static_cast<FileBrowser*>(lv_event_get_user_data(e));
		self->goBack(); }, LV_EVENT_CLICKED, this);

	m_pathLabel = lv_label_create(header);
	lv_obj_set_flex_grow(m_pathLabel, 1);
	lv_label_set_long_mode(m_pathLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);

	lv_obj_t* closeBtn = lv_button_create(header);
	lv_obj_set_size(closeBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_bg_color(closeBtn, lv_palette_main(LV_PALETTE_RED), 0);
	lv_obj_t* closeLabel = lv_label_create(closeBtn);
	lv_label_set_text(closeLabel, LV_SYMBOL_CLOSE);
	lv_obj_add_event_cb(closeBtn, [](lv_event_t* e) {
		auto* self = static_cast<FileBrowser*>(lv_event_get_user_data(e));
		self->closeModal(); }, LV_EVENT_CLICKED, this);

	// File List
	m_list = lv_list_create(m_container);
	lv_obj_set_size(m_list, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(m_list, 1);

	refreshList();
}

inline void FileBrowser::refreshList() {
	if (!m_list) return;

	lv_obj_clean(m_list);
	lv_label_set_text(m_pathLabel, m_currentPath.c_str());

	// Update back button state for modal mode
	if (m_isModal && m_backBtn) {
		if (m_history.empty()) {
			lv_obj_add_state(m_backBtn, LV_STATE_DISABLED);
		} else {
			lv_obj_remove_state(m_backBtn, LV_STATE_DISABLED);
		}
	}

	// Add parent directory option if not at root (full-screen mode)
	if (!m_isModal && m_currentPath != "A:/" && m_currentPath != "A:/data") {
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
	size_t pos = m_currentPath.find_last_of('/');
	if (pos != std::string::npos && pos > 2) {
		m_currentPath = m_currentPath.substr(0, pos);
	}
	refreshList();
}

inline void FileBrowser::goBack() {
	if (!m_history.empty()) {
		m_currentPath = m_history.top();
		m_history.pop();
		refreshList();
	}
}

inline void FileBrowser::enterDirectory(const std::string& name) {
	if (m_isModal) {
		m_history.push(m_currentPath);
	}
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
		if (m_isModal) {
			closeModal();
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

inline void FileBrowser::closeModal() {
	if (m_container) {
		lv_obj_delete(m_container);
	}
	delete this;
}

inline bool FileBrowser::hasExtension(const std::string& fileName, const std::string& ext) {
	if (fileName.length() < ext.length()) return false;
	return fileName.compare(fileName.length() - ext.length(), ext.length(), ext) == 0;
}

} // namespace System::UI
