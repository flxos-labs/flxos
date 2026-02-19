#include <flx/ui/components/FileBrowser.hpp>

#include "flx/ui/theming/ui_constants/UiConstants.hpp"
#include <flx/system/services/FileSystemService.hpp>
#include <flx/ui/common/SettingsCommon.hpp>

#include <algorithm>
#include <cctype>
#include <string_view>

using flx::services::FileSystemService;

namespace flx::ui {

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────
namespace {

constexpr std::string_view ROOT_PATH = "A:/";
constexpr std::string_view DEFAULT_FILENAME = "untitled.txt";

/// Case-insensitive suffix match (e.g. ext = ".txt").
bool hasExtension(const std::string& filename, const std::string& ext) {
	if (filename.size() < ext.size()) return false;

	const std::string suffix = filename.substr(filename.size() - ext.size());

	return std::equal(suffix.begin(), suffix.end(), ext.begin(), ext.end(), [](unsigned char a, unsigned char b) {
		return std::tolower(a) == std::tolower(b);
	});
}

/// Returns true if `filename` passes the (possibly empty) extension filter.
bool passesFilter(const std::string& filename, const std::vector<std::string>& extensions) {
	if (extensions.empty()) return true;
	for (const auto& ext: extensions) {
		if (hasExtension(filename, ext)) return true;
	}
	return false;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

FileBrowser::FileBrowser(lv_obj_t* parent, BackCallback onBack)
	: m_parent(parent), m_onBack(std::move(onBack)) {}

FileBrowser::~FileBrowser() {
	destroy();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

void FileBrowser::show(bool forSave, FileSelectedCallback onFileSelected, const std::string& defaultFilename) {
	m_forSave = forSave;
	m_onFileSelected = std::move(onFileSelected);

	if (!m_container) createUI();

	// ── Filename row (save mode only) ────────────────────────────────────────
	if (m_filenameInput) {
		lv_obj_t* row = lv_obj_get_parent(m_filenameInput);
		if (forSave) {
			lv_obj_remove_flag(row, LV_OBJ_FLAG_HIDDEN);
			lv_textarea_set_text(m_filenameInput, defaultFilename.empty() ? DEFAULT_FILENAME.data() : defaultFilename.c_str());
		} else {
			lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
		}
	}

	// ── Action button (save mode only) ───────────────────────────────────────
	if (m_actionBtn) {
		lv_obj_t* lbl = lv_obj_get_child(m_actionBtn, 0);
		if (lbl) lv_label_set_text(lbl, forSave ? "Save" : "Open");

		if (forSave) {
			lv_obj_remove_flag(m_actionBtn, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_add_flag(m_actionBtn, LV_OBJ_FLAG_HIDDEN);
		}
	}

	lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	refreshList();
}

void FileBrowser::hide() {
	if (m_container) lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
}

void FileBrowser::destroy() {
	if (m_container) {
		lv_obj_delete(m_container);
		m_container = nullptr;
	}
	// Child widget pointers are now dangling — null them all.
	m_list = m_pathLabel = m_filenameInput = m_actionBtn = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// UI construction
// ─────────────────────────────────────────────────────────────────────────────

void FileBrowser::createUI() {
	using namespace flx::ui::common;

	m_container = create_page_container(m_parent);

	// ── Header ───────────────────────────────────────────────────────────────
	lv_obj_t* backBtn = nullptr;
	lv_obj_t* header = create_header(m_container, "Browse", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	m_pathLabel = lv_label_create(header);
	lv_obj_set_style_pad_hor(m_pathLabel, lv_dpx(UiConstants::PAD_DEFAULT), 0);
	lv_label_set_long_mode(m_pathLabel, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
	lv_obj_set_flex_grow(m_pathLabel, 1);

	// ── Action button (shown only in save mode) ───────────────────────────────
	m_actionBtn = lv_button_create(header);
	lv_obj_set_size(m_actionBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

	lv_obj_t* actionLbl = lv_label_create(m_actionBtn);
	lv_label_set_text(actionLbl, "Save");

	lv_obj_add_event_cb(m_actionBtn, [](lv_event_t* e) { static_cast<FileBrowser*>(lv_event_get_user_data(e))->confirmSelection(); }, LV_EVENT_CLICKED, this);

	lv_obj_add_flag(m_actionBtn, LV_OBJ_FLAG_HIDDEN);

	// ── Filename input row (shown only in save mode) ──────────────────────────
	lv_obj_t* inputRow = lv_obj_create(m_container);
	lv_obj_set_size(inputRow, lv_pct(100), lv_dpx(44));
	lv_obj_set_style_pad_all(inputRow, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_border_width(inputRow, 0, 0);
	lv_obj_set_style_bg_opa(inputRow, LV_OPA_TRANSP, 0);
	lv_obj_set_flex_flow(inputRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(inputRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_remove_flag(inputRow, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(inputRow, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t* fnLabel = lv_label_create(inputRow);
	lv_label_set_text(fnLabel, "Name:");
	lv_obj_set_style_pad_right(fnLabel, lv_dpx(UiConstants::PAD_DEFAULT), 0);

	m_filenameInput = lv_textarea_create(inputRow);
	lv_obj_set_flex_grow(m_filenameInput, 1);
	lv_textarea_set_one_line(m_filenameInput, true);
	lv_textarea_set_text(m_filenameInput, DEFAULT_FILENAME.data());

	// ── File list ─────────────────────────────────────────────────────────────
	m_list = create_settings_list(m_container);
}

// ─────────────────────────────────────────────────────────────────────────────
// Directory listing
// ─────────────────────────────────────────────────────────────────────────────

void FileBrowser::refreshList() {
	if (!m_list) return;

	lv_obj_clean(m_list);
	lv_label_set_text(m_pathLabel, m_currentPath.c_str());

	// ".." entry when not at root
	if (m_currentPath != ROOT_PATH) {
		lv_obj_t* upBtn = lv_list_add_button(m_list, LV_SYMBOL_UP, "..");
		lv_obj_add_event_cb(upBtn, [](lv_event_t* e) { static_cast<FileBrowser*>(lv_event_get_user_data(e))->navigateUp(); }, LV_EVENT_CLICKED, this);
	}

	const auto entries =
		FileSystemService::getInstance().listDirectory(m_currentPath);

	for (const auto& entry: entries) {
		// Apply extension filter to files only
		if (!entry.isDirectory && !passesFilter(entry.name, m_extensions)) {
			continue;
		}

		const char* icon = entry.isDirectory ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;
		lv_obj_t* btn = lv_list_add_button(m_list, icon, entry.name.c_str());

		// Per-button context — freed via LV_EVENT_DELETE.
		struct EntryCtx {
			FileBrowser* browser;
			std::string name;
			bool isDir;
		};
		auto* ctx = new EntryCtx {this, entry.name, entry.isDirectory};

		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
                auto* c = static_cast<EntryCtx*>(lv_event_get_user_data(e));
                if (c->isDir) {
                    c->browser->enterDirectory(c->name);
                } else {
                    c->browser->selectFile(c->name);
                } }, LV_EVENT_ALL, ctx);

		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                if (lv_event_get_code(e) == LV_EVENT_DELETE) {
                    delete static_cast<EntryCtx*>(lv_event_get_user_data(e));
                } }, LV_EVENT_DELETE, ctx);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Navigation
// ─────────────────────────────────────────────────────────────────────────────

void FileBrowser::navigateUp() {
	if (m_currentPath == ROOT_PATH) return;

	const size_t pos = m_currentPath.find_last_of('/');
	if (pos == std::string::npos) return;

	// "A:/foo" → pos==2 → go to root; "A:/foo/bar" → "A:/foo"
	m_currentPath = (pos <= 2) ? std::string {ROOT_PATH}
							   : m_currentPath.substr(0, pos);
	refreshList();
}

void FileBrowser::enterDirectory(const std::string& name) {
	m_currentPath = FileSystemService::joinPath(m_currentPath, name);
	refreshList();
}

// ─────────────────────────────────────────────────────────────────────────────
// Selection
// ─────────────────────────────────────────────────────────────────────────────

void FileBrowser::selectFile(const std::string& name) {
	if (m_forSave && m_filenameInput) {
		// Save mode: populate the filename box but don't confirm yet.
		lv_textarea_set_text(m_filenameInput, name.c_str());
		return;
	}

	// Open mode: resolve and deliver the path immediately.
	dispatchSelection(name);
}

void FileBrowser::confirmSelection() {
	if (!m_filenameInput) return;

	const char* text = lv_textarea_get_text(m_filenameInput);
	if (!text || text[0] == '\0') return;

	dispatchSelection(text);
}

void FileBrowser::dispatchSelection(const std::string& name) {
	if (!m_onFileSelected) return;

	// Build a native path then convert back to LVGL VFS path for callers.
	const std::string fullPath =
		FileSystemService::joinPath(
			FileSystemService::toNativePath(m_currentPath), name
		);

	m_onFileSelected(fullPath);
}

} // namespace flx::ui