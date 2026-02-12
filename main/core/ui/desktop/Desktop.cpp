#include "Desktop.hpp"
#include "../theming/StyleUtils.hpp"
#include "../theming/layout_constants/LayoutConstants.hpp"
#include "../theming/theme_engine/ThemeEngine.hpp"
#include "../theming/ui_constants/UiConstants.hpp"
#include "core/apps/AppManager.hpp"
#include "core/apps/Intent.hpp"
#include "core/common/Logger.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_style_gen.h"
#include "core/lv_observer.h"
#include "core/system/display/DisplayManager.hpp"
#include "core/system/focus/FocusManager.hpp"
#include "core/system/system_core/SystemManager.hpp"
#include "core/system/theme/ThemeManager.hpp"

#include "core/ui/theming/themes/Themes.hpp"
#include "display/lv_display.h"
#include "font/lv_symbol_def.h"
#include "misc/cache/instance/lv_image_cache.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_types.h"
#include "widgets/image/lv_image.h"
#include "window_manager/WindowManager.hpp"
#include <ctime>
#include <string_view>

static constexpr std::string_view TAG = "Desktop";

Desktop& Desktop::getInstance() {
	static Desktop instance;
	return instance;
}

Desktop::Desktop()
	: m_screen(nullptr), m_wallpaper(nullptr), m_wallpaper_img(nullptr),
	  m_wallpaper_icon(nullptr), m_window_container(nullptr),
	  m_statusBarModule(nullptr), m_status_bar(nullptr),
	  m_dockModule(nullptr), m_dock(nullptr),
	  m_launcherModule(nullptr), m_launcher(nullptr),
	  m_quickAccessPanelModule(nullptr), m_quick_access_panel(nullptr),
	  m_notificationPanelModule(nullptr), m_notification_panel(nullptr),
	  m_notification_list(nullptr), m_clear_all_btn(nullptr), m_greetings(nullptr), m_app_container(nullptr),
	  m_swipeManagerModule(nullptr) {}

Desktop::~Desktop() = default;

void Desktop::init() {
	Log::info(TAG, "Initializing Desktop Environment...");
	m_screen = lv_obj_create(NULL);
	lv_obj_remove_style_all(m_screen);
	lv_obj_set_flex_flow(m_screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_screen_load(m_screen);

	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());

	if (!System::SystemManager::getInstance().isSafeMode()) {
		m_wallpaper = lv_obj_create(m_screen);
		lv_obj_remove_style_all(m_wallpaper);
		lv_obj_set_size(m_wallpaper, lv_pct(100), lv_pct(100));

		lv_obj_set_style_bg_color(m_wallpaper, cfg.primary, 0);
		lv_obj_set_style_bg_opa(m_wallpaper, UiConstants::OPA_COVER, 0);
		lv_obj_add_flag(m_wallpaper, LV_OBJ_FLAG_FLOATING);
		lv_obj_move_background(m_wallpaper);

		m_wallpaper_icon = lv_image_create(m_wallpaper);
		lv_image_set_src(m_wallpaper_icon, LV_SYMBOL_IMAGE);
		lv_obj_set_style_text_opa(m_wallpaper_icon, UiConstants::OPA_30, 0);

		lv_obj_center(m_wallpaper_icon);

		lv_subject_add_observer_obj(
			&System::ThemeManager::getInstance().getThemeSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* wp = lv_observer_get_target_obj(observer);
				if (wp) {
					ThemeType theme = (ThemeType)lv_subject_get_int(subject);
					ThemeConfig cfg = Themes::GetConfig(theme);
					lv_obj_set_style_bg_color(wp, cfg.primary, 0);
				}
			},
			m_wallpaper, nullptr
		);

		lv_subject_add_observer(
			&System::ThemeManager::getInstance().getWallpaperEnabledSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				auto* instance = (Desktop*)lv_observer_get_user_data(observer);
				bool const enabled = lv_subject_get_int(subject);

				if (enabled) {
					if (instance->m_wallpaper_icon) {
						lv_obj_add_flag(instance->m_wallpaper_icon, LV_OBJ_FLAG_HIDDEN);
					}
					if (instance->m_wallpaper_img == nullptr && instance->m_wallpaper != nullptr) {
						const char* path = (const char*)lv_subject_get_pointer(&System::ThemeManager::getInstance().getWallpaperPathSubject());
						if (path && strlen(path) > 0) {
							instance->createWallpaperImage(path);
						}
					}
				} else {
					if (instance->m_wallpaper_icon) {
						lv_obj_remove_flag(instance->m_wallpaper_icon, LV_OBJ_FLAG_HIDDEN);
					}
					if (instance->m_wallpaper_img != nullptr) {
						lv_image_cache_drop(lv_image_get_src(instance->m_wallpaper_img));
						lv_obj_delete(instance->m_wallpaper_img);
						instance->m_wallpaper_img = nullptr;
					}
				}
			},
			this
		);

		lv_subject_add_observer(
			&System::ThemeManager::getInstance().getWallpaperPathSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				auto* instance = (Desktop*)lv_observer_get_user_data(observer);
				const char* path = (const char*)lv_subject_get_pointer(subject);
				if (path && strlen(path) > 0) {
					if (instance->m_wallpaper_img) {
						lv_image_set_src(instance->m_wallpaper_img, path);
					} else if (System::ThemeManager::getInstance().getWallpaperEnabledObservable().get()) {
						if (instance->m_wallpaper != nullptr) {
							instance->createWallpaperImage(path);
						}
					}
				} else {
					if (instance->m_wallpaper_img) {
						lv_image_cache_drop(lv_image_get_src(instance->m_wallpaper_img));
						lv_obj_delete(instance->m_wallpaper_img);
						instance->m_wallpaper_img = nullptr;
					}
				}
			},
			this
		);
		lv_obj_set_style_bg_opa(m_screen, UiConstants::OPA_COVER, 0);
	}

	m_statusBarModule = std::make_unique<UI::Modules::StatusBar>(m_screen);
	m_status_bar = m_statusBarModule->getObj();

	m_window_container = lv_obj_create(m_screen);
	lv_obj_remove_style_all(m_window_container);
	lv_obj_remove_flag(m_window_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_width(m_window_container, lv_pct(100));
	lv_obj_set_flex_grow(m_window_container, 1);

	m_dockModule = std::make_unique<UI::Modules::Dock>(m_screen, UI::Modules::Dock::Callbacks {.onStartClick = [this]() { on_start_click(); }, .onUpClick = [this]() { on_up_click(); }});
	m_dock = m_dockModule->getObj();
	m_app_container = m_dockModule->getAppContainer();

	WindowManager::getInstance().init(m_window_container, m_app_container, m_screen, m_status_bar, m_dock);

	if (m_wallpaper) {
		m_greetings = lv_label_create(m_wallpaper);
		lv_label_set_text(m_greetings, "Hey !");
		lv_obj_align_to(m_greetings, m_dock, LV_ALIGN_OUT_TOP_RIGHT, -lv_dpx(UiConstants::OFFSET_TINY), -lv_dpx(UiConstants::OFFSET_TINY));
	}

	m_launcherModule = std::make_unique<UI::Modules::Launcher>(m_screen, m_dock, on_app_click, this);
	m_launcher = m_launcherModule->getObj();

	m_quickAccessPanelModule = std::make_unique<UI::Modules::QuickAccessPanel>(m_screen, m_dock);
	m_quick_access_panel = m_quickAccessPanelModule->getObj();

	m_notificationPanelModule = std::make_unique<UI::Modules::NotificationPanel>(m_screen, m_status_bar);
	m_notification_panel = m_notificationPanelModule->getObj();
	m_notification_list = m_notificationPanelModule->getList();

	m_swipeManagerModule = std::make_unique<UI::Modules::SwipeManager>(UI::Modules::SwipeManager::Config {
		.screen = m_screen,
		.statusBar = m_status_bar,
		.notificationPanel = m_notification_panel,
		.notificationList = m_notification_list
	});

	// Register panels with FocusManager
	System::FocusManager::getInstance().registerPanel(m_launcher);
	System::FocusManager::getInstance().registerPanel(m_quick_access_panel);
	System::FocusManager::getInstance().registerPanel(m_notification_panel);
	System::FocusManager::getInstance().setNotificationPanel(m_notification_panel);

	lv_subject_add_observer(
		&System::DisplayManager::getInstance().getRotationSubject(),
		[](lv_observer_t* observer, lv_subject_t* /*subject*/) {
			auto* instance = (Desktop*)lv_observer_get_user_data(observer);
			if (instance && instance->m_screen) {
				Log::info(TAG, "Realigning panels due to rotation");
				lv_obj_update_layout(instance->m_screen);
				instance->realign_panels();
			}
		},
		this
	);
	Log::info(TAG, "DE initialization complete");
}

void Desktop::configure_panel_style(lv_obj_t* panel) {
	lv_obj_set_size(panel, lv_pct(LayoutConstants::PANEL_WIDTH_PCT), lv_pct(LayoutConstants::PANEL_HEIGHT_PCT));
	lv_obj_set_style_pad_all(panel, 0, 0);
	lv_obj_set_style_radius(panel, lv_dpx(UiConstants::RADIUS_LARGE), 0);
	lv_obj_set_style_border_width(panel, 0, 0);
	lv_obj_add_flag(panel, LV_OBJ_FLAG_FLOATING);
	lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
	UI::StyleUtils::apply_glass(panel, lv_dpx(UiConstants::GLASS_BLUR_SMALL));
}

void Desktop::createWallpaperImage(const char* path) {
	m_wallpaper_img = lv_image_create(m_wallpaper);
	lv_image_set_src(m_wallpaper_img, path);
	lv_obj_set_size(m_wallpaper_img, lv_pct(100), lv_pct(100));
	lv_obj_set_style_pad_all(m_wallpaper_img, 0, 0);
	lv_obj_set_style_border_width(m_wallpaper_img, 0, 0);
	lv_image_set_inner_align(m_wallpaper_img, LV_IMAGE_ALIGN_COVER);
	lv_obj_move_background(m_wallpaper_img);
}

void Desktop::realign_panels() {
	if (m_dock) {
		if (m_launcher) {
			lv_obj_align_to(m_launcher, m_dock, LV_ALIGN_OUT_TOP_LEFT, 0, -lv_dpx(UiConstants::OFFSET_TINY));
		}
		if (m_quick_access_panel) {
			lv_obj_align_to(m_quick_access_panel, m_dock, LV_ALIGN_OUT_TOP_RIGHT, 0, -lv_dpx(UiConstants::OFFSET_TINY));
		}
		if (m_greetings) {
			lv_obj_align_to(m_greetings, m_dock, LV_ALIGN_OUT_TOP_RIGHT, -lv_dpx(UiConstants::OFFSET_TINY), -lv_dpx(UiConstants::OFFSET_TINY));
		}
		if (m_notification_panel) {
			lv_obj_align(m_notification_panel, LV_ALIGN_TOP_MID, 0, 0);
		}
	}
}

void Desktop::on_start_click() {
	if (m_launcher) {
		realign_panels();
		System::FocusManager::getInstance().togglePanel(m_launcher);
	}
}

void Desktop::on_up_click() {
	if (m_quick_access_panel) {
		realign_panels();
		System::FocusManager::getInstance().togglePanel(m_quick_access_panel);
	}
}

void Desktop::on_app_click(lv_event_t* e) {
	auto* d = (Desktop*)lv_event_get_user_data(e);
	lv_obj_t* btn = lv_event_get_target_obj(e);

	auto* appPtr = (System::Apps::App*)lv_obj_get_user_data(btn);
	if (!appPtr) {
		return;
	}

	std::string packageName = appPtr->getPackageName();

	System::FocusManager::getInstance().dismissAllPanels();
	System::Apps::AppManager::getInstance().startApp(
		System::Apps::Intent::forApp(packageName)
	);
}

void Desktop::openApp(const std::string& packageName) {
	Log::info(TAG, "Requesting WM to open app: %s", packageName.c_str());
	WindowManager::getInstance().openApp(packageName);
}

void Desktop::closeApp(const std::string& packageName) {
	WindowManager::getInstance().closeApp(packageName);
}

void Desktop::update_notification_list() {
	if (m_notificationPanelModule) m_notificationPanelModule->update_list();
}
