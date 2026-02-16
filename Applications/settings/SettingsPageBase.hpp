#pragma once

#include "lvgl.h"
#include <flx/ui/common/SettingsCommon.hpp>
#include <functional>

namespace System::Apps::Settings {

class SettingsPageBase {
public:

	SettingsPageBase(lv_obj_t* parent, std::function<void()> onBack)
		: m_parent(parent), m_onBack(std::move(onBack)) {}

	virtual ~SettingsPageBase() { destroyBase(); }

	SettingsPageBase(const SettingsPageBase&) = delete;
	SettingsPageBase& operator=(const SettingsPageBase&) = delete;

	virtual void show() {
		if (m_container == nullptr) {
			createUI();
		} else {
			lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}
		onShow();
	}

	void hide() {
		onHide();
		if (m_container) {
			lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}
	}

	void destroy() {
		onDestroy();
		destroyBase();
	}

protected:

	virtual void createUI() = 0;
	virtual void onShow() {}
	virtual void onHide() {}
	virtual void onDestroy() {}

	lv_obj_t* m_parent;
	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_list = nullptr;
	std::function<void()> m_onBack;

private:

	void destroyBase() {
		if (m_container && lv_obj_is_valid(m_container)) {
			lv_obj_delete(m_container);
		}
		m_container = nullptr;
		m_list = nullptr;
	}
};

} // namespace System::Apps::Settings
