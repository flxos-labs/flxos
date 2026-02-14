#pragma once

#include "core/apps/AppManager.hpp"
#include "core/apps/AppManifest.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include <flx/core/ClipboardManager.hpp>
#include "core/services/filesystem/FileSystemService.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace System::Apps {

class FilesApp : public App {
public:

	std::string getPackageName() const override;
	std::string getAppName() const override;
	const void* getIcon() const override;

	static const AppManifest manifest;

	void createUI(void* parent) override;
	void onStop() override;

private:

	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_page = nullptr;
	lv_obj_t* m_header = nullptr;
	lv_obj_t* m_backBtn = nullptr;
	lv_obj_t* m_pasteBtn = nullptr;
	lv_obj_t* m_pathLabel = nullptr;
	lv_obj_t* m_list = nullptr;

	std::string m_currentPath;
	std::stack<std::string> m_history;
	flx::kernel::Task* m_guiTask = nullptr;
	uint32_t m_lastFeed = 0;

	lv_obj_t* m_progressMbox = nullptr;
	lv_obj_t* m_progressBar = nullptr;
	lv_obj_t* m_progressLabel = nullptr;

	void feedWatchdog();
	void showProgressDialog(const char* title);
	void updateProgress(int percent, const char* text);
	void closeProgressDialog();

	void refreshList();
	void addListItem(const std::string& name, bool isDir);
	void handleMenuAction(const std::string& action, const std::string& name, bool isDir);
	void showInputDialog(const char* title, const std::string& defaultVal, std::function<void(std::string)> cb);

	void pasteItem();
	void deleteItem(const std::string& name, bool isDir);
	void renameItem(const std::string& oldName, const std::string& newName);
	void createFolder(const std::string& name);
	void enterDir(const std::string& name);
	void goBack();
	void goHome();
	void onFileClick(const std::string& name);
};

} // namespace System::Apps
