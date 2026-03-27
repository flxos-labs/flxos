#include "DynamicPage.hpp"

#include <algorithm>
#include <cstdio>
#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <string>

using namespace flx::ui::common;

namespace System::Apps::WallpaperEngine {

struct AlgorithmEntry {
	const char* label;
	const char* source;
	const char* description;
};

static constexpr AlgorithmEntry ALGORITHMS[] = {
	{"Plasma", "algo://plasma", "Vibrant sinusoidal colour interference"},
	{"Cloud Noise", "algo://perlin", "Smooth organic noise cloud pattern"},
	{"Gradient Waves", "algo://gradient", "Animated colour gradient waves"},
};

static constexpr const char* PALETTES[] = {"vivid", "cool", "sunset"};

DynamicPage::DynamicPage(lv_obj_t* parent, std::function<void()> onBack)
	: m_onBack(std::move(onBack)) {

	m_container = create_page_container(parent);

	// Header
	lv_obj_t* backBtn = nullptr;
	create_header(m_container, "Dynamic Wallpapers", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	lv_obj_t* list = create_settings_list(m_container);
	lv_list_add_text(list, "Select Algorithm");

	for (const auto& algo : ALGORITHMS) {
		lv_obj_t* btn = add_list_btn(list, LV_SYMBOL_LOOP, algo.label);
		lv_obj_set_flex_grow(lv_obj_get_child(btn, 1), 1);

		lv_obj_t* descLabel = lv_label_create(btn);
		lv_label_set_text(descLabel, algo.description);
		lv_obj_set_style_text_opa(descLabel, LV_OPA_60, 0);

		// Store source pointer directly — string literals have static storage duration
		lv_obj_set_user_data(btn, static_cast<void*>(const_cast<char*>(algo.source)));

		lv_obj_add_event_cb(
			btn,
			[](lv_event_t* e) {
				auto* self = static_cast<DynamicPage*>(lv_event_get_user_data(e));
				if (self == nullptr) {
					return;
				}
				const char* source = static_cast<const char*>(
					lv_obj_get_user_data(lv_event_get_target_obj(e)));
				if (source != nullptr) {
					std::string src = source;
					size_t const sep = src.find("algo://");
					if (sep == 0) {
						self->setAlgorithm(src.substr(7));
					} else {
						self->setAlgorithm(src);
					}
					self->applyDynamicSource();
				}
			},
			LV_EVENT_CLICKED, this);
	}

	lv_list_add_text(list, "Parameters");

	lv_obj_t* speedBtn = add_list_btn(list, LV_SYMBOL_PLAY, "Speed");
	m_speedSlider = lv_slider_create(speedBtn);
	lv_obj_set_flex_grow(m_speedSlider, 1);
	lv_slider_set_range(m_speedSlider, 10, 200);
	lv_slider_set_value(m_speedSlider, m_speed, LV_ANIM_OFF);
	lv_obj_add_event_cb(
		m_speedSlider,
		[](lv_event_t* e) {
			auto* self = static_cast<DynamicPage*>(lv_event_get_user_data(e));
			if (self == nullptr) {
				return;
			}
			self->m_speed = lv_slider_get_value(lv_event_get_target_obj(e));
			self->applyDynamicSource();
		},
		LV_EVENT_VALUE_CHANGED,
		this);

	lv_obj_t* noiseBtn = add_list_btn(list, LV_SYMBOL_SETTINGS, "Noise Scale");
	m_noiseSlider = lv_slider_create(noiseBtn);
	lv_obj_set_flex_grow(m_noiseSlider, 1);
	lv_slider_set_range(m_noiseSlider, 25, 200);
	lv_slider_set_value(m_noiseSlider, m_noise, LV_ANIM_OFF);
	lv_obj_add_event_cb(
		m_noiseSlider,
		[](lv_event_t* e) {
			auto* self = static_cast<DynamicPage*>(lv_event_get_user_data(e));
			if (self == nullptr) {
				return;
			}
			self->m_noise = lv_slider_get_value(lv_event_get_target_obj(e));
			self->applyDynamicSource();
		},
		LV_EVENT_VALUE_CHANGED,
		this);

	lv_obj_t* particlesBtn = add_list_btn(list, LV_SYMBOL_PLUS, "Particle Density");
	m_particlesSlider = lv_slider_create(particlesBtn);
	lv_obj_set_flex_grow(m_particlesSlider, 1);
	lv_slider_set_range(m_particlesSlider, 16, 256);
	lv_slider_set_value(m_particlesSlider, m_particles, LV_ANIM_OFF);
	lv_obj_add_event_cb(
		m_particlesSlider,
		[](lv_event_t* e) {
			auto* self = static_cast<DynamicPage*>(lv_event_get_user_data(e));
			if (self == nullptr) {
				return;
			}
			self->m_particles = lv_slider_get_value(lv_event_get_target_obj(e));
			self->applyDynamicSource();
		},
		LV_EVENT_VALUE_CHANGED,
		this);

	lv_obj_t* paletteBtn = add_list_btn(list, LV_SYMBOL_TINT, "Palette");
	lv_obj_set_flex_grow(lv_obj_get_child(paletteBtn, 1), 1);
	lv_obj_t* paletteValueBtn = lv_button_create(paletteBtn);
	lv_obj_set_size(paletteValueBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	m_paletteLabel = lv_label_create(paletteValueBtn);
	lv_label_set_text(m_paletteLabel, m_palette.c_str());
	lv_obj_add_event_cb(
		paletteValueBtn,
		[](lv_event_t* e) {
			auto* self = static_cast<DynamicPage*>(lv_event_get_user_data(e));
			if (self == nullptr) {
				return;
			}
			for (size_t i = 0; i < (sizeof(PALETTES) / sizeof(PALETTES[0])); ++i) {
				if (self->m_palette == PALETTES[i]) {
					size_t const next = (i + 1) % (sizeof(PALETTES) / sizeof(PALETTES[0]));
					self->m_palette = PALETTES[next];
					break;
				}
			}
			if (self->m_paletteLabel != nullptr) {
				lv_label_set_text(self->m_paletteLabel, self->m_palette.c_str());
			}
			self->applyDynamicSource();
		},
		LV_EVENT_CLICKED,
		this);

	syncFromCurrentWallpaper();

	lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
}

void DynamicPage::show() {
	if (m_container != nullptr) {
		syncFromCurrentWallpaper();
		lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

void DynamicPage::hide() {
	if (m_container != nullptr) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

void DynamicPage::setAlgorithm(const std::string& algorithm) {
	if (algorithm.empty()) {
		m_algorithm = "plasma";
		return;
	}
	m_algorithm = algorithm;
}

void DynamicPage::applyDynamicSource() const {
	char source[192];
	std::snprintf(
		source,
		sizeof(source),
		"algo://%s?speed=%d&palette=%s&particles=%d&noise=%d",
		m_algorithm.c_str(),
		m_speed,
		m_palette.c_str(),
		m_particles,
		m_noise);

	flx::system::WallpaperManager::getInstance().setWallpaper(source, "dynamic");
}

void DynamicPage::syncFromCurrentWallpaper() {
	auto& manager = flx::system::WallpaperManager::getInstance();
	if (manager.getWallpaperTypeObservable().get() != "dynamic") {
		if (m_speedSlider) lv_slider_set_value(m_speedSlider, m_speed, LV_ANIM_OFF);
		if (m_noiseSlider) lv_slider_set_value(m_noiseSlider, m_noise, LV_ANIM_OFF);
		if (m_particlesSlider) lv_slider_set_value(m_particlesSlider, m_particles, LV_ANIM_OFF);
		if (m_paletteLabel) lv_label_set_text(m_paletteLabel, m_palette.c_str());
		return;
	}

	std::string const src = manager.getWallpaperSourceObservable().get();
	std::string parsed = src;
	if (parsed.rfind("algo://", 0) == 0) {
		parsed = parsed.substr(7);
	}

	size_t const queryPos = parsed.find('?');
	if (queryPos == std::string::npos) {
		setAlgorithm(parsed);
	} else {
		setAlgorithm(parsed.substr(0, queryPos));
		std::string const query = parsed.substr(queryPos + 1);
		size_t start = 0;
		while (start < query.size()) {
			size_t end = query.find('&', start);
			if (end == std::string::npos) {
				end = query.size();
			}
			std::string pair = query.substr(start, end - start);
			size_t eq = pair.find('=');
			if (eq != std::string::npos && eq + 1 < pair.size()) {
				std::string const key = pair.substr(0, eq);
				std::string const value = pair.substr(eq + 1);
				if (key == "speed") {
					m_speed = std::clamp(std::atoi(value.c_str()), 10, 200);
				} else if (key == "noise") {
					m_noise = std::clamp(std::atoi(value.c_str()), 25, 200);
				} else if (key == "particles") {
					m_particles = std::clamp(std::atoi(value.c_str()), 16, 256);
				} else if (key == "palette") {
					if (value == "cool" || value == "sunset" || value == "vivid") {
						m_palette = value;
					}
				}
			}
			start = end + 1;
		}
	}

	if (m_speedSlider) lv_slider_set_value(m_speedSlider, m_speed, LV_ANIM_OFF);
	if (m_noiseSlider) lv_slider_set_value(m_noiseSlider, m_noise, LV_ANIM_OFF);
	if (m_particlesSlider) lv_slider_set_value(m_particlesSlider, m_particles, LV_ANIM_OFF);
	if (m_paletteLabel) lv_label_set_text(m_paletteLabel, m_palette.c_str());
}

} // namespace System::Apps::WallpaperEngine
