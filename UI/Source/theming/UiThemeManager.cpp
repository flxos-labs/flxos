#include <flx/system/managers/ThemeManager.hpp>
#include <flx/ui/theming/UiThemeManager.hpp>

namespace flx::ui::theming {

UiThemeManager::~UiThemeManager() {
	lv_subject_deinit(&m_theme_subject);
	lv_subject_deinit(&m_glass_enabled_subject);
	lv_subject_deinit(&m_transparency_enabled_subject);
	lv_subject_deinit(&m_wallpaper_enabled_subject);
}

void UiThemeManager::init() {
	auto& sys = flx::system::ThemeManager::getInstance();

	// Initialize LVGL subjects
	lv_subject_init_int(&m_theme_subject, 1); // Default to Dark?
	lv_subject_init_int(&m_glass_enabled_subject, 0);
	lv_subject_init_int(&m_transparency_enabled_subject, 0);
	lv_subject_init_int(&m_wallpaper_enabled_subject, 0);

	// Subscribe to System Observables
	sys.getThemeObservable().subscribe([this](const int32_t& val) {
		syncFromSystem(val, &m_theme_subject);
	});

	sys.getGlassEnabledObservable().subscribe([this](const int32_t& val) {
		syncFromSystem(val, &m_glass_enabled_subject);
	});

	sys.getTransparencyEnabledObservable().subscribe([this](const int32_t& val) {
		syncFromSystem(val, &m_transparency_enabled_subject);
	});

	sys.getWallpaperEnabledObservable().subscribe([this](const int32_t& val) {
		syncFromSystem(val, &m_wallpaper_enabled_subject);
	});
}

lv_subject_t* UiThemeManager::getThemeSubject() { return &m_theme_subject; }
lv_subject_t* UiThemeManager::getGlassEnabledSubject() { return &m_glass_enabled_subject; }
lv_subject_t* UiThemeManager::getTransparencyEnabledSubject() { return &m_transparency_enabled_subject; }
lv_subject_t* UiThemeManager::getWallpaperEnabledSubject() { return &m_wallpaper_enabled_subject; }

void UiThemeManager::syncFromSystem(int32_t value, lv_subject_t* subject) {
	lv_subject_set_int(subject, value);
}

} // namespace flx::ui::theming
