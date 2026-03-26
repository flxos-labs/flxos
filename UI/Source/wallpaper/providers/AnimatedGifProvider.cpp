#include <flx/ui/wallpaper/providers/AnimatedGifProvider.hpp>

#include <algorithm>

namespace flx::ui::wallpaper {

void AnimatedGifProvider::initialize() {
	m_last_error.clear();
	m_ready = false;
	m_animation_speed = 50;
}

void AnimatedGifProvider::destroy() {
	if (m_gif_obj != nullptr) {
		lv_obj_delete(m_gif_obj);
		m_gif_obj = nullptr;
	}
	m_parent = nullptr;
	m_source.clear();
	m_last_error.clear();
	m_ready = false;
}

void AnimatedGifProvider::render(lv_obj_t* parent, uint32_t /*elapsed_ms*/) {
	if (parent == nullptr) {
		m_last_error = "Parent object is null";
		m_ready = false;
		return;
	}

	if (m_parent != parent) {
		if (m_gif_obj != nullptr) {
			lv_obj_delete(m_gif_obj);
			m_gif_obj = nullptr;
		}
		m_parent = parent;
	}

	if (m_gif_obj != nullptr) {
		return;
	}

#if LV_USE_GIF
	m_gif_obj = lv_gif_create(parent);
	lv_obj_set_size(m_gif_obj, lv_pct(100), lv_pct(100));
	lv_obj_set_style_pad_all(m_gif_obj, 0, 0);
	lv_obj_set_style_border_width(m_gif_obj, 0, 0);
	lv_obj_move_background(m_gif_obj);

	if (!m_source.empty()) {
		lv_gif_set_src(m_gif_obj, m_source.c_str());
		m_ready = lv_gif_is_loaded(m_gif_obj);
		if (!m_ready) {
			m_last_error = "Failed to load GIF source";
		}
	}
#else
	m_last_error = "GIF support is disabled (CONFIG_LV_USE_GIF)";
	m_ready = false;
#endif
}

void AnimatedGifProvider::setSource(const std::string& source) {
	m_source = source;
	m_last_error.clear();

#if LV_USE_GIF
	if (m_gif_obj != nullptr && !m_source.empty()) {
		lv_gif_set_src(m_gif_obj, m_source.c_str());
		m_ready = lv_gif_is_loaded(m_gif_obj);
		if (!m_ready) {
			m_last_error = "Failed to load GIF source";
		}
		return;
	}
	m_ready = !m_source.empty();
#else
	if (!m_source.empty()) {
		m_last_error = "GIF support is disabled (CONFIG_LV_USE_GIF)";
	}
	m_ready = false;
#endif
}

void AnimatedGifProvider::setAnimationSpeed(int32_t speed) {
	m_animation_speed = std::clamp(speed, static_cast<int32_t>(0), static_cast<int32_t>(100));
}

size_t AnimatedGifProvider::getMemoryUsage() const {
	return m_source.size();
}

} // namespace flx::ui::wallpaper