#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "display/lv_display.h"
#include "layouts/flex/lv_flex.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_text.h"
#include "misc/lv_types.h"
#include "widgets/label/lv_label.h"
#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/apps/AppRegistry.hpp>
#include <flx/ui/desktop/modules/launcher/Launcher.hpp>
#include <flx/ui/theming/StyleUtils.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>

namespace UI::Modules {

Launcher::Launcher(lv_obj_t* parent, lv_obj_t* dock, lv_event_cb_t appClickCb, void* userData)
	: m_parent(parent), m_dock(dock), m_appClickCb(appClickCb), m_userData(userData) {
	create();
}

void Launcher::create() {
	m_panel = lv_obj_create(m_parent);
	// configure_panel_style(launcher) logic:
	lv_obj_set_size(m_panel, lv_pct(LayoutConstants::PANEL_WIDTH_PCT), lv_pct(LayoutConstants::PANEL_HEIGHT_PCT));
	lv_obj_set_style_pad_all(m_panel, 0, 0);
	lv_obj_set_style_radius(m_panel, lv_dpx(UiConstants::RADIUS_LARGE), 0);
	lv_obj_set_style_border_width(m_panel, 0, 0);
	lv_obj_add_flag(m_panel, LV_OBJ_FLAG_FLOATING);
	lv_obj_add_flag(m_panel, LV_OBJ_FLAG_HIDDEN);
	UI::StyleUtils::apply_glass(m_panel, lv_dpx(UiConstants::GLASS_BLUR_SMALL));

	lv_obj_align_to(m_panel, m_dock, LV_ALIGN_OUT_TOP_LEFT, 0, -lv_dpx(UiConstants::OFFSET_TINY));
	lv_obj_set_flex_flow(m_panel, LV_FLEX_FLOW_COLUMN);

	lv_obj_t* label = lv_label_create(m_panel);
	lv_label_set_text(label, "Applications");
	lv_obj_set_width(label, lv_pct(100));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_pad_all(label, lv_dpx(UiConstants::PAD_MEDIUM), 0);

	m_list = lv_obj_create(m_panel);
	lv_obj_remove_style_all(m_list);
	lv_obj_set_width(m_list, lv_pct(100));
	lv_obj_set_flex_grow(m_list, 1);
	lv_obj_set_flex_flow(m_list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(m_list, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_pad_row(m_list, lv_dpx(UiConstants::PAD_TINY), 0);

	auto apps = flx::apps::AppManager::getInstance().getInstalledApps();
	for (auto& app: apps) {
		// Skip hidden apps (e.g. Image Viewer — launched via intent only)
		auto manifest = flx::apps::AppRegistry::getInstance().findById(app->getPackageName());
		if (manifest && (manifest->flags & flx::apps::AppFlags::Hidden)) {
			continue;
		}
		lv_obj_t* btn = lv_button_create(m_list);
		lv_obj_set_width(btn, lv_pct(100));
		lv_obj_set_style_border_width(btn, 0, 0);
		lv_obj_set_style_shadow_width(btn, 0, 0);
		lv_obj_set_style_pad_all(btn, lv_dpx(UiConstants::PAD_LARGE), 0);
		lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		lv_obj_t* icon = lv_label_create(btn);
		lv_label_set_text(icon, (const char*)app->getIcon());
		lv_obj_set_style_margin_right(icon, lv_dpx(UiConstants::PAD_SMALL), 0);

		lv_obj_t* name = lv_label_create(btn);
		lv_label_set_text(name, app->getAppName().c_str());

		lv_obj_set_user_data(btn, app.get());
		lv_obj_add_event_cb(btn, m_appClickCb, LV_EVENT_CLICKED, m_userData);
	}
}

} // namespace UI::Modules
