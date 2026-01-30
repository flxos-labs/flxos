#pragma once

#include "core/apps/AppManager.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include "core/common/ClipboardManager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace System {
namespace Apps {

class FilesApp : public App {
public:

	std::string getPackageName() const override;
	std::string getAppName() const override;
	const void* getIcon() const override;

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
	Task* m_guiTask = nullptr;
	uint32_t m_lastFeed = 0;

	lv_obj_t* m_progressMbox = nullptr;
	lv_obj_t* m_progressBar = nullptr;
	lv_obj_t* m_progressLabel = nullptr;

	void feedWatchdog();
	void showProgressDialog(const char* title);
	void updateProgress(int percent, const char* text);
	void closeProgressDialog();

	std::string toVfsPath(const std::string& lvPath);

	void refreshList();
	void addListItem(const std::string& name, bool isDir);
	void handleMenuAction(const std::string& action, const std::string& name, bool isDir);
	void showInputDialog(const char* title, const std::string& defaultVal, std::function<void(std::string)> cb);

	int copyFile(const char* src, const char* dst);
	int copyRecursive(const char* src, const char* dst);
	void pasteItem();
	int removeRecursive(const char* path);
	void deleteItem(const std::string& name, bool isDir);
	void renameItem(const std::string& oldName, const std::string& newName);
	void createFolder(const std::string& name);
	void enterDir(const std::string& name);
	void goBack();
	void goHome();
	void onFileClick(const std::string& name);
};

} // namespace Apps
} // namespace System
