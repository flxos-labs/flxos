#include "WallpaperEngineApp.hpp"

#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/ui/common/SettingsCommon.hpp>

using namespace flx::apps;
using namespace flx::ui::common;

namespace System::Apps {

// ---------------------------------------------------------------------------
// Manifest
// ---------------------------------------------------------------------------

const AppManifest WallpaperEngineApp::manifest = {
	.appId = "com.flxos.wallpaper_engine",
	.appName = "Wallpaper Engine",
	.appIcon = LV_SYMBOL_IMAGE,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::SingleInstance,
	.location = AppLocation::internal(),
	.description = "Browse presets, apply effects, and configure generative wallpapers",
	.sortPriority = 15,
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<WallpaperEngineApp>(); }};

// ---------------------------------------------------------------------------
// App identity
// ---------------------------------------------------------------------------

std::string WallpaperEngineApp::getPackageName() const { return manifest.appId; }
std::string WallpaperEngineApp::getAppName() const { return manifest.appName; }
const void* WallpaperEngineApp::getIcon() const { return manifest.appIcon; }

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void WallpaperEngineApp::createUI(void* parent) {
	m_container = static_cast<lv_obj_t*>(parent);

	// Lazily create all pages (hidden by default)
	m_presetsPage = std::make_unique<WallpaperEngine::PresetsPage>(
		m_container, [this]() { showMainMenu(); });
	m_effectsPage = std::make_unique<WallpaperEngine::EffectsPage>(
		m_container, [this]() { showMainMenu(); });
	m_dynamicPage = std::make_unique<WallpaperEngine::DynamicPage>(
		m_container, [this]() { showMainMenu(); });
	m_adaptivePage = std::make_unique<WallpaperEngine::AdaptivePage>(
		m_container, [this]() { showMainMenu(); });

	showMainMenu();
}

void WallpaperEngineApp::onStop() {
	m_presetsPage.reset();
	m_effectsPage.reset();
	m_dynamicPage.reset();
	m_adaptivePage.reset();
	m_mainMenu = nullptr;
	m_container = nullptr;
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void WallpaperEngineApp::hideAllPages() {
	if (m_mainMenu != nullptr) {
		lv_obj_add_flag(m_mainMenu, LV_OBJ_FLAG_HIDDEN);
	}
	if (m_presetsPage) m_presetsPage->hide();
	if (m_effectsPage) m_effectsPage->hide();
	if (m_dynamicPage) m_dynamicPage->hide();
	if (m_adaptivePage) m_adaptivePage->hide();
}

void WallpaperEngineApp::showMainMenu() {
	hideAllPages();

	if (m_mainMenu == nullptr) {
		// Build main menu list
		m_mainMenu = lv_list_create(m_container);
		lv_obj_set_size(m_mainMenu, lv_pct(100), lv_pct(100));
		lv_obj_set_style_border_width(m_mainMenu, 0, 0);

		lv_list_add_text(m_mainMenu, "Wallpaper Engine");

		// Presets
		lv_obj_t* presetsBtn = add_list_btn(m_mainMenu, LV_SYMBOL_IMAGE, "Presets");
		lv_obj_add_event_cb(
			presetsBtn,
			[](lv_event_t* e) {
				auto* app = static_cast<WallpaperEngineApp*>(lv_event_get_user_data(e));
				app->showPresetsPage();
			},
			LV_EVENT_CLICKED, this);

		// Effects
		lv_obj_t* effectsBtn = add_list_btn(m_mainMenu, LV_SYMBOL_SETTINGS, "Effects & Controls");
		lv_obj_add_event_cb(
			effectsBtn,
			[](lv_event_t* e) {
				auto* app = static_cast<WallpaperEngineApp*>(lv_event_get_user_data(e));
				app->showEffectsPage();
			},
			LV_EVENT_CLICKED, this);

		// Dynamic / Generative
		lv_obj_t* dynamicBtn = add_list_btn(m_mainMenu, LV_SYMBOL_LOOP, "Dynamic Wallpapers");
		lv_obj_add_event_cb(
			dynamicBtn,
			[](lv_event_t* e) {
				auto* app = static_cast<WallpaperEngineApp*>(lv_event_get_user_data(e));
				app->showDynamicPage();
			},
			LV_EVENT_CLICKED, this);

		// Adaptive
		lv_obj_t* adaptiveBtn = add_list_btn(m_mainMenu, LV_SYMBOL_LEFT, "Adaptive Mode");
		lv_obj_add_event_cb(
			adaptiveBtn,
			[](lv_event_t* e) {
				auto* app = static_cast<WallpaperEngineApp*>(lv_event_get_user_data(e));
				app->showAdaptivePage();
			},
			LV_EVENT_CLICKED, this);
	} else {
		lv_obj_remove_flag(m_mainMenu, LV_OBJ_FLAG_HIDDEN);
	}
}

void WallpaperEngineApp::showPresetsPage() {
	hideAllPages();
	if (m_presetsPage) m_presetsPage->show();
}

void WallpaperEngineApp::showEffectsPage() {
	hideAllPages();
	if (m_effectsPage) m_effectsPage->show();
}

void WallpaperEngineApp::showDynamicPage() {
	hideAllPages();
	if (m_dynamicPage) m_dynamicPage->show();
}

void WallpaperEngineApp::showAdaptivePage() {
	hideAllPages();
	if (m_adaptivePage) m_adaptivePage->show();
}

} // namespace System::Apps
