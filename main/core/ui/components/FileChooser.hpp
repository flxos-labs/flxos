#pragma once

#include "lvgl.h"
#include <cstring>
#include <dirent.h>
#include <functional>
#include <stack>
#include <string>
#include <sys/stat.h>

namespace System {
namespace UI {

class FileChooser {
public:

	static void show(std::function<void(std::string)> callback, const char* initialPath = "A:/") {
		auto* chooser = new FileChooser(callback, initialPath);
		chooser->createUI();
	}

private:

	std::function<void(std::string)> m_callback;
	std::string m_currentPath;
	std::stack<std::string> m_history;

	lv_obj_t* m_dialog = nullptr;
	lv_obj_t* m_list = nullptr;
	lv_obj_t* m_pathLabel = nullptr;
	lv_obj_t* m_backBtn = nullptr;

	FileChooser(std::function<void(std::string)> cb, const char* path)
		: m_callback(cb), m_currentPath(path) {}

	void createUI() {
		m_dialog = lv_obj_create(lv_layer_top());
		lv_obj_set_size(m_dialog, lv_pct(90), lv_pct(80));
		lv_obj_center(m_dialog);
		lv_obj_set_flex_flow(m_dialog, LV_FLEX_FLOW_COLUMN);

		// Header
		lv_obj_t* header = lv_obj_create(m_dialog);
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
			auto* self = (FileChooser*)lv_event_get_user_data(e);
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
			auto* self = (FileChooser*)lv_event_get_user_data(e);
			self->close(); }, LV_EVENT_CLICKED, this);

		// File List
		m_list = lv_list_create(m_dialog);
		lv_obj_set_size(m_list, lv_pct(100), lv_pct(100)); // Flex grow
		lv_obj_set_flex_grow(m_list, 1);

		refreshList();
	}

	void close() {
		if (m_dialog) {
			lv_obj_delete(m_dialog);
		}
		delete this;
	}

	void goBack() {
		if (!m_history.empty()) {
			m_currentPath = m_history.top();
			m_history.pop();
			refreshList();
		}
	}

	void enterDir(const std::string& name) {
		m_history.push(m_currentPath);
		if (m_currentPath.back() != '/') m_currentPath += "/";
		m_currentPath += name;
		refreshList();
	}

	void handleSelection(const std::string& name) {
		std::string fullPath = m_currentPath;
		if (fullPath.back() != '/') fullPath += "/";
		fullPath += name;

		if (m_callback) {
			m_callback(fullPath);
		}
		close();
	}

	void refreshList() {
		lv_obj_clean(m_list);
		lv_label_set_text(m_pathLabel, m_currentPath.c_str());

		if (m_history.empty()) {
			lv_obj_add_state(m_backBtn, LV_STATE_DISABLED);
		} else {
			lv_obj_remove_state(m_backBtn, LV_STATE_DISABLED);
		}

		lv_fs_dir_t dir;
		lv_fs_res_t res = lv_fs_dir_open(&dir, m_currentPath.c_str());

		if (res != LV_FS_RES_OK) {
			// Try fallback for root
			if (m_currentPath == "A:/" || m_currentPath == "A:") {
				addListItem("system", true);
				addListItem("data", true);
				return;
			}
			lv_list_add_text(m_list, "Error opening directory");
			return;
		}

		char fn[256];
		while (lv_fs_dir_read(&dir, fn, sizeof(fn)) == LV_FS_RES_OK) {
			if (fn[0] == '\0') break;
			bool is_dir = (fn[0] == '/');
			const char* name = is_dir ? fn + 1 : fn;
			if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
			addListItem(name, is_dir);
		}
		lv_fs_dir_close(&dir);
	}

	void addListItem(const std::string& name, bool isDir) {
		const char* symbol = isDir ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;
		lv_obj_t* btn = lv_list_add_button(m_list, symbol, name.c_str());

		// Use user_data to store directory flag
		lv_obj_set_user_data(btn, (void*)(uintptr_t)isDir);

		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
			auto* self = (FileChooser*)lv_event_get_user_data(e);
			lv_obj_t* btn = lv_event_get_target_obj(e);
			bool isDir = (bool)(uintptr_t)lv_obj_get_user_data(btn);
			const char* name = lv_list_get_button_text(self->m_list, btn);

			if (isDir) {
				self->enterDir(name);
			} else {
				self->handleSelection(name);
			} }, LV_EVENT_CLICKED, this);
	}
};

} // namespace UI
} // namespace System
