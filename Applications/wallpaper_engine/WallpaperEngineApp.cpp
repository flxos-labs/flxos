#include "WallpaperEngineApp.hpp"

#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/common/SettingsCommon.hpp>

using namespace flx::apps;
using namespace flx::ui::common;

namespace System::Apps {

static constexpr const char* TAG_WPE_APP = "WallpaperEngineApp";

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
	.description = "Dynamic wallpaper controls",
	.sortPriority = 15,
	.requiredServices = {},
	.supportedMimeTypes = {},
	.urlSchemes = {},
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
	ensureFallbackBanner();

	m_wallpaperErrorSubscriptionId = flx::core::EventBus::getInstance().subscribe(
		"wallpaper.error",
		[this](const std::string& /*event*/, const flx::core::Bundle& data) {
			handleWallpaperErrorEvent(data);
		});

	m_bannerPollTimer = lv_timer_create(
		[](lv_timer_t* timer) {
			auto* app = static_cast<WallpaperEngineApp*>(lv_timer_get_user_data(timer));
			if (app != nullptr) {
				app->pollBannerDismissState();
			}
		},
		250,
		this);

	if (getContext() != nullptr) {
		navigateFromIntent(getContext()->getIntent());
	} else {
		showDynamicPage();
	}
}

void WallpaperEngineApp::onNewIntent(const flx::apps::Intent& intent) {
	navigateFromIntent(intent);
}

void WallpaperEngineApp::onStop() {
	if (m_wallpaperErrorSubscriptionId != 0) {
		flx::core::EventBus::getInstance().unsubscribe(m_wallpaperErrorSubscriptionId);
		m_wallpaperErrorSubscriptionId = 0;
	}

	if (m_bannerPollTimer != nullptr) {
		lv_timer_delete(m_bannerPollTimer);
		m_bannerPollTimer = nullptr;
	}

	if (m_dynamicPage != nullptr) {
		lv_obj_del(m_dynamicPage);
		m_dynamicPage = nullptr;
	}
	m_fallbackBanner = nullptr;
	m_fallbackLabel = nullptr;
	m_retryBtn = nullptr;
	m_chooseSourceBtn = nullptr;
	m_lastFailedType.clear();
	m_lastFailedSource.clear();
	m_lastReasonCode.clear();
	m_lastErrorTickMs = 0;
	m_container = nullptr;
}

void WallpaperEngineApp::ensureFallbackBanner() {
	if (m_container == nullptr || m_fallbackBanner != nullptr) {
		return;
	}

	m_fallbackBanner = lv_obj_create(m_container);
	lv_obj_set_width(m_fallbackBanner, lv_pct(100));
	lv_obj_set_height(m_fallbackBanner, LV_SIZE_CONTENT);
	lv_obj_set_style_radius(m_fallbackBanner, 0, 0);
	lv_obj_set_style_border_width(m_fallbackBanner, 0, 0);
	lv_obj_set_style_bg_color(m_fallbackBanner, lv_palette_main(LV_PALETTE_RED), 0);
	lv_obj_set_style_bg_opa(m_fallbackBanner, LV_OPA_80, 0);
	lv_obj_set_style_pad_all(m_fallbackBanner, 8, 0);
	lv_obj_set_style_pad_column(m_fallbackBanner, 8, 0);
	lv_obj_set_style_pad_row(m_fallbackBanner, 8, 0);
	lv_obj_set_flex_flow(m_fallbackBanner, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(m_fallbackBanner, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_add_flag(m_fallbackBanner, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_align(m_fallbackBanner, LV_ALIGN_TOP_MID);
	lv_obj_move_foreground(m_fallbackBanner);

	lv_obj_t* icon = lv_label_create(m_fallbackBanner);
	lv_label_set_text(icon, LV_SYMBOL_WARNING);
	lv_obj_set_style_text_color(icon, lv_color_white(), 0);

	m_fallbackLabel = lv_label_create(m_fallbackBanner);
	lv_obj_set_flex_grow(m_fallbackLabel, 1);
	lv_label_set_long_mode(m_fallbackLabel, LV_LABEL_LONG_WRAP);
	lv_label_set_text(m_fallbackLabel, "Wallpaper fallback active");
	lv_obj_set_style_text_color(m_fallbackLabel, lv_color_white(), 0);

	m_retryBtn = lv_button_create(m_fallbackBanner);
	lv_obj_t* retryLabel = lv_label_create(m_retryBtn);
	lv_label_set_text(retryLabel, "Retry");
	lv_obj_add_event_cb(
		m_retryBtn,
		[](lv_event_t* e) {
			auto* app = static_cast<WallpaperEngineApp*>(lv_event_get_user_data(e));
			if (app != nullptr) {
				app->retryLastWallpaper();
			}
		},
		LV_EVENT_CLICKED,
		this);

	m_chooseSourceBtn = lv_button_create(m_fallbackBanner);
	lv_obj_t* chooseLabel = lv_label_create(m_chooseSourceBtn);
	lv_label_set_text(chooseLabel, "Choose Source");
	lv_obj_add_event_cb(
		m_chooseSourceBtn,
		[](lv_event_t* e) {
			auto* app = static_cast<WallpaperEngineApp*>(lv_event_get_user_data(e));
			if (app != nullptr) {
				app->openSourceSelector();
			}
		},
		LV_EVENT_CLICKED,
		this);

	lv_obj_add_flag(m_fallbackBanner, LV_OBJ_FLAG_HIDDEN);
}

void WallpaperEngineApp::handleWallpaperErrorEvent(const flx::core::Bundle& data) {
	m_lastFailedType = data.getStringOr("requested_type", "static");
	m_lastFailedSource = data.getStringOr("source", "");
	m_lastReasonCode = data.getStringOr("reason_code", "render_error");
	m_lastErrorTickMs = lv_tick_get();

	std::string message = "Wallpaper fallback active";
	if (m_lastReasonCode == "complexity_exceeded") {
		message = "Lottie too complex for this device. Fallback active.";
	} else if (m_lastReasonCode == "parse_error") {
		message = "Wallpaper file could not be parsed. Fallback active.";
	} else if (m_lastReasonCode == "render_error") {
		message = "Wallpaper failed to render. Fallback active.";
	}

	showFallbackBanner(message);
}

void WallpaperEngineApp::showFallbackBanner(const std::string& message) {
	ensureFallbackBanner();
	if (m_fallbackBanner == nullptr || m_fallbackLabel == nullptr) {
		return;
	}

	lv_label_set_text(m_fallbackLabel, message.c_str());
	lv_obj_remove_flag(m_fallbackBanner, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(m_fallbackBanner);
}

void WallpaperEngineApp::hideFallbackBanner() {
	if (m_fallbackBanner != nullptr) {
		lv_obj_add_flag(m_fallbackBanner, LV_OBJ_FLAG_HIDDEN);
	}
}

void WallpaperEngineApp::retryLastWallpaper() {
	if (m_lastFailedSource.empty()) {
		return;
	}

	flx::system::WallpaperManager::getInstance().setWallpaper(m_lastFailedSource);
	Log::info(TAG_WPE_APP,
		"Retrying wallpaper apply type=%s source=%s",
		m_lastFailedType.c_str(),
		m_lastFailedSource.c_str());
}

void WallpaperEngineApp::openSourceSelector() {
	showDynamicPage();
}

void WallpaperEngineApp::pollBannerDismissState() {
	if (m_fallbackBanner == nullptr || lv_obj_has_flag(m_fallbackBanner, LV_OBJ_FLAG_HIDDEN)) {
		return;
	}

	if (m_lastFailedSource.empty()) {
		return;
	}

	auto& manager = flx::system::WallpaperManager::getInstance();
	std::string const activeType = manager.getWallpaperTypeObservable().get();
	std::string const activeSource = manager.getWallpaperSourceObservable().get();

	bool const matchedRequested = (activeType == m_lastFailedType) && (activeSource == m_lastFailedSource);
	if (matchedRequested && lv_tick_elaps(m_lastErrorTickMs) >= 1000U) {
		hideFallbackBanner();
	}
}

void WallpaperEngineApp::navigateFromIntent(const flx::apps::Intent& intent) {
	std::string const route = intent.extras.getStringOr("route", "");
	if (route == "dynamic") {
		showDynamicPage();
		return;
	}

	showDynamicPage();
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void WallpaperEngineApp::hideAllPages() {
	if (m_dynamicPage != nullptr) {
		lv_obj_add_flag(m_dynamicPage, LV_OBJ_FLAG_HIDDEN);
	}
}

void WallpaperEngineApp::showDynamicPage() {
	hideAllPages();
	if (m_container == nullptr) {
		return;
	}

	if (m_dynamicPage == nullptr) {
		m_dynamicPage = lv_obj_create(m_container);
		lv_obj_set_size(m_dynamicPage, lv_pct(100), lv_pct(100));
		lv_obj_set_style_border_width(m_dynamicPage, 0, 0);
		lv_obj_set_flex_flow(m_dynamicPage, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_style_pad_all(m_dynamicPage, 16, 0);
		lv_obj_set_style_pad_row(m_dynamicPage, 8, 0);

		lv_obj_t* title = lv_label_create(m_dynamicPage);
		lv_label_set_text(title, "Dynamic Wallpapers");

		lv_obj_t* body = lv_label_create(m_dynamicPage);
		lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
		lv_obj_set_width(body, lv_pct(100));
		lv_label_set_text(
			body,
			"Dynamic wallpapers are not available in this build yet. Use Static to choose a wallpaper image.");

		lv_obj_t* backBtn = lv_button_create(m_dynamicPage);
		lv_obj_t* backLabel = lv_label_create(backBtn);
		lv_label_set_text(backLabel, "Close");
		lv_obj_add_event_cb(
			backBtn,
			[](lv_event_t* e) {
				auto* app = static_cast<WallpaperEngineApp*>(lv_event_get_user_data(e));
				if (app != nullptr) {
					flx::apps::AppManager::getInstance().finishApp(
						app->getContext() ? app->getContext()->getLaunchId() : flx::apps::LAUNCH_ID_INVALID);
				}
			},
			LV_EVENT_CLICKED,
			this);
	} else {
		lv_obj_remove_flag(m_dynamicPage, LV_OBJ_FLAG_HIDDEN);
	}
}

// Effects/Adaptive pages removed; no-op implementations omitted.

} // namespace System::Apps
