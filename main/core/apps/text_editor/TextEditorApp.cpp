#include "TextEditorApp.hpp"
#include "misc/lv_style.h"
#include <cstdio>
#include <cstring>
#include <flx/core/Logger.hpp>
#include <flx/system/services/FileSystemService.hpp>
#include <flx/ui/common/SettingsCommon.hpp>

using namespace flx::apps;
using namespace flx::ui::common;

namespace System::Apps {

const AppManifest TextEditorApp::manifest = {
	.appId = "com.flxos.texteditor",
	.appName = "Text Editor",
	.appIcon = LV_SYMBOL_EDIT,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::User,
	.flags = AppFlags::None,
	.location = AppLocation::internal(),
	.description = "Create and edit text files",
	.sortPriority = 40,
	.capabilities = AppCapability::Storage,
	.supportedMimeTypes = {"text/plain", "text/*"},
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<TextEditorApp>(); }
};

static constexpr const char* TAG = "TextEditorApp";
static constexpr size_t MAX_FILE_SIZE = 32 * 1024; // 32KB max file size

bool TextEditorApp::onStart() {
	Log::info(TAG, "Text Editor app started");
	return true;
}

bool TextEditorApp::onResume() {
	Log::info(TAG, "Text Editor app resumed");
	updateStatusBar();
	return true;
}

void TextEditorApp::onPause() {
	Log::info(TAG, "Text Editor app paused");
}

void TextEditorApp::onStop() {
	Log::info(TAG, "Text Editor app stopped");
	if (m_fileBrowser) {
		m_fileBrowser->destroy();
	}
	m_fileBrowser.reset();
	m_container = nullptr;
	m_editorScreen = nullptr;
	m_toolbar = nullptr;
	m_textarea = nullptr;
	m_viewContainer = nullptr;
	m_viewLabel = nullptr;
	m_statusBar = nullptr;
	m_lineCountLabel = nullptr;
	m_fileNameLabel = nullptr;
	m_editSwitch = nullptr;
	m_optionsDropdown = nullptr;
	m_isDirty = false;
	m_editMode = false;
	m_currentFilePath.clear();
}

void TextEditorApp::update() {
	// No periodic updates needed
}

void TextEditorApp::createUI(void* parent) {
	m_container = static_cast<lv_obj_t*>(parent);

	// Create file browser component
	m_fileBrowser = std::make_unique<flx::ui::FileBrowser>(
		m_container, [this]() { showEditorScreen(); }
	);
	m_fileBrowser->setExtensions({".txt", ".md", ".log", ".json", ".xml", ".csv", ".yaml", ".yml", ".ini", ".cfg", ".conf", ".env", ".html", ".css", ".js", ".ts", ".py", ".java", ".c", ".cpp", ".h", ".hpp", ".cs", ".php", ".rb", ".go", ".rs", ".sh", ".bat", ".ps1", ".sql"});

	// Create and show editor screen
	createEditorScreen();
	showEditorScreen();
	setEditMode(true); // Default to edit mode

	// If launched via Intent with a file path, auto-load it
	if (m_context && !m_context->getData().empty()) {
		std::string filePath = m_context->getData();
		Log::info(TAG, "Loading file from intent: %s", filePath.c_str());
		loadFile(filePath);
	}
}

void TextEditorApp::createEditorScreen() {
	m_editorScreen = create_page_container(m_container);

	// Create toolbar at top
	createToolbar(m_editorScreen);

	// Create main text area
	createTextArea(m_editorScreen);

	// Create status bar at bottom
	createStatusBar(m_editorScreen);
}

void TextEditorApp::showEditorScreen() {
	if (m_fileBrowser) {
		m_fileBrowser->hide();
	}
	if (m_editorScreen) {
		lv_obj_remove_flag(m_editorScreen, LV_OBJ_FLAG_HIDDEN);
	}
}

void TextEditorApp::showFileBrowser(bool forSave) {
	if (m_editorScreen) {
		lv_obj_add_flag(m_editorScreen, LV_OBJ_FLAG_HIDDEN);
	}
	if (m_fileBrowser) {
		std::string defaultFilename = "untitled.txt";
		if (forSave && !m_currentFilePath.empty()) {
			size_t pos = m_currentFilePath.find_last_of('/');
			defaultFilename = (pos != std::string::npos) ? m_currentFilePath.substr(pos + 1) : m_currentFilePath;
		}

		m_fileBrowser->show(forSave, [this, forSave](const std::string& vfsPath) { onFileSelected(vfsPath, forSave); }, defaultFilename);
	}
}

void TextEditorApp::onFileSelected(const std::string& vfsPath, bool forSave) {
	if (forSave) {
		saveFile(vfsPath);
	} else {
		loadFile(vfsPath);
	}
	showEditorScreen();
}

void TextEditorApp::createToolbar(lv_obj_t* parent) {
	m_toolbar = lv_obj_create(parent);
	lv_obj_set_size(m_toolbar, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(m_toolbar, 0, 0);
	lv_obj_set_style_pad_gap(m_toolbar, 8, 0);
	lv_obj_set_style_border_width(m_toolbar, 0, 0);
	lv_obj_set_style_radius(m_toolbar, 0, 0);
	lv_obj_set_flex_flow(m_toolbar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(m_toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_remove_flag(m_toolbar, LV_OBJ_FLAG_SCROLLABLE);

	// Options Dropdown
	m_optionsDropdown = lv_dropdown_create(m_toolbar);
	lv_dropdown_set_text(m_optionsDropdown, "File");
	lv_dropdown_set_options(m_optionsDropdown, "New\nOpen\nSave"); // Initial options for Edit mode
	lv_obj_add_event_cb(m_optionsDropdown, onMenuOptionSelected, LV_EVENT_VALUE_CHANGED, this);

	// Edit mode switch with label
	lv_obj_t* editLabel = lv_label_create(m_toolbar);
	lv_label_set_text(editLabel, "Edit");

	m_editSwitch = lv_switch_create(m_toolbar);
	lv_obj_add_event_cb(m_editSwitch, onEditSwitchChanged, LV_EVENT_VALUE_CHANGED, this);
	lv_obj_add_state(m_editSwitch, LV_STATE_CHECKED); // Default: on (edit mode)
}

void TextEditorApp::createTextArea(lv_obj_t* parent) {
	// View container with label - shown in view-only mode (no cursor issues)
	m_viewContainer = lv_obj_create(parent);
	lv_obj_set_size(m_viewContainer, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(m_viewContainer, 1);
	lv_obj_set_style_radius(m_viewContainer, 0, 0);
	lv_obj_set_style_border_width(m_viewContainer, 0, 0);
	lv_obj_set_style_pad_all(m_viewContainer, 0, 0);

	m_viewLabel = lv_label_create(m_viewContainer);
	lv_label_set_text(m_viewLabel, "Open a file to view...");
	lv_obj_set_width(m_viewLabel, LV_PCT(100));
	lv_label_set_long_mode(m_viewLabel, LV_LABEL_LONG_MODE_WRAP);

	// Textarea - hidden by default, shown in edit mode
	m_textarea = lv_textarea_create(parent);
	lv_obj_set_size(m_textarea, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(m_textarea, 1);
	lv_obj_set_style_radius(m_textarea, 0, 0);
	lv_obj_set_style_border_width(m_textarea, 0, 0);
	lv_textarea_set_placeholder_text(m_textarea, "Start typing...");
	lv_textarea_set_one_line(m_textarea, false);
	lv_obj_add_flag(m_textarea, LV_OBJ_FLAG_HIDDEN);

	// Add text changed callback
	lv_obj_add_event_cb(m_textarea, onTextChanged, LV_EVENT_VALUE_CHANGED, this);
}

void TextEditorApp::createStatusBar(lv_obj_t* parent) {
	m_statusBar = lv_obj_create(parent);
	lv_obj_set_size(m_statusBar, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(m_statusBar, 0, 0);
	lv_obj_set_style_border_side(m_statusBar, LV_BORDER_SIDE_TOP, 0);
	lv_obj_set_style_border_width(m_statusBar, 1, 0);
	lv_obj_set_style_radius(m_statusBar, 0, 0);
	lv_obj_set_flex_flow(m_statusBar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(m_statusBar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_remove_flag(m_statusBar, LV_OBJ_FLAG_SCROLLABLE);

	// File name label (left side)
	m_fileNameLabel = lv_label_create(m_statusBar);
	lv_label_set_text(m_fileNameLabel, "New File");
	lv_label_set_long_mode(m_fileNameLabel, LV_LABEL_LONG_MODE_DOTS);
	lv_obj_set_flex_grow(m_fileNameLabel, 1);

	// Line count label (right side)
	m_lineCountLabel = lv_label_create(m_statusBar);
	lv_label_set_text(m_lineCountLabel, "L:0 C:0");
}

void TextEditorApp::updateStatusBar() {
	if (!m_textarea || !m_lineCountLabel) {
		return;
	}

	const char* text = lv_textarea_get_text(m_textarea);
	size_t charCount = strlen(text);

	// Count lines
	int lineCount = 1;
	for (size_t i = 0; i < charCount; i++) {
		if (text[i] == '\n') {
			lineCount++;
		}
	}

	// Update line/char count
	char statusText[32];
	snprintf(statusText, sizeof(statusText), "L:%d C:%zu%s", lineCount, charCount, m_isDirty ? " *" : "");
	lv_label_set_text(m_lineCountLabel, statusText);

	// Update file name
	if (m_fileNameLabel) {
		if (m_currentFilePath.empty()) {
			lv_label_set_text(m_fileNameLabel, m_isDirty ? "New File *" : "New File");
		} else {
			size_t pos = m_currentFilePath.find_last_of('/');
			std::string fileName = (pos != std::string::npos) ? m_currentFilePath.substr(pos + 1) : m_currentFilePath;
			if (m_isDirty) {
				fileName += " *";
			}
			lv_label_set_text(m_fileNameLabel, fileName.c_str());
		}
	}
}

bool TextEditorApp::loadFile(const std::string& vfsPath) {
	FILE* file = fopen(vfsPath.c_str(), "r");
	if (!file) {
		Log::error(TAG, "Failed to open file: %s", vfsPath.c_str());
		return false;
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (size > static_cast<long>(MAX_FILE_SIZE)) {
		fclose(file);
		Log::error(TAG, "File too large: %ld bytes", size);
		return false;
	}

	// Read file content
	char* buffer = new char[size + 1];
	size_t bytesRead = fread(buffer, 1, size, file);
	buffer[bytesRead] = '\0';
	fclose(file);

	// Update both view label and textarea
	if (m_viewLabel) {
		lv_label_set_text(m_viewLabel, buffer);
		lv_obj_scroll_to_y(m_viewContainer, 0, LV_ANIM_OFF);
	}
	if (m_textarea) {
		lv_textarea_set_text(m_textarea, buffer);
		lv_textarea_set_cursor_pos(m_textarea, 0);
	}
	delete[] buffer;

	m_currentFilePath = vfsPath;
	m_isDirty = false;
	updateStatusBar();

	Log::info(TAG, "Loaded file: %s (%ld bytes)", vfsPath.c_str(), size);
	return true;
}

bool TextEditorApp::saveFile(const std::string& vfsPath) {
	if (!m_textarea) {
		return false;
	}

	const char* text = lv_textarea_get_text(m_textarea);

	FILE* file = fopen(vfsPath.c_str(), "w");
	if (!file) {
		Log::error(TAG, "Failed to create file: %s", vfsPath.c_str());
		return false;
	}

	size_t len = strlen(text);
	size_t written = fwrite(text, 1, len, file);
	fclose(file);

	if (written != len) {
		Log::error(TAG, "Failed to write all data to file");
		return false;
	}

	m_currentFilePath = vfsPath;
	m_isDirty = false;
	updateStatusBar();

	Log::info(TAG, "Saved file: %s (%zu bytes)", vfsPath.c_str(), written);
	return true;
}

void TextEditorApp::onMenuOptionSelected(lv_event_t* e) {
	auto* app = static_cast<TextEditorApp*>(lv_event_get_user_data(e));
	if (!app) {
		return;
	}

	lv_obj_t* dropdown = lv_event_get_target_obj(e);
	char buf[32];
	lv_dropdown_get_selected_str(dropdown, buf, sizeof(buf));

	if (strcmp(buf, "New") == 0) {
		if (app->m_textarea) {
			lv_textarea_set_text(app->m_textarea, "");
			if (app->m_viewLabel) {
				lv_label_set_text(app->m_viewLabel, "");
			}
			app->m_currentFilePath.clear();
			app->m_isDirty = false;
			app->updateStatusBar();
			Log::info(TAG, "New document created");
		}
	} else if (strcmp(buf, "Open") == 0) {
		app->showFileBrowser(false);
	} else if (strcmp(buf, "Save") == 0) {
		if (app->m_currentFilePath.empty()) {
			// No file path, show save as dialog
			app->showFileBrowser(true);
		} else {
			// Save to existing path
			app->saveFile(app->m_currentFilePath);
		}
	}
}

void TextEditorApp::onTextChanged(lv_event_t* e) {
	auto* app = static_cast<TextEditorApp*>(lv_event_get_user_data(e));
	if (!app) {
		return;
	}

	app->m_isDirty = true;
	app->updateStatusBar();
}

void TextEditorApp::onEditSwitchChanged(lv_event_t* e) {
	auto* app = static_cast<TextEditorApp*>(lv_event_get_user_data(e));
	if (!app) {
		return;
	}

	bool enabled = lv_obj_has_state(app->m_editSwitch, LV_STATE_CHECKED);
	app->setEditMode(enabled);
}

void TextEditorApp::setEditMode(bool enabled) {
	m_editMode = enabled;

	// Switch between view container (label) and textarea
	if (enabled) {
		// Edit mode: show textarea, hide view container
		if (m_viewContainer) {
			lv_obj_add_flag(m_viewContainer, LV_OBJ_FLAG_HIDDEN);
		}
		if (m_textarea) {
			lv_obj_remove_flag(m_textarea, LV_OBJ_FLAG_HIDDEN);
		}
	} else {
		// View mode: show view container, hide textarea
		if (m_textarea) {
			lv_obj_add_flag(m_textarea, LV_OBJ_FLAG_HIDDEN);
			// Sync current textarea content to view label
			if (m_viewLabel) {
				lv_label_set_text(m_viewLabel, lv_textarea_get_text(m_textarea));
			}
		}
		if (m_viewContainer) {
			lv_obj_remove_flag(m_viewContainer, LV_OBJ_FLAG_HIDDEN);
		}
	}

	// Show/hide edit-only buttons in dropdown
	if (m_optionsDropdown) {
		if (enabled) {
			lv_dropdown_set_options(m_optionsDropdown, "New\nOpen\nSave");
		} else {
			lv_dropdown_set_options(m_optionsDropdown, "Open");
		}
		// Reset selection to first option to prevent stale index from previous mode
		lv_dropdown_set_selected(m_optionsDropdown, 0);
	}

	Log::info(TAG, "Edit mode: %s", enabled ? "enabled" : "disabled");
}

} // namespace System::Apps
