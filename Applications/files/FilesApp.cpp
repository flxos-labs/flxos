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
#include "layouts/flex/lv_flex.h"
#include "misc/lv_anim.h"
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

#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/core/ClipboardManager.hpp>
#include <flx/core/Logger.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <flx/system/services/FileSystemService.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string_view>

static constexpr std::string_view TAG = "FilesApp";

using namespace flx::apps;
using namespace flx::ui::common;
using flx::services::FileSystemService;

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────
namespace {

constexpr std::string_view ROOT_PATH = "A:/";
constexpr uint32_t WATCHDOG_MS = 100;
constexpr uint32_t REFRESH_UI_MS = 50;
constexpr size_t FILENAME_BUFSZ = 32;

/// Supported file-to-MIME mappings (checked by extension).
struct MimeEntry {
	std::string_view ext;
	std::string_view mime;
};
constexpr MimeEntry MIME_TABLE[] = {
	{"png", "image/png"},
	{"jpg", "image/jpeg"},
	{"jpeg", "image/jpeg"},
	{"txt", "text/plain"},
	{"log", "text/plain"},
	{"json", "text/plain"},
	{"csv", "text/plain"},
	{"md", "text/plain"},
};

/// Returns the MIME type for a filename, or empty string_view if unknown.
std::string_view mimeForFilename(const std::string& name) {
	const auto dot = name.rfind('.');
	if (dot == std::string::npos) return {};

	std::string ext = name.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

	for (const auto& entry: MIME_TABLE) {
		if (entry.ext == ext) return entry.mime;
	}
	return {};
}

/// Return only the filename portion of a path (after the last '/').
const char* basenameOf(const char* path) {
	const char* slash = std::strrchr(path, '/');
	return slash ? slash + 1 : path;
}

// ── Modal helpers ─────────────────────────────────────────────────────────────

lv_obj_t* createModal(int widthPct, int maxHeightPct) {
	lv_obj_t* mb = lv_msgbox_create(nullptr);
	lv_obj_set_size(mb, lv_pct(widthPct), LV_SIZE_CONTENT);
	lv_obj_set_style_max_height(mb, lv_pct(maxHeightPct), 0);
	lv_obj_center(mb);
	return mb;
}

void showMsgBox(const char* title, const char* text) {
	lv_obj_t* mb = createModal(LayoutConstants::MODAL_WIDTH_PCT, LayoutConstants::FILE_DIALOG_HEIGHT_PCT);
	lv_msgbox_add_title(mb, title);
	lv_msgbox_add_text(mb, text);
	lv_msgbox_add_close_button(mb);
}

/// Returns current time in milliseconds (from esp_timer).
inline uint32_t nowMs() {
	return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Manifest
// ─────────────────────────────────────────────────────────────────────────────
namespace System::Apps {

const flx::apps::AppManifest FilesApp::manifest = {
	.appId = "com.flxos.files",
	.appName = "Files",
	.appIcon = LV_SYMBOL_DIRECTORY,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = flx::apps::AppCategory::System,
	.flags = flx::apps::AppFlags::None,
	.location = flx::apps::AppLocation::internal(),
	.description = "Browse and manage files on device storage",
	.sortPriority = 20,
	.capabilities = flx::apps::AppCapability::Storage,
	.requiredServices = {},
	.supportedMimeTypes = {},
	.urlSchemes = {},
	.createApp = []() -> std::shared_ptr<flx::apps::App> {
		return std::make_shared<FilesApp>();
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// App identity
// ─────────────────────────────────────────────────────────────────────────────

std::string FilesApp::getPackageName() const { return "com.flxos.files"; }
std::string FilesApp::getAppName() const { return "Files"; }
const void* FilesApp::getIcon() const { return LV_SYMBOL_DIRECTORY; }

// ─────────────────────────────────────────────────────────────────────────────
// UI lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::createUI(void* parent) {
	m_container = static_cast<lv_obj_t*>(parent);
	m_page = create_page_container(m_container);

	// ── Header ───────────────────────────────────────────────────────────────
	lv_obj_t* backBtn = nullptr;
	m_header = create_header(m_page, "", &backBtn);
	m_backBtn = backBtn;

	addHeaderButton(LV_SYMBOL_HOME, [this]() { goHome(); });
	addHeaderButton(LV_SYMBOL_PLUS, [this]() {
		showInputDialog("New Folder", "", [this](const std::string& name) {
			createFolder(name);
		});
	});

	m_pasteBtn = addHeaderButton(LV_SYMBOL_PASTE, [this]() { pasteItem(); });

	m_pathLabel = lv_label_create(m_header);
	lv_obj_set_flex_grow(m_pathLabel, 1);
	lv_label_set_long_mode(m_pathLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);

	lv_obj_add_event_cb(m_backBtn, [](lv_event_t* e) { static_cast<FilesApp*>(lv_event_get_user_data(e))->goBack(); }, LV_EVENT_CLICKED, this);

	// ── File list ─────────────────────────────────────────────────────────────
	m_list = create_settings_list(m_page);

	if (m_currentPath.empty()) {
		m_currentPath = ROOT_PATH;
	}

	Log::info(TAG, "UI created, navigating to: %s", m_currentPath.c_str());
	refreshList();
}

void FilesApp::onStop() {
	// Null out all widget pointers — LVGL owns the memory.
	m_container = m_page = m_header = m_backBtn = m_pasteBtn = m_pathLabel = m_list = nullptr;
	m_progressMbox = m_progressBar = m_progressLabel = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Header button factory
// ─────────────────────────────────────────────────────────────────────────────

lv_obj_t* FilesApp::addHeaderButton(const char* symbol, std::function<void()> onClick) {
	lv_obj_t* btn = lv_button_create(m_header);
	lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

	lv_obj_t* img = lv_image_create(btn);
	lv_image_set_src(img, symbol);
	lv_obj_center(img);

	// Heap-allocate the callback so the lambda survives past this scope.
	// Freed via LV_EVENT_DELETE on the button.
	auto* cb = new std::function<void()>(std::move(onClick));
	lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            auto* fn = static_cast<std::function<void()>*>(lv_event_get_user_data(e));
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                (*fn)();
            } else if (lv_event_get_code(e) == LV_EVENT_DELETE) {
                delete fn;
            } }, LV_EVENT_ALL, cb);

	return btn;
}

// ─────────────────────────────────────────────────────────────────────────────
// Watchdog
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::feedWatchdog() {
	const uint32_t now = nowMs();
	if (now - m_lastFeedMs < WATCHDOG_MS) return;

	if (!m_guiTask) {
		m_guiTask = flx::kernel::TaskManager::getInstance().getTask("gui_task");
	}
	if (m_guiTask) {
		m_guiTask->heartbeat();
	}
	vTaskDelay(pdMS_TO_TICKS(1));
	m_lastFeedMs = now;
}

// ─────────────────────────────────────────────────────────────────────────────
// Progress dialog
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::showProgressDialog(const char* title) {
	m_progressMbox = createModal(LayoutConstants::MODAL_WIDTH_PCT, LayoutConstants::FILE_DIALOG_HEIGHT_PCT);
	lv_msgbox_add_title(m_progressMbox, title);

	lv_obj_t* cont = lv_msgbox_get_content(m_progressMbox);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

	m_progressLabel = lv_label_create(cont);
	lv_obj_set_width(m_progressLabel, lv_pct(100));
	lv_label_set_long_mode(m_progressLabel, LV_LABEL_LONG_DOT);
	lv_label_set_text(m_progressLabel, "Initialising…");

	m_progressBar = lv_bar_create(cont);
	lv_obj_set_width(m_progressBar, lv_pct(100));
	lv_bar_set_range(m_progressBar, 0, 100);
	lv_bar_set_value(m_progressBar, 0, LV_ANIM_OFF);

	lv_refr_now(nullptr);
}

void FilesApp::updateProgress(int percent, const char* path) {
	if (m_progressBar) {
		lv_bar_set_value(m_progressBar, percent, LV_ANIM_OFF);
	}
	if (m_progressLabel && path) {
		lv_label_set_text(m_progressLabel, basenameOf(path));
	}

	const uint32_t now = nowMs();
	if (now - m_lastRefreshMs >= REFRESH_UI_MS) {
		lv_refr_now(nullptr);
		m_lastRefreshMs = now;
	}
}

void FilesApp::closeProgressDialog() {
	if (m_progressMbox) {
		lv_msgbox_close(m_progressMbox);
		m_progressMbox = m_progressBar = m_progressLabel = nullptr;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Directory listing
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::refreshList() {
	if (!m_list) return;

	lv_obj_clean(m_list);
	lv_label_set_text(m_pathLabel, m_currentPath.c_str());

	const bool atRoot = (m_currentPath == ROOT_PATH || m_currentPath == "A:");
	if (atRoot) {
		lv_obj_add_state(m_backBtn, LV_STATE_DISABLED);
	} else {
		lv_obj_remove_state(m_backBtn, LV_STATE_DISABLED);
	}

	if (flx::ClipboardManager::getInstance().hasContent()) {
		lv_obj_remove_flag(m_pasteBtn, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(m_pasteBtn, LV_OBJ_FLAG_HIDDEN);
	}

	Log::info(TAG, "Listing: %s", m_currentPath.c_str());
	const auto entries = FileSystemService::getInstance().listDirectory(m_currentPath);
	Log::info(TAG, "Found %zu entries", entries.size());

	if (entries.empty()) {
		lv_list_add_text(m_list, "Empty directory");
		return;
	}

	for (const auto& entry: entries) {
		feedWatchdog();
		addListItem(entry.name, entry.isDirectory);
	}
}

void FilesApp::addListItem(const std::string& name, bool isDir) {
	const char* symbol = isDir ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;
	lv_obj_t* btn = lv_list_add_button(m_list, symbol, name.c_str());
	lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<uintptr_t>(isDir)));

	// ── Dropdown action menu ──────────────────────────────────────────────────
	lv_obj_t* dd = lv_dropdown_create(btn);
	lv_obj_set_size(dd, lv_dpx(LayoutConstants::SIZE_DROPDOWN_BTN_WIDTH), lv_dpx(LayoutConstants::SIZE_DROPDOWN_HEIGHT));
	lv_obj_add_flag(dd, LV_OBJ_FLAG_FLOATING);
	lv_obj_align(dd, LV_ALIGN_RIGHT_MID, 0, 0);

	lv_dropdown_set_options(dd, "Copy\nCut\nRename\nDelete\nInfo");
	lv_dropdown_set_text(dd, LV_SYMBOL_BARS);
	lv_dropdown_set_symbol(dd, nullptr);
	lv_dropdown_set_selected_highlight(dd, false);
	lv_dropdown_set_dir(dd, LV_DIR_LEFT);

	lv_obj_set_style_bg_opa(dd, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(dd, 0, 0);
	lv_obj_set_style_shadow_width(dd, 0, 0);

	lv_obj_add_event_cb(dd, [](lv_event_t* e) {
            const lv_event_code_t code = lv_event_get_code(e);
            if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
                lv_event_stop_bubbling(e);
                return;
            }
            if (code != LV_EVENT_VALUE_CHANGED) return;

            auto* app     = static_cast<FilesApp*>(lv_event_get_user_data(e));
            lv_obj_t* obj = lv_event_get_target_obj(e);

            char actionBuf[FILENAME_BUFSZ];
            lv_dropdown_get_selected_str(obj, actionBuf, sizeof(actionBuf));

            lv_obj_t* listBtn = lv_obj_get_parent(obj);
            const bool   entryIsDir = static_cast<bool>(
                reinterpret_cast<uintptr_t>(lv_obj_get_user_data(listBtn)));
            const char* entryName = lv_list_get_button_text(app->m_list, listBtn);

            app->handleMenuAction(actionBuf, entryName, entryIsDir); }, LV_EVENT_ALL, this);

	// ── Primary tap: navigate or open ─────────────────────────────────────────
	lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

            auto* app     = static_cast<FilesApp*>(lv_event_get_user_data(e));
            lv_obj_t* b   = lv_event_get_target_obj(e);
            const bool entryIsDir = static_cast<bool>(
                reinterpret_cast<uintptr_t>(lv_obj_get_user_data(b)));
            const char* entryName = lv_list_get_button_text(app->m_list, b);

            if (entryIsDir) {
                app->enterDir(entryName);
            } else {
                app->onFileClick(entryName);
            } }, LV_EVENT_ALL, this);
}

// ─────────────────────────────────────────────────────────────────────────────
// Context menu actions
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::handleMenuAction(const std::string& action, const std::string& name, bool isDir) {
	const std::string nativeBase = FileSystemService::toNativePath(m_currentPath);
	const std::string fullPath = FileSystemService::joinPath(nativeBase, name);

	if (action == "Info") {
		showMsgBox("File Info", fullPath.c_str());
		return;
	}

	if (action == "Copy") {
		flx::ClipboardManager::getInstance().set(fullPath, isDir, flx::ClipboardOp::COPY);
		refreshList();
		return;
	}

	if (action == "Cut") {
		flx::ClipboardManager::getInstance().set(fullPath, isDir, flx::ClipboardOp::CUT);
		refreshList();
		return;
	}

	if (action == "Rename") {
		showInputDialog("Rename", name, [this, name](const std::string& newName) {
			renameItem(name, newName);
		});
		return;
	}

	if (action == "Delete") {
		showDeleteConfirm(name, isDir);
		return;
	}

	Log::warn(TAG, "Unknown menu action: %s", action.c_str());
}

void FilesApp::showDeleteConfirm(const std::string& name, bool isDir) {
	lv_obj_t* confirm = createModal(LayoutConstants::MODAL_WIDTH_PCT, LayoutConstants::FILE_DIALOG_HEIGHT_PCT);
	lv_msgbox_add_title(confirm, "Delete?");
	lv_msgbox_add_text(confirm, ("Delete " + name + "?").c_str());

	lv_obj_t* btnYes = lv_msgbox_add_footer_button(confirm, "Yes");
	lv_msgbox_add_close_button(confirm);

	struct Ctx {
		FilesApp* app;
		std::string name;
		bool isDir;
		lv_obj_t* mbox;
	};
	auto* ctx = new Ctx {this, name, isDir, confirm};

	lv_obj_add_event_cb(btnYes, [](lv_event_t* e) {
            auto* c = static_cast<Ctx*>(lv_event_get_user_data(e));
            c->app->deleteItem(c->name, c->isDir);
            lv_msgbox_close(c->mbox); }, LV_EVENT_CLICKED, ctx);

	lv_obj_add_event_cb(confirm, [](lv_event_t* e) {
            if (lv_event_get_code(e) == LV_EVENT_DELETE) {
                delete static_cast<Ctx*>(lv_event_get_user_data(e));
            } }, LV_EVENT_DELETE, ctx);

	Log::info(TAG, "Prompting delete for: %s", name.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Input dialog
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::showInputDialog(const char* title, const std::string& defaultVal, std::function<void(std::string)> onConfirm) {
	lv_obj_t* mbox = createModal(LayoutConstants::MODAL_WIDTH_PCT, LayoutConstants::FILE_DIALOG_HEIGHT_PCT);
	lv_msgbox_add_title(mbox, title);

	lv_obj_t* ta = lv_textarea_create(lv_msgbox_get_content(mbox));
	lv_obj_set_width(ta, lv_pct(100));
	lv_textarea_set_text(ta, defaultVal.c_str());
	lv_textarea_set_one_line(ta, true);
	lv_obj_add_state(ta, LV_STATE_FOCUSED);

	lv_obj_t* btnOk = lv_msgbox_add_footer_button(mbox, "OK");
	lv_msgbox_add_close_button(mbox);

	struct Ctx {
		std::function<void(std::string)> cb;
		lv_obj_t* ta;
		lv_obj_t* mbox;
	};
	auto* ctx = new Ctx {std::move(onConfirm), ta, mbox};

	lv_obj_add_event_cb(btnOk, [](lv_event_t* e) {
            auto* c = static_cast<Ctx*>(lv_event_get_user_data(e));
            c->cb(lv_textarea_get_text(c->ta));
            lv_msgbox_close(c->mbox); }, LV_EVENT_CLICKED, ctx);

	lv_obj_add_event_cb(mbox, [](lv_event_t* e) {
            if (lv_event_get_code(e) == LV_EVENT_DELETE) {
                delete static_cast<Ctx*>(lv_event_get_user_data(e));
            } }, LV_EVENT_DELETE, ctx);
}

// ─────────────────────────────────────────────────────────────────────────────
// File operations
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::pasteItem() {
	auto& clipboard = flx::ClipboardManager::getInstance();
	if (!clipboard.hasContent()) return;

	const auto& clip = clipboard.get();
	const std::string srcPath = clip.path;
	const std::string name = srcPath.substr(srcPath.find_last_of('/') + 1);
	const std::string dstBase = FileSystemService::toNativePath(m_currentPath);
	const std::string dstPath = FileSystemService::joinPath(dstBase, name);

	if (srcPath == dstPath) {
		showMsgBox("Error", "Source and destination are the same.");
		return;
	}

	auto progress = [this](int pct, std::string_view path) {
		updateProgress(pct, path.data());
		feedWatchdog();
	};

	if (clip.op == flx::ClipboardOp::CUT) {
		if (!FileSystemService::getInstance().move(srcPath, dstPath)) {
			showMsgBox("Error", "Could not move item.");
		}
		clipboard.clear();
	} else {
		showProgressDialog("Copying");
		if (!FileSystemService::getInstance().copy(srcPath, dstPath, progress)) {
			showMsgBox("Error", "Copy failed.");
		}
		closeProgressDialog();
	}

	refreshList();
}

void FilesApp::deleteItem(const std::string& name, bool /*isDir*/) {
	const std::string fullPath = FileSystemService::joinPath(
		FileSystemService::toNativePath(m_currentPath), name
	);

	showProgressDialog("Deleting");

	const bool ok = FileSystemService::getInstance().remove(fullPath, [this](int pct, std::string_view path) {
		updateProgress(pct, path.data());
		feedWatchdog();
	});

	closeProgressDialog();

	if (!ok) showMsgBox("Error", "Could not delete item.");
	refreshList();
}

void FilesApp::renameItem(const std::string& oldName, const std::string& newName) {
	if (newName.empty() || oldName == newName) return;

	const std::string base = FileSystemService::toNativePath(m_currentPath);
	const std::string oldPath = FileSystemService::joinPath(base, oldName);
	const std::string newPath = FileSystemService::joinPath(base, newName);

	Log::info(TAG, "Renaming '%s' → '%s'", oldName.c_str(), newName.c_str());

	if (!FileSystemService::getInstance().move(oldPath, newPath)) {
		showMsgBox("Error", "Could not rename item.");
	}
	refreshList();
}

void FilesApp::createFolder(const std::string& name) {
	if (name.empty()) return;

	const std::string fullPath = FileSystemService::joinPath(
		FileSystemService::toNativePath(m_currentPath), name
	);

	if (!FileSystemService::getInstance().mkdir(fullPath)) {
		showMsgBox("Error", "Could not create folder.");
	}
	refreshList();
}

// ─────────────────────────────────────────────────────────────────────────────
// Navigation
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::enterDir(const std::string& name) {
	m_history.push(m_currentPath);
	m_currentPath = FileSystemService::joinPath(m_currentPath, name);
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
	while (!m_history.empty()) m_history.pop();
	m_currentPath = ROOT_PATH;
	refreshList();
}

// ─────────────────────────────────────────────────────────────────────────────
// File open
// ─────────────────────────────────────────────────────────────────────────────

void FilesApp::onFileClick(const std::string& name) {
	const std::string vfsPath = FileSystemService::joinPath(
		FileSystemService::toNativePath(m_currentPath), name
	);

	const std::string_view mime = mimeForFilename(name);
	if (!mime.empty()) {
		Intent intent = Intent::view(vfsPath, std::string {mime});
		flx::apps::AppManager::getInstance().startApp(intent);
		return;
	}

	// Unknown extension: show info.
	showMsgBox("File Info", vfsPath.c_str());
}

} // namespace System::Apps
