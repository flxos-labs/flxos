#pragma once

#include "lvgl.h"
#include <algorithm>
#include <cctype>
#include <flx/system/services/FileSystemService.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
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

} // namespace System::UI
