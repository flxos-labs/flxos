#include "FilesApp.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "core/lv_obj_tree.h"
#include "core/lv_refr.h"
#include "display/lv_display.h"
#include "esp_timer.h"
#include "font/lv_symbol_def.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "layouts/flex/lv_flex.h"
#include "lv_api_map_v9_1.h"
#include "misc/lv_anim.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_event.h"
#include "misc/lv_types.h"
#include "widgets/bar/lv_bar.h"
#include "widgets/button/lv_button.h"
#include "widgets/dropdown/lv_dropdown.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include "widgets/list/lv_list.h"
#include "widgets/msgbox/lv_msgbox.h"
#include "widgets/textarea/lv_textarea.h"
#include <cstdint>
#include <cstring>
#include <flx/core/ClipboardManager.hpp>
#include <flx/core/Logger.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <flx/system/services/FileSystemService.hpp>
#include <flx/ui/app/AppManager.hpp>
#include <flx/ui/app/AppManifest.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <string_view>

static constexpr std::string_view TAG = "FilesApp";

using namespace flx::app;

using namespace flx::ui::common;

namespace System::Apps {

const flx::app::AppManifest FilesApp::manifest = {
	.appId = "com.flxos.files",
	.appName = "Files",
	.appIcon = LV_SYMBOL_DIRECTORY,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = flx::app::AppCategory::System,
	.flags = flx::app::AppFlags::None,
	.location = flx::app::AppLocation::internal(),
	.description = "Browse and manage files on device storage",
	.sortPriority = 20,
	.capabilities = flx::app::AppCapability::Storage,
	.createApp = []() -> std::shared_ptr<flx::app::App> { return std::make_shared<FilesApp>(); }
};

// Helper: Create a styled message box
static void showMsgBox(const char* title, const char* text) {
	lv_obj_t* mb = lv_msgbox_create(nullptr);
	lv_obj_set_size(mb, lv_pct(LayoutConstants::MODAL_WIDTH_PCT), LV_SIZE_CONTENT);
	lv_obj_set_style_max_height(mb, lv_pct(LayoutConstants::FILE_DIALOG_HEIGHT_PCT), 0);
	lv_obj_center(mb);
	lv_msgbox_add_title(mb, title);
	lv_msgbox_add_text(mb, text);
	lv_msgbox_add_close_button(mb);
}

std::string FilesApp::getPackageName() const { return "com.flxos.files"; }
std::string FilesApp::getAppName() const { return "Files"; }
const void* FilesApp::getIcon() const { return LV_SYMBOL_DIRECTORY; }

void FilesApp::createUI(void* parent) {
	m_container = (lv_obj_t*)parent;

	m_page = create_page_container(m_container);

	lv_obj_t* backBtn = nullptr;
	m_header = create_header(m_page, "", &backBtn);
	m_backBtn = backBtn;

	// Add a Home button
	lv_obj_t* homeBtn = lv_button_create(m_header);
	lv_obj_set_size(homeBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* homeLabel = lv_image_create(homeBtn);
	lv_image_set_src(homeLabel, LV_SYMBOL_HOME);
	lv_obj_center(homeLabel);
	lv_obj_add_event_cb(
		homeBtn,
		[](lv_event_t* e) {
			auto* app = (FilesApp*)lv_event_get_user_data(e);
			app->goHome();
		},
		LV_EVENT_CLICKED, this
	);

	// Add a New Folder button
	lv_obj_t* mkdirBtn = lv_button_create(m_header);
	lv_obj_set_size(mkdirBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* mkdirLabel = lv_image_create(mkdirBtn);
	lv_image_set_src(mkdirLabel, LV_SYMBOL_PLUS);
	lv_obj_center(mkdirLabel);
	lv_obj_add_event_cb(
		mkdirBtn,
		[](lv_event_t* e) {
			auto* app = (FilesApp*)lv_event_get_user_data(e);
			app->showInputDialog("New Folder", "", [app](std::string name) {
				app->createFolder(name);
			});
		},
		LV_EVENT_CLICKED, this
	);

	// Add a Paste button
	m_pasteBtn = lv_button_create(m_header);
	lv_obj_set_size(m_pasteBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* pasteLabel = lv_image_create(m_pasteBtn);
	lv_image_set_src(pasteLabel, LV_SYMBOL_PASTE);
	lv_obj_center(pasteLabel);
	lv_obj_add_event_cb(
		m_pasteBtn,
		[](lv_event_t* e) {
			auto* app = (FilesApp*)lv_event_get_user_data(e);
			app->pasteItem();
		},
		LV_EVENT_CLICKED, this
	);

	m_pathLabel = lv_label_create(m_header);
	lv_obj_set_flex_grow(m_pathLabel, 1);
	lv_label_set_long_mode(m_pathLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);

	lv_obj_add_event_cb(
		m_backBtn,
		[](lv_event_t* e) {
			auto* app = (FilesApp*)lv_event_get_user_data(e);
			app->goBack();
		},
		LV_EVENT_CLICKED, this
	);

	m_list = create_settings_list(m_page);

	if (m_currentPath.empty()) {
		m_currentPath = "A:/";
	}
	Log::info(TAG, "UI created, starting at path: %s", m_currentPath.c_str());
	refreshList();
}

void FilesApp::onStop() {
	m_container = nullptr;
	m_page = nullptr;
	m_header = nullptr;
	m_backBtn = nullptr;
	m_pasteBtn = nullptr;
	m_pathLabel = nullptr;
	m_list = nullptr;
}

void FilesApp::feedWatchdog() {
	auto const now = (uint32_t)(esp_timer_get_time() / 1000);
	if (now - m_lastFeed >= 100) {
		if (!m_guiTask) {
			m_guiTask = flx::kernel::TaskManager::getInstance().getTask("gui_task");
		}
		if (m_guiTask) {
			m_guiTask->heartbeat();
		}
		vTaskDelay(pdMS_TO_TICKS(1));
		m_lastFeed = now;
	}
}

void FilesApp::showProgressDialog(const char* title) {
	m_progressMbox = lv_msgbox_create(nullptr);
	lv_obj_set_size(m_progressMbox, lv_pct(LayoutConstants::MODAL_WIDTH_PCT), LV_SIZE_CONTENT);
	lv_obj_set_style_max_height(m_progressMbox, lv_pct(LayoutConstants::FILE_DIALOG_HEIGHT_PCT), 0);
	lv_obj_center(m_progressMbox);
	lv_msgbox_add_title(m_progressMbox, title);

	lv_obj_t* cont = lv_msgbox_get_content(m_progressMbox);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

	m_progressLabel = lv_label_create(cont);
	lv_obj_set_width(m_progressLabel, lv_pct(100));
	lv_label_set_long_mode(m_progressLabel, LV_LABEL_LONG_DOT);
	lv_label_set_text(m_progressLabel, "Initialising...");

	m_progressBar = lv_bar_create(cont);
	lv_obj_set_width(m_progressBar, lv_pct(100));
	lv_bar_set_range(m_progressBar, 0, 100);
	lv_bar_set_value(m_progressBar, 0, LV_ANIM_OFF);

	lv_refr_now(nullptr);
}

void FilesApp::updateProgress(int percent, const char* text) {
	if (m_progressBar) {
		lv_bar_set_value(m_progressBar, percent, LV_ANIM_OFF);
	}
	if (m_progressLabel && text) {
		// Only show the filename part if it's a long path
		const char* name = strrchr(text, '/');
		lv_label_set_text(m_progressLabel, name ? name + 1 : text);
	}

	static uint32_t last_refr = 0;
	auto const now = (uint32_t)(esp_timer_get_time() / 1000);
	if (now - last_refr >= 50) {
		lv_refr_now(nullptr);
		last_refr = now;
	}
}

void FilesApp::closeProgressDialog() {
	if (m_progressMbox) {
		lv_msgbox_close(m_progressMbox);
		m_progressMbox = nullptr;
		m_progressBar = nullptr;
		m_progressLabel = nullptr;
	}
}

void FilesApp::refreshList() {
	if (!m_list) {
		return;
	}

	lv_obj_clean(m_list);
	lv_label_set_text(m_pathLabel, m_currentPath.c_str());

	if (m_currentPath == "A:/" || m_currentPath == "A:") {
		lv_obj_add_state(m_backBtn, LV_STATE_DISABLED);
	} else {
		lv_obj_remove_state(m_backBtn, LV_STATE_DISABLED);
	}

	// Update Paste button visibility
	if (flx::ClipboardManager::getInstance().hasContent()) {
		lv_obj_remove_flag(m_pasteBtn, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(m_pasteBtn, LV_OBJ_FLAG_HIDDEN);
	}

	// Use FileSystemService to list directory
	Log::info(TAG, "Listing directory: %s", m_currentPath.c_str());
	auto entries = flx::services::FileSystemService::getInstance().listDirectory(m_currentPath);
	Log::info(TAG, "Found %d entries", entries.size());

	if (entries.empty()) {
		lv_list_add_text(m_list, "Empty directory");
	} else {
		for (const auto& entry: entries) {
			feedWatchdog();
			addListItem(entry.name, entry.isDirectory);
		}
	}
}

void FilesApp::addListItem(const std::string& name, bool isDir) {
	const char* symbol = isDir ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;
	lv_obj_t* btn = lv_list_add_button(m_list, symbol, name.c_str());

	lv_obj_set_user_data(btn, (void*)(uintptr_t)isDir);

	// Add a dropdown menu on the right side
	lv_obj_t* dropdown = lv_dropdown_create(btn);
	lv_obj_set_size(dropdown, lv_dpx(LayoutConstants::SIZE_DROPDOWN_BTN_WIDTH), lv_dpx(LayoutConstants::SIZE_DROPDOWN_HEIGHT));
	lv_obj_add_flag(dropdown, LV_OBJ_FLAG_FLOATING);
	lv_obj_align(dropdown, LV_ALIGN_RIGHT_MID, 0, 0);

	lv_dropdown_set_options(dropdown, "Copy\nCut\nRename\nDelete\nInfo");
	lv_dropdown_set_text(dropdown, LV_SYMBOL_BARS);
	lv_dropdown_set_symbol(dropdown, nullptr);
	lv_dropdown_set_selected_highlight(dropdown, false);
	lv_dropdown_set_dir(dropdown, LV_DIR_LEFT);

	// Styling
	lv_obj_set_style_bg_opa(dropdown, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(dropdown, 0, 0);
	lv_obj_set_style_shadow_width(dropdown, 0, 0);

	lv_obj_add_event_cb(
		dropdown,
		[](lv_event_t* e) {
			lv_event_code_t const code = lv_event_get_code(e);
			if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
				lv_event_stop_bubbling(e);
			} else if (code == LV_EVENT_VALUE_CHANGED) {
				auto* app = (FilesApp*)lv_event_get_user_data(e);
				lv_obj_t* obj = lv_event_get_target_obj(e);

				char buf[32];
				lv_dropdown_get_selected_str(obj, buf, sizeof(buf));

				lv_obj_t* listBtn = lv_obj_get_parent(obj);
				bool isDir = (bool)(uintptr_t)lv_obj_get_user_data(listBtn);
				const char* name = lv_list_get_button_text(app->m_list, listBtn);

				app->handleMenuAction(buf, name, isDir);
			}
		},
		LV_EVENT_ALL, this
	);

	lv_obj_add_event_cb(
		btn,
		[](lv_event_t* e) {
			auto* app = (FilesApp*)lv_event_get_user_data(e);
			lv_obj_t* btn = lv_event_get_target_obj(e);
			lv_event_code_t const code = lv_event_get_code(e);

			if (code == LV_EVENT_CLICKED) {
				bool const isDir = (bool)(uintptr_t)lv_obj_get_user_data(btn);
				const char* name = lv_list_get_button_text(app->m_list, btn);
				if (isDir) {
					Log::debug(TAG, "User clicked directory: %s", name);
					app->enterDir(name);
				} else {
					Log::debug("FilesApp", "User clicked file: %s", name);
					app->onFileClick(name);
				}
			}
		},
		LV_EVENT_ALL, this
	);
}

void FilesApp::handleMenuAction(const std::string& action, const std::string& name, bool isDir) {
	std::string basePath = flx::services::FileSystemService::toVfsPath(m_currentPath);
	if (action == "Info") {
		std::string fullPath = flx::services::FileSystemService::buildPath(basePath, name);
		showMsgBox("File Info", fullPath.c_str());
	} else if (action == "Copy") {
		flx::ClipboardManager::getInstance().set(flx::services::FileSystemService::buildPath(basePath, name), isDir, flx::ClipboardOp::COPY);
		refreshList();
	} else if (action == "Cut") {
		flx::ClipboardManager::getInstance().set(flx::services::FileSystemService::buildPath(basePath, name), isDir, flx::ClipboardOp::CUT);
		refreshList();
	} else if (action == "Rename") {
		showInputDialog("Rename", name, [this, name](std::string newName) {
			this->renameItem(name, newName);
		});
	} else if (action == "Delete") {
		lv_obj_t* confirm = lv_msgbox_create(nullptr);
		lv_obj_set_size(confirm, lv_pct(LayoutConstants::MODAL_WIDTH_PCT), LV_SIZE_CONTENT);
		lv_obj_set_style_max_height(confirm, lv_pct(LayoutConstants::FILE_DIALOG_HEIGHT_PCT), 0);
		lv_obj_center(confirm);
		lv_msgbox_add_title(confirm, "Delete?");
		lv_msgbox_add_text(
			confirm, ("Are you sure you want to delete " + name + "?").c_str()
		);
		lv_obj_t* btn_yes = lv_msgbox_add_footer_button(confirm, "Yes");
		lv_msgbox_add_close_button(confirm);

		struct ConfirmCtx {
			FilesApp* app {};
			std::string name {};
			bool isDir {};
			lv_obj_t* mbox {};
		};
		auto* cctx = new ConfirmCtx {this, name, isDir, confirm};

		lv_obj_add_event_cb(
			btn_yes,
			[](lv_event_t* e) {
				auto* cc = (ConfirmCtx*)lv_event_get_user_data(e);
				cc->app->deleteItem(cc->name, cc->isDir);
				lv_msgbox_close(cc->mbox);
			},
			LV_EVENT_CLICKED, cctx
		);

		lv_obj_add_event_cb(
			confirm,
			[](lv_event_t* e) {
				if (lv_event_get_code(e) == LV_EVENT_DELETE) {
					delete (ConfirmCtx*)lv_event_get_user_data(e);
				}
			},
			LV_EVENT_DELETE, cctx
		);

		Log::info(TAG, "User requested delete for: %s", name.c_str());
	}
}

void FilesApp::showInputDialog(const char* title, const std::string& defaultVal, std::function<void(std::string)> cb) {
	lv_obj_t* mbox = lv_msgbox_create(nullptr);
	lv_obj_set_size(mbox, lv_pct(LayoutConstants::MODAL_WIDTH_PCT), LV_SIZE_CONTENT);
	lv_obj_set_style_max_height(mbox, lv_pct(LayoutConstants::FILE_DIALOG_HEIGHT_PCT), 0);
	lv_obj_center(mbox);
	lv_msgbox_add_title(mbox, title);

	lv_obj_t* ta = lv_textarea_create(lv_msgbox_get_content(mbox));
	lv_obj_set_width(ta, lv_pct(100));
	lv_textarea_set_text(ta, defaultVal.c_str());
	lv_textarea_set_one_line(ta, true);
	lv_obj_add_state(ta, LV_STATE_FOCUSED);

	lv_obj_t* btn_ok = lv_msgbox_add_footer_button(mbox, "OK");
	lv_msgbox_add_close_button(mbox);

	struct InputCtx {
		std::function<void(std::string)> cb {};
		lv_obj_t* ta {};
		lv_obj_t* mbox {};
	};
	auto* ctx = new InputCtx {cb, ta, mbox};

	lv_obj_add_event_cb(
		btn_ok,
		[](lv_event_t* e) {
			auto* c = (InputCtx*)lv_event_get_user_data(e);
			std::string val = lv_textarea_get_text(c->ta);
			c->cb(val);
			lv_msgbox_close(c->mbox);
		},
		LV_EVENT_CLICKED, ctx
	);

	lv_obj_add_event_cb(
		mbox,
		[](lv_event_t* e) {
			if (lv_event_get_code(e) == LV_EVENT_DELETE) {
				delete (InputCtx*)lv_event_get_user_data(e);
			}
		},
		LV_EVENT_DELETE, ctx
	);
}

void FilesApp::pasteItem() {
	auto& cb = flx::ClipboardManager::getInstance();
	if (!cb.hasContent()) {
		return;
	}

	std::string srcPath = cb.get().path;
	Log::info(TAG, "Pasting item from clipboard: %s", srcPath.c_str());
	std::string name = srcPath.substr(srcPath.find_last_of('/') + 1);
	std::string destBase = flx::services::FileSystemService::toVfsPath(m_currentPath);
	std::string destPath = flx::services::FileSystemService::buildPath(destBase, name);

	if (srcPath == destPath) {
		showMsgBox("Error", "Source and destination are the same.");
		return;
	}

	if (cb.get().op == flx::ClipboardOp::CUT) {
		if (!flx::services::FileSystemService::getInstance().move(srcPath, destPath)) {
			showMsgBox("Error", "Could not move item.");
		}
		cb.clear();
	} else {
		showProgressDialog("Copying");

		auto progressCb = [this](int percent, const char* path) {
			this->updateProgress(percent, path);
			this->feedWatchdog();
		};

		if (!flx::services::FileSystemService::getInstance().copy(srcPath, destPath, progressCb)) {
			showMsgBox("Error", "Copy failed.");
		}
		closeProgressDialog();
	}
	refreshList();
}

void FilesApp::deleteItem(const std::string& name, bool /*isDir*/) {
	std::string fullPath = flx::services::FileSystemService::buildPath(
		flx::services::FileSystemService::toVfsPath(m_currentPath), name
	);
	showProgressDialog("Deleting");

	auto progressCb = [this](int percent, const char* path) {
		this->updateProgress(percent, path);
		this->feedWatchdog();
	};

	bool success = flx::services::FileSystemService::getInstance().remove(fullPath, progressCb);
	closeProgressDialog();

	if (!success) {
		showMsgBox("Error", "Could not delete item.");
	}
	refreshList();
}

void FilesApp::renameItem(const std::string& oldName, const std::string& newName) {
	if (newName.empty() || oldName == newName) {
		return;
	}
	std::string basePath = flx::services::FileSystemService::toVfsPath(m_currentPath);
	std::string oldPath = flx::services::FileSystemService::buildPath(basePath, oldName);
	std::string newPath = flx::services::FileSystemService::buildPath(basePath, newName);
	Log::info(TAG, "Renaming item: %s -> %s", oldName.c_str(), newName.c_str());
	if (!flx::services::FileSystemService::getInstance().move(oldPath, newPath)) {
		showMsgBox("Error", "Could not rename item.");
	}
	refreshList();
}

void FilesApp::createFolder(const std::string& name) {
	if (name.empty()) {
		return;
	}
	std::string fullPath = flx::services::FileSystemService::buildPath(
		flx::services::FileSystemService::toVfsPath(m_currentPath), name
	);
	if (!flx::services::FileSystemService::getInstance().mkdir(fullPath)) {
		showMsgBox("Error", "Could not create folder.");
	}
	refreshList();
}

void FilesApp::enterDir(const std::string& name) {
	m_history.push(m_currentPath);
	m_currentPath = flx::services::FileSystemService::buildPath(m_currentPath, name);
	refreshList();
}

void FilesApp::goBack() {
	if (!m_history.empty()) {
		m_currentPath = m_history.top();
		m_history.pop();
		refreshList();
	}
}

void FilesApp::goHome() {
	while (!m_history.empty())
		m_history.pop();
	m_currentPath = "A:/";
	refreshList();
}

void FilesApp::onFileClick(const std::string& name) {
	std::string vfsPath = flx::services::FileSystemService::buildPath(
		flx::services::FileSystemService::toVfsPath(m_currentPath), name
	);

	// Check file extension
	auto dotPos = name.rfind('.');
	if (dotPos != std::string::npos) {
		std::string ext = name.substr(dotPos + 1);
		// Convert to lowercase
		for (auto& c: ext) c = static_cast<char>(tolower(c));

		if (ext == "png" || ext == "jpg" || ext == "jpeg") {
			std::string mimeType = "image/" + (ext == "jpg" ? std::string("jpeg") : ext);
			Intent intent = Intent::view(vfsPath, mimeType);
			flx::app::AppManager::getInstance().startApp(intent);
			return;
		}

		if (ext == "txt" || ext == "log" || ext == "json" || ext == "csv" || ext == "md") {
			Intent intent = Intent::view(vfsPath, "text/plain");
			flx::app::AppManager::getInstance().startApp(intent);
			return;
		}
	}

	showMsgBox("File Info", vfsPath.c_str());
}

} // namespace System::Apps
