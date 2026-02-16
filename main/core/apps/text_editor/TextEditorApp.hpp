#pragma once

#include "lvgl.h"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/ui/components/FileBrowser.hpp>
#include <functional>
#include <memory>
#include <string>

namespace System::Apps {

using flx::apps::AppManifest;

class TextEditorApp : public flx::apps::App {
public:

	TextEditorApp() = default;
	~TextEditorApp() override = default;

	bool onStart() override;
	bool onResume() override;
	void onPause() override;
	void createUI(void* parent) override;
	void onStop() override;
	void update() override;

	std::string getPackageName() const override { return "com.flxos.texteditor"; }
	std::string getAppName() const override { return "Text Editor"; }
	const void* getIcon() const override { return LV_SYMBOL_EDIT; }

	static const AppManifest manifest;

private:

	lv_obj_t* m_container {nullptr};

	// Editor screen
	lv_obj_t* m_editorScreen {nullptr};
	lv_obj_t* m_toolbar {nullptr};
	lv_obj_t* m_textarea {nullptr};
	lv_obj_t* m_viewContainer {nullptr}; // Scrollable container for view mode
	lv_obj_t* m_viewLabel {nullptr}; // Label for view mode (no cursor issues)
	lv_obj_t* m_statusBar {nullptr};
	lv_obj_t* m_lineCountLabel {nullptr};
	lv_obj_t* m_fileNameLabel {nullptr};

	// File browser component
	std::unique_ptr<flx::ui::FileBrowser> m_fileBrowser;

	std::string m_currentFilePath; // VFS path (e.g., "/data/file.txt")
	bool m_isDirty {false};
	bool m_editMode {true}; // Default: edit mode

	// Edit mode controls
	lv_obj_t* m_editSwitch {nullptr};
	lv_obj_t* m_optionsDropdown {nullptr};

	void showEditorScreen();
	void showFileBrowser(bool forSave);
	void onFileSelected(const std::string& vfsPath, bool forSave);

	void createEditorScreen();
	void createToolbar(lv_obj_t* parent);
	void createTextArea(lv_obj_t* parent);
	void createStatusBar(lv_obj_t* parent);
	void updateStatusBar();

	// File operations
	bool loadFile(const std::string& vfsPath);
	bool saveFile(const std::string& vfsPath);

	static void onMenuOptionSelected(lv_event_t* e);
	static void onTextChanged(lv_event_t* e);
	static void onEditSwitchChanged(lv_event_t* e);

	void setEditMode(bool enabled);
};

} // namespace System::Apps
