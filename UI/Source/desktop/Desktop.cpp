#include <ctime>
#include <esp_heap_caps.h>
#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/core/Bundle.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/system/SystemManager.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/GuiTask.hpp>
#include <flx/ui/desktop/Desktop.hpp>
#include <flx/ui/desktop/window_manager/WindowManager.hpp>
#include <flx/ui/managers/FocusManager.hpp>
#include <flx/ui/theming/StyleUtils.hpp>
#include <flx/ui/theming/UiThemeManager.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>
#include <flx/ui/wallpaper/providers/AnimatedGifProvider.hpp>
#include <flx/ui/wallpaper/providers/DynamicProvider.hpp>
#include <flx/ui/wallpaper/providers/LottieProvider.hpp>
#include <flx/ui/wallpaper/providers/StaticImageProvider.hpp>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

static constexpr std::string_view TAG = "Desktop";
static constexpr uint32_t WALLPAPER_GATE_WINDOW_MS = 5000;
static constexpr uint32_t WALLPAPER_BENCHMARK_WINDOW_MS = 1000;
static constexpr float WALLPAPER_GATE_MIN_FPS = 24.0f;
static constexpr uint32_t WALLPAPER_GATE_GIF_MAX_EXTRA_HEAP_BYTES = 180 * 1024;
static constexpr uint32_t WALLPAPER_GATE_LOTTIE_MAX_EXTRA_HEAP_BYTES = 220 * 1024;

namespace UI {

Desktop& Desktop::getInstance() {
	static Desktop instance;
	return instance;
}

Desktop::Desktop()
	: m_screen(nullptr), m_wallpaper(nullptr),
	  m_wallpaper_icon(nullptr), m_window_container(nullptr),
	  m_statusBarModule(nullptr), m_status_bar(nullptr),
	  m_floatingNotificationsModule(nullptr),
	  m_dockModule(nullptr), m_dock(nullptr),
	  m_launcherModule(nullptr), m_launcher(nullptr),
	  m_quickAccessPanelModule(nullptr), m_quick_access_panel(nullptr),
	  m_notificationPanelModule(nullptr), m_notification_panel(nullptr),
	  m_notification_list(nullptr), m_clear_all_btn(nullptr), m_greetings(nullptr), m_app_container(nullptr),
	  m_swipeManagerModule(nullptr) {}

Desktop::~Desktop() {
	if (m_wallpaperProvider) {
		m_wallpaperProvider->destroy();
		m_wallpaperProvider.reset();
	}
}

void Desktop::init() {
	Log::info(TAG, "Initializing Desktop Environment...");
	m_screen = lv_obj_create(NULL);
	lv_obj_remove_style_all(m_screen);
	lv_obj_set_flex_flow(m_screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_screen_load(m_screen);

	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
	auto& uiTheme = flx::ui::theming::UiThemeManager::getInstance();

	if (!flx::system::SystemManager::getInstance().isSafeMode()) {
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

		// Theme Change Observer
		lv_subject_add_observer_obj(
			uiTheme.getThemeSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* wp = lv_observer_get_target_obj(observer);
				if (wp) {
					ThemeType theme = (ThemeType)lv_subject_get_int(subject);
					ThemeConfig cfg = Themes::GetConfig(theme);
					lv_obj_set_style_bg_color(wp, cfg.primary, 0);
				}
			},
			m_wallpaper, nullptr);
	}
	lv_obj_set_style_bg_opa(m_screen, UiConstants::OPA_COVER, 0);

	m_statusBarModule.reset(new UI::Modules::StatusBar(m_screen));
	m_status_bar = m_statusBarModule->getObj();
	m_floatingNotificationsModule.reset(new UI::Modules::FloatingNotifications(m_screen, m_status_bar));

	m_window_container = lv_obj_create(m_screen);
	lv_obj_remove_style_all(m_window_container);
	lv_obj_remove_flag(m_window_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_width(m_window_container, lv_pct(100));
	lv_obj_set_flex_grow(m_window_container, 1);

	UI::Modules::Dock::Callbacks dockCallbacks {
		.onStartClick = [this]() { this->on_start_click(); },
		.onUpClick = [this]() { this->on_up_click(); }};
	m_dockModule.reset(new UI::Modules::Dock(m_screen, dockCallbacks));
	m_dock = m_dockModule->getObj();
	m_app_container = m_dockModule->getAppContainer();

	flx::ui::window_manager::WindowManager::getInstance().init(m_window_container, m_app_container, m_screen, m_status_bar, m_dock);

	if (!flx::system::SystemManager::getInstance().isSafeMode()) {
		if (m_wallpaper) {
			m_greetings = lv_label_create(m_wallpaper);
			lv_label_set_text(m_greetings, "Hey !");
			lv_obj_align_to(m_greetings, m_dock, LV_ALIGN_OUT_TOP_RIGHT, -lv_dpx(UiConstants::OFFSET_TINY), -lv_dpx(UiConstants::OFFSET_TINY));
		}

		m_launcherModule.reset(new UI::Modules::Launcher(m_screen, m_dock, on_app_click, this));
		m_launcher = m_launcherModule->getObj();

		m_quickAccessPanelModule.reset(new UI::Modules::QuickAccessPanel(m_screen, m_dock));
		m_quick_access_panel = m_quickAccessPanelModule->getObj();

		m_notificationPanelModule.reset(new UI::Modules::NotificationPanel(m_screen, m_status_bar));
		m_notification_panel = m_notificationPanelModule->getObj();
		m_notification_list = m_notificationPanelModule->getList();

		UI::Modules::SwipeManager::Config swipeConfig {
			.screen = m_screen,
			.statusBar = m_status_bar,
			.notificationPanel = m_notification_panel,
			.notificationList = m_notification_list};
		m_swipeManagerModule.reset(new UI::Modules::SwipeManager(swipeConfig));

		// Register panels with FocusManager
		flx::ui::FocusManager::getInstance().registerPanel(m_launcher);
		flx::ui::FocusManager::getInstance().registerPanel(m_quick_access_panel);
		flx::ui::FocusManager::getInstance().registerPanel(m_notification_panel);
		flx::ui::FocusManager::getInstance().setNotificationPanel(m_notification_panel);

		// Register AppManager callbacks
		flx::apps::AppManager::getInstance().setWindowCallbacks(
			[this](const std::string& pkg) { this->openApp(pkg); },
			[this](const std::string& pkg) { this->closeApp(pkg); });

		flx::apps::AppManager::getInstance().setGuiCallbacks(
			[]() { flx::ui::GuiTask::lock(); },
			[]() { flx::ui::GuiTask::unlock(); });

		m_rotationObserver = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(flx::system::DisplayManager::getInstance().getRotationObservable());
		lv_subject_add_observer(
			m_rotationObserver->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* /*subject*/) {
				auto* instance = (Desktop*)lv_observer_get_user_data(observer);
				if (instance && instance->m_screen) {
					Log::info(TAG, "Realigning panels due to rotation");
					lv_obj_update_layout(instance->m_screen);
					instance->realign_panels();
				}
			},
			this);
		Log::info(TAG, "DE initialization complete");
	}
}

void Desktop::onFrame(uint32_t delta_ms) {
	syncWallpaperProvider(delta_ms);
	flx::system::WallpaperManager::getInstance().onFrame(delta_ms);
}

void Desktop::syncWallpaperProvider(uint32_t delta_ms) {
	if (m_wallpaper == nullptr) {
		return;
	}

	auto updatePlaceholderIconVisibility = [this](bool show) {
		if (!m_wallpaper_icon) {
			return;
		}
		if (show) {
			lv_obj_clear_flag(m_wallpaper_icon, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_add_flag(m_wallpaper_icon, LV_OBJ_FLAG_HIDDEN);
		}
	};

	auto& wallpaperManager = flx::system::WallpaperManager::getInstance();
	bool const enabled = wallpaperManager.getWallpaperEnabledObservable().get() != 0;
	std::string type = wallpaperManager.getWallpaperTypeObservable().get();
	std::string const source = wallpaperManager.getWallpaperSourceObservable().get();
	int32_t const speed = wallpaperManager.getAnimationSpeedObservable().get();

	if (!enabled) {
		if (m_wallpaperProvider) {
			m_wallpaperProvider->destroy();
			m_wallpaperProvider.reset();
		}
		m_wallpaperProviderType.clear();
		m_wallpaperProviderSource.clear();
		m_wallpaperProviderSpeed = -1;
		m_lastWallpaperFailureKey.clear();
		m_providerBaselineHeapBytes = 0;
		m_providerPerfWindowMs = 0;
		m_providerPerfFrameCount = 0;
		m_providerPerfKey.clear();
		m_providerBenchmarkWindowMs = 0;
		m_providerBenchmarkFrameCount = 0;
		m_providerBenchmarkTotalFrameMs = 0;
		m_providerBenchmarkMaxFrameMs = 0;
		m_providerBenchmarkFrameSamples.clear();
		m_providerBenchmarkBaselineHeapBytes = 0;
		m_providerBenchmarkKey.clear();
		m_providerBenchmarkWriteFailed = false;
		updatePlaceholderIconVisibility(true);
		return;
	}

	if (type.empty()) {
		type = "static";
	}

	if (!m_wallpaperProvider || m_wallpaperProviderType != type) {
		if (m_wallpaperProvider) {
			m_wallpaperProvider->destroy();
			m_wallpaperProvider.reset();
		}

		if (type == "animated" || type == "gif") {
			m_wallpaperProvider = std::make_unique<flx::ui::wallpaper::AnimatedGifProvider>();
		} else if (type == "lottie") {
			m_wallpaperProvider = std::make_unique<flx::ui::wallpaper::LottieProvider>();
		} else if (type == "dynamic") {
			m_wallpaperProvider = std::make_unique<flx::ui::wallpaper::DynamicProvider>();
		} else {
			type = "static";
			m_wallpaperProvider = std::make_unique<flx::ui::wallpaper::StaticImageProvider>();
		}

		m_providerBaselineHeapBytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
		m_wallpaperProvider->initialize();
		m_wallpaperProviderType = type;
		m_wallpaperProviderSource.clear();
		m_wallpaperProviderSpeed = -1;
		m_providerPerfWindowMs = 0;
		m_providerPerfFrameCount = 0;
		m_providerPerfKey.clear();
		m_providerBenchmarkWindowMs = 0;
		m_providerBenchmarkFrameCount = 0;
		m_providerBenchmarkTotalFrameMs = 0;
		m_providerBenchmarkMaxFrameMs = 0;
		m_providerBenchmarkFrameSamples.clear();
		m_providerBenchmarkBaselineHeapBytes = 0;
		m_providerBenchmarkKey.clear();
		m_providerBenchmarkWriteFailed = false;
	}

	if (!m_wallpaperProvider) {
		updatePlaceholderIconVisibility(true);
		return;
	}

	if (m_wallpaperProviderSource != source) {
		m_wallpaperProvider->setSource(source);
		m_wallpaperProviderSource = source;
		m_providerPerfWindowMs = 0;
		m_providerPerfFrameCount = 0;
		m_providerPerfKey.clear();
		m_providerBaselineHeapBytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
		m_providerBenchmarkWindowMs = 0;
		m_providerBenchmarkFrameCount = 0;
		m_providerBenchmarkTotalFrameMs = 0;
		m_providerBenchmarkMaxFrameMs = 0;
		m_providerBenchmarkFrameSamples.clear();
		m_providerBenchmarkBaselineHeapBytes = m_providerBaselineHeapBytes;
		m_providerBenchmarkKey = type + "|" + source;
		m_providerBenchmarkWriteFailed = false;
	}

	if (m_wallpaperProviderSpeed != speed) {
		m_wallpaperProvider->setAnimationSpeed(speed);
		m_wallpaperProviderSpeed = speed;
	}

	updatePlaceholderIconVisibility(source.empty());

	m_wallpaperProvider->render(m_wallpaper, delta_ms);

	if (!source.empty()) {
		std::string const providerError = m_wallpaperProvider->getLastError();
		if (!providerError.empty() && !m_wallpaperProvider->isReady()) {
			handleWallpaperProviderFailure(type, source, providerError);
			return;
		}

		evaluateWallpaperAcceptanceGate(type, source, delta_ms);

		if (m_wallpaperProvider->isReady()) {
			m_lastWallpaperFailureKey.clear();
		}
	}
}

void Desktop::handleWallpaperProviderFailure(const std::string& requestedType, const std::string& source, const std::string& error) {
	if (requestedType.empty() || requestedType == "static") {
		return;
	}

	std::string const failureKey = requestedType + "|" + source + "|" + error;
	if (failureKey == m_lastWallpaperFailureKey) {
		return;
	}
	m_lastWallpaperFailureKey = failureKey;

	Log::warn(TAG, "Wallpaper provider '%s' failed: %s. Falling back to static.", requestedType.c_str(), error.c_str());

	std::string reasonCode = "render_error";
	size_t const sep = error.find(':');
	if (sep != std::string::npos && sep > 0) {
		reasonCode = error.substr(0, sep);
	} else if (!error.empty()) {
		reasonCode = error;
	}

	flx::core::Bundle data;
	data.putString("requested_type", requestedType);
	data.putString("source", source);
	data.putString("error", error);
	data.putString("reason_code", reasonCode);
	data.putString("fallback_type", "static");
	flx::core::EventBus::getInstance().publish("wallpaper.error", data);

	flx::system::NotificationManager::getInstance().addNotification(
		"Wallpaper Fallback",
		"Failed to load wallpaper. Switched to static mode.",
		"Wallpaper",
		LV_SYMBOL_WARNING,
		2);

	auto& wallpaperManager = flx::system::WallpaperManager::getInstance();
	wallpaperManager.setWallpaper(source, "static");
}

void Desktop::evaluateWallpaperAcceptanceGate(const std::string& type, const std::string& source, uint32_t delta_ms) {
	if (!(type == "animated" || type == "gif" || type == "lottie")) {
		return;
	}

	if (source.empty()) {
		return;
	}

	std::string const key = type + "|" + source;
	if (m_providerPerfKey != key) {
		m_providerPerfKey = key;
		m_providerPerfWindowMs = 0;
		m_providerPerfFrameCount = 0;
		m_providerBaselineHeapBytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	}

	auto& wallpaperManager = flx::system::WallpaperManager::getInstance();
	bool const benchmarkEnabled = wallpaperManager.getBenchmarkEnabledObservable().get() != 0;
	if (benchmarkEnabled) {
		if (m_providerBenchmarkKey != key) {
			m_providerBenchmarkKey = key;
			m_providerBenchmarkWindowMs = 0;
			m_providerBenchmarkFrameCount = 0;
			m_providerBenchmarkTotalFrameMs = 0;
			m_providerBenchmarkMaxFrameMs = 0;
			m_providerBenchmarkFrameSamples.clear();
			m_providerBenchmarkBaselineHeapBytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
			m_providerBenchmarkWriteFailed = false;
		}

		m_providerBenchmarkWindowMs += delta_ms;
		m_providerBenchmarkFrameCount += 1;
		m_providerBenchmarkTotalFrameMs += static_cast<uint64_t>(delta_ms);
		m_providerBenchmarkFrameSamples.push_back(delta_ms);
		if (delta_ms > m_providerBenchmarkMaxFrameMs) {
			m_providerBenchmarkMaxFrameMs = delta_ms;
		}

		if (m_providerBenchmarkWindowMs >= WALLPAPER_BENCHMARK_WINDOW_MS) {
			float benchmarkFps = 0.0f;
			if (m_providerBenchmarkWindowMs > 0) {
				benchmarkFps = (static_cast<float>(m_providerBenchmarkFrameCount) * 1000.0f)
					/ static_cast<float>(m_providerBenchmarkWindowMs);
			}

			float averageFrameMs = 0.0f;
			if (m_providerBenchmarkFrameCount > 0) {
				averageFrameMs = static_cast<float>(m_providerBenchmarkTotalFrameMs)
					/ static_cast<float>(m_providerBenchmarkFrameCount);
			}

			uint32_t p95FrameMs = 0;
			if (!m_providerBenchmarkFrameSamples.empty()) {
				size_t const p95Index = ((m_providerBenchmarkFrameSamples.size() - 1U) * 95U) / 100U;
				std::nth_element(
					m_providerBenchmarkFrameSamples.begin(),
					m_providerBenchmarkFrameSamples.begin() + static_cast<std::ptrdiff_t>(p95Index),
					m_providerBenchmarkFrameSamples.end());
				p95FrameMs = m_providerBenchmarkFrameSamples[p95Index];
			}

			uint32_t const benchCurrentHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
			uint32_t const benchBaselineHeap = m_providerBenchmarkBaselineHeapBytes;
			uint32_t const benchExtraHeapBytes = (benchBaselineHeap > benchCurrentHeap)
				? (benchBaselineHeap - benchCurrentHeap)
				: 0;
			uint32_t constexpr watchdogIncidents = 0;

			FILE* benchmarkFile = std::fopen(m_providerBenchmarkPath.c_str(), "a");
			if (benchmarkFile != nullptr) {
				if (!m_providerBenchmarkHeaderWritten) {
					std::fprintf(benchmarkFile, "timestamp_ms,type,fps,avg_frame_ms,p95_frame_ms,max_frame_ms,extra_heap_bytes,watchdog_incidents,source\n");
					m_providerBenchmarkHeaderWritten = true;
				}

				std::string escapedSource;
				escapedSource.reserve(source.size() + 2);
				escapedSource.push_back('"');
				for (char const ch : source) {
					if (ch == '"') {
						escapedSource += "\"\"";
					} else if (ch == '\n' || ch == '\r') {
						escapedSource.push_back(' ');
					} else {
						escapedSource.push_back(ch);
					}
				}
				escapedSource.push_back('"');

				std::fprintf(
					benchmarkFile,
					"%llu,%s,%.2f,%.2f,%u,%u,%u,%u,%s\n",
					static_cast<unsigned long long>(lv_tick_get()),
					type.c_str(),
					benchmarkFps,
					averageFrameMs,
					static_cast<unsigned>(p95FrameMs),
					static_cast<unsigned>(m_providerBenchmarkMaxFrameMs),
					static_cast<unsigned>(benchExtraHeapBytes),
					static_cast<unsigned>(watchdogIncidents),
					escapedSource.c_str());
				std::fclose(benchmarkFile);
			} else if (!m_providerBenchmarkWriteFailed) {
				Log::warn(TAG,
					"Wallpaper benchmark enabled but unable to write CSV at '%s'",
					m_providerBenchmarkPath.c_str());
				m_providerBenchmarkWriteFailed = true;
			}

			Log::info(TAG,
				"Wallpaper benchmark type=%s fps=%.2f avg_ms=%.2f p95_ms=%u max_ms=%u extra_heap=%u",
				type.c_str(),
				benchmarkFps,
				averageFrameMs,
				static_cast<unsigned>(p95FrameMs),
				static_cast<unsigned>(m_providerBenchmarkMaxFrameMs),
				static_cast<unsigned>(benchExtraHeapBytes));

			m_providerBenchmarkWindowMs = 0;
			m_providerBenchmarkFrameCount = 0;
			m_providerBenchmarkTotalFrameMs = 0;
			m_providerBenchmarkMaxFrameMs = 0;
			m_providerBenchmarkFrameSamples.clear();
			m_providerBenchmarkBaselineHeapBytes = benchCurrentHeap;
		}
	} else if (m_providerBenchmarkWindowMs != 0 || m_providerBenchmarkFrameCount != 0) {
		m_providerBenchmarkWindowMs = 0;
		m_providerBenchmarkFrameCount = 0;
		m_providerBenchmarkTotalFrameMs = 0;
		m_providerBenchmarkMaxFrameMs = 0;
		m_providerBenchmarkFrameSamples.clear();
		m_providerBenchmarkBaselineHeapBytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
		m_providerBenchmarkKey = key;
		m_providerBenchmarkWriteFailed = false;
	}

	m_providerPerfWindowMs += delta_ms;
	m_providerPerfFrameCount += 1;

	if (m_providerPerfWindowMs < WALLPAPER_GATE_WINDOW_MS) {
		return;
	}

	float fps = 0.0f;
	if (m_providerPerfWindowMs > 0) {
		fps = (static_cast<float>(m_providerPerfFrameCount) * 1000.0f) / static_cast<float>(m_providerPerfWindowMs);
	}

	uint32_t const currentHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	uint32_t const baselineHeap = m_providerBaselineHeapBytes;
	uint32_t const extraHeapBytes = (baselineHeap > currentHeap) ? (baselineHeap - currentHeap) : 0;

	uint32_t maxExtraHeapBytes = WALLPAPER_GATE_GIF_MAX_EXTRA_HEAP_BYTES;
	const char* gateReason = "Animated acceptance gate failed";
	if (type == "lottie") {
		maxExtraHeapBytes = WALLPAPER_GATE_LOTTIE_MAX_EXTRA_HEAP_BYTES;
		gateReason = "Lottie acceptance gate failed";
	}

	bool const fpsOk = fps >= WALLPAPER_GATE_MIN_FPS;
	bool const heapOk = extraHeapBytes <= maxExtraHeapBytes;

	if (!fpsOk || !heapOk) {
		Log::warn(TAG,
			"Wallpaper acceptance gate failed for type=%s source=%s: fps=%.2f (min %.2f), extra_heap=%u (max %u)",
			type.c_str(),
			source.c_str(),
			fps,
			WALLPAPER_GATE_MIN_FPS,
			static_cast<unsigned>(extraHeapBytes),
			static_cast<unsigned>(maxExtraHeapBytes));
		handleWallpaperProviderFailure(type, source, gateReason);
		return;
	}

	Log::info(TAG,
		"Wallpaper acceptance gate passed for type=%s source=%s: fps=%.2f, extra_heap=%u",
		type.c_str(),
		source.c_str(),
		fps,
		static_cast<unsigned>(extraHeapBytes));

	m_providerPerfWindowMs = 0;
	m_providerPerfFrameCount = 0;
	m_providerBaselineHeapBytes = currentHeap;
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
	if (m_floatingNotificationsModule) {
		m_floatingNotificationsModule->realign();
	}
}

void Desktop::on_start_click() {
	if (m_launcher) {
		realign_panels();
		flx::ui::FocusManager::getInstance().togglePanel(m_launcher);
	}
}

void Desktop::on_up_click() {
	if (m_quick_access_panel) {
		realign_panels();
		flx::ui::FocusManager::getInstance().togglePanel(m_quick_access_panel);
	}
}

void Desktop::on_app_click(lv_event_t* e) {
	lv_obj_t* btn = lv_event_get_target_obj(e);

	auto* appPtr = (flx::apps::App*)lv_obj_get_user_data(btn);
	if (!appPtr) {
		return;
	}

	std::string packageName = appPtr->getPackageName();

	flx::ui::FocusManager::getInstance().dismissAllPanels();
	flx::apps::AppManager::getInstance().startApp(
		flx::apps::Intent::forApp(packageName));
}

void Desktop::openApp(const std::string& packageName) {
	Log::info(TAG, "Requesting WM to open app: %s", packageName.c_str());
	flx::ui::window_manager::WindowManager::getInstance().openApp(packageName);
}

void Desktop::closeApp(const std::string& packageName) {
	flx::ui::window_manager::WindowManager::getInstance().closeApp(packageName);
}

void Desktop::update_notification_list() {
	if (m_notificationPanelModule) m_notificationPanelModule->update_list();
}

} // namespace UI
