#include "DE.hpp"
#include "../theming/LayoutConstants/LayoutConstants.hpp"
#include "../theming/StyleUtils.hpp"
#include "../theming/ThemeEngine/ThemeEngine.hpp"
#include "../theming/UiConstants/UiConstants.hpp"
#include "core/apps/AppManager.hpp"
#include "core/common/Logger.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/system/Display/DisplayManager.hpp"
#include "core/system/Focus/FocusManager.hpp"
#include "core/system/Notification/NotificationManager.hpp"
#include "core/system/System/SystemManager.hpp"
#include "core/system/Theme/ThemeManager.hpp"

#include "wm/WM.hpp"
#include <ctime>
#include <string_view>

static constexpr std::string_view TAG = "DE";

DE& DE::getInstance() {
	static DE instance;
	return instance;
}

DE::DE()
	: screen(nullptr), wallpaper(nullptr), wallpaper_img(nullptr),
	  wallpaper_icon(nullptr), window_container(nullptr),
	  m_statusBarModule(nullptr), status_bar(nullptr),
	  m_dockModule(nullptr), dock(nullptr),
	  m_launcherModule(nullptr), launcher(nullptr),
	  m_quickAccessPanelModule(nullptr), quick_access_panel(nullptr),
	  m_notificationPanelModule(nullptr), notification_panel(nullptr),
	  notification_list(nullptr), clear_all_btn(nullptr), greetings(nullptr), app_container(nullptr),
	  m_swipeManagerModule(nullptr) {}

DE::~DE() {}

void DE::init() {
	Log::info(TAG, "Initializing Desktop Environment...");
	screen = lv_obj_create(NULL);
	lv_obj_remove_style_all(screen);
	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_screen_load(screen);

	ThemeConfig cfg = Themes::GetConfig(ThemeEngine::get_current_theme());

	if (!System::SystemManager::getInstance().isSafeMode()) {
		wallpaper = lv_obj_create(screen);
		lv_obj_remove_style_all(wallpaper);
		lv_obj_set_size(wallpaper, lv_pct(100), lv_pct(100));

		lv_obj_set_style_bg_color(wallpaper, cfg.primary, 0);
		lv_obj_set_style_bg_opa(wallpaper, UiConstants::OPA_COVER, 0);
		lv_obj_add_flag(wallpaper, LV_OBJ_FLAG_FLOATING);
		lv_obj_move_background(wallpaper);

		wallpaper_icon = lv_image_create(wallpaper);
		lv_image_set_src(wallpaper_icon, LV_SYMBOL_IMAGE);
		lv_obj_set_style_text_opa(wallpaper_icon, UiConstants::OPA_30, 0);

		lv_obj_center(wallpaper_icon);

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
			wallpaper, nullptr
		);

		lv_subject_add_observer(
			&System::ThemeManager::getInstance().getWallpaperEnabledSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				DE* instance = (DE*)lv_observer_get_user_data(observer);
				bool enabled = lv_subject_get_int(subject);

				if (enabled) {
					if (instance->wallpaper_icon)
						lv_obj_add_flag(instance->wallpaper_icon, LV_OBJ_FLAG_HIDDEN);
					if (instance->wallpaper_img == nullptr &&
						instance->wallpaper != nullptr) {
						instance->wallpaper_img = lv_image_create(instance->wallpaper);
						lv_image_set_src(instance->wallpaper_img, "A:/data/wallpaper.jpg");
						lv_obj_set_size(instance->wallpaper_img, lv_pct(100), lv_pct(100));
						lv_obj_set_style_pad_all(instance->wallpaper_img, 0, 0);
						lv_obj_set_style_border_width(instance->wallpaper_img, 0, 0);
						lv_image_set_inner_align(instance->wallpaper_img, LV_IMAGE_ALIGN_COVER);
						lv_obj_move_background(instance->wallpaper_img);
					}
				} else {
					if (instance->wallpaper_icon)
						lv_obj_remove_flag(instance->wallpaper_icon, LV_OBJ_FLAG_HIDDEN);
					if (instance->wallpaper_img != nullptr) {
						lv_obj_delete(instance->wallpaper_img);
						instance->wallpaper_img = nullptr;
					}
				}
			},
			this
		);
		lv_obj_set_style_bg_opa(screen, UiConstants::OPA_COVER, 0);
	}

	m_statusBarModule = std::make_unique<UI::Modules::StatusBar>(screen);
	status_bar = m_statusBarModule->getObj();

	window_container = lv_obj_create(screen);
	lv_obj_remove_style_all(window_container);
	lv_obj_remove_flag(window_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_width(window_container, lv_pct(100));
	lv_obj_set_flex_grow(window_container, 1);

	m_dockModule = std::make_unique<UI::Modules::Dock>(screen, UI::Modules::Dock::Callbacks {.onStartClick = [this]() { on_start_click(); }, .onUpClick = [this]() { on_up_click(); }});
	dock = m_dockModule->getObj();
	app_container = m_dockModule->getAppContainer();

	WM::getInstance().init(window_container, app_container, screen, status_bar, dock);

	if (wallpaper) {
		greetings = lv_label_create(wallpaper);
		lv_label_set_text(greetings, "Hey !");
		lv_obj_align_to(greetings, dock, LV_ALIGN_OUT_TOP_RIGHT, -lv_dpx(UiConstants::OFFSET_TINY), -lv_dpx(UiConstants::OFFSET_TINY));
	}

	m_launcherModule = std::make_unique<UI::Modules::Launcher>(screen, dock, on_app_click, this);
	launcher = m_launcherModule->getObj();

	m_quickAccessPanelModule = std::make_unique<UI::Modules::QuickAccessPanel>(screen, dock);
	quick_access_panel = m_quickAccessPanelModule->getObj();

	m_notificationPanelModule = std::make_unique<UI::Modules::NotificationPanel>(screen, status_bar);
	notification_panel = m_notificationPanelModule->getObj();
	notification_list = m_notificationPanelModule->getList();

	m_swipeManagerModule = std::make_unique<UI::Modules::SwipeManager>(UI::Modules::SwipeManager::Config {
		.screen = screen,
		.statusBar = status_bar,
		.notificationPanel = notification_panel,
		.notificationList = notification_list
	});

	// Register panels with FocusManager
	System::FocusManager::getInstance().registerPanel(launcher);
	System::FocusManager::getInstance().registerPanel(quick_access_panel);
	System::FocusManager::getInstance().registerPanel(notification_panel);
	System::FocusManager::getInstance().setNotificationPanel(notification_panel);

	lv_subject_add_observer(
		&System::DisplayManager::getInstance().getRotationSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			DE* instance = (DE*)lv_observer_get_user_data(observer);
			if (instance && instance->screen) {
				Log::info(TAG, "Realigning panels due to rotation");
				lv_obj_update_layout(instance->screen);
				instance->realign_panels();
			}
		},
		this
	);
	Log::info(TAG, "DE initialization complete");
}

void DE::configure_panel_style(lv_obj_t* panel) {
	lv_obj_set_size(panel, lv_pct(LayoutConstants::PANEL_WIDTH_PCT), lv_pct(LayoutConstants::PANEL_HEIGHT_PCT));
	lv_obj_set_style_pad_all(panel, 0, 0);
	lv_obj_set_style_radius(panel, lv_dpx(UiConstants::RADIUS_LARGE), 0);
	lv_obj_set_style_border_width(panel, 0, 0);
	lv_obj_add_flag(panel, LV_OBJ_FLAG_FLOATING);
	lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
	UI::StyleUtils::apply_glass(panel, lv_dpx(UiConstants::GLASS_BLUR_SMALL));
}

void DE::realign_panels() {
	if (dock) {
		if (launcher) {
			lv_obj_align_to(launcher, dock, LV_ALIGN_OUT_TOP_LEFT, 0, -lv_dpx(UiConstants::OFFSET_TINY));
		}
		if (quick_access_panel) {
			lv_obj_align_to(quick_access_panel, dock, LV_ALIGN_OUT_TOP_RIGHT, 0, -lv_dpx(UiConstants::OFFSET_TINY));
		}
		if (greetings) {
			lv_obj_align_to(greetings, dock, LV_ALIGN_OUT_TOP_RIGHT, -lv_dpx(UiConstants::OFFSET_TINY), -lv_dpx(UiConstants::OFFSET_TINY));
		}
		if (notification_panel) {
			lv_obj_align(notification_panel, LV_ALIGN_TOP_MID, 0, 0);
		}
	}
}

void DE::on_start_click() {
	if (launcher) {
		realign_panels();
		System::FocusManager::getInstance().togglePanel(launcher);
	}
}

void DE::on_up_click() {
	if (quick_access_panel) {
		realign_panels();
		System::FocusManager::getInstance().togglePanel(quick_access_panel);
	}
}

void DE::on_app_click(lv_event_t* e) {
	DE* d = (DE*)lv_event_get_user_data(e);
	lv_obj_t* btn = lv_event_get_target_obj(e);

	System::Apps::App* appPtr = (System::Apps::App*)lv_obj_get_user_data(btn);
	if (!appPtr)
		return;

	std::string packageName = appPtr->getPackageName();

	System::FocusManager::getInstance().dismissAllPanels();
	d->openApp(packageName);
}

void DE::openApp(const std::string& packageName) {
	Log::info(TAG, "Requesting WM to open app: %s", packageName.c_str());
	WM::getInstance().openApp(packageName);
}

void DE::closeApp(const std::string& packageName) {
	WM::getInstance().closeApp(packageName);
}

void DE::update_notification_list() {
	if (m_notificationPanelModule) m_notificationPanelModule->update_list();
}
