#include <flx/ui/wallpaper/providers/LottieProvider.hpp>

#include <algorithm>
#include <cstdlib>

namespace flx::ui::wallpaper {

void LottieProvider::initialize() {
	m_last_error.clear();
	m_ready = false;
	m_animation_speed = 50;
	m_base_duration_ms = 0;
}

void LottieProvider::destroy() {
	if (m_lottie_obj != nullptr) {
		lv_obj_delete(m_lottie_obj);
		m_lottie_obj = nullptr;
	}
	if (m_lottie_buffer != nullptr) {
		std::free(m_lottie_buffer);
		m_lottie_buffer = nullptr;
	}
	m_parent = nullptr;
	m_buffer_width = 0;
	m_buffer_height = 0;
	m_source.clear();
	m_last_error.clear();
	m_base_duration_ms = 0;
	m_ready = false;
}

void LottieProvider::render(lv_obj_t* parent, uint32_t /*elapsed_ms*/) {
	if (parent == nullptr) {
		m_last_error = "Parent object is null";
		m_ready = false;
		return;
	}

	if (m_parent != parent) {
		if (m_lottie_obj != nullptr) {
			lv_obj_delete(m_lottie_obj);
			m_lottie_obj = nullptr;
		}
		if (m_lottie_buffer != nullptr) {
			std::free(m_lottie_buffer);
			m_lottie_buffer = nullptr;
		}
		m_parent = parent;
		m_buffer_width = 0;
		m_buffer_height = 0;
		m_base_duration_ms = 0;
	}

	if (m_lottie_obj != nullptr) {
		return;
	}

#if LV_USE_LOTTIE
	int32_t width = lv_obj_get_width(parent);
	int32_t height = lv_obj_get_height(parent);
	if (width <= 0 || height <= 0) {
		lv_display_t* display = lv_obj_get_display(parent);
		if (display != nullptr) {
			if (width <= 0) {
				width = static_cast<int32_t>(lv_display_get_horizontal_resolution(display));
			}
			if (height <= 0) {
				height = static_cast<int32_t>(lv_display_get_vertical_resolution(display));
			}
		}
	}
	if (width <= 0) {
		width = 240;
	}
	if (height <= 0) {
		height = 320;
	}

	size_t const bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4U;
	m_lottie_buffer = std::malloc(bytes);
	if (m_lottie_buffer == nullptr) {
		m_last_error = "Failed to allocate Lottie buffer";
		m_ready = false;
		return;
	}
	m_buffer_width = width;
	m_buffer_height = height;

	m_lottie_obj = lv_lottie_create(parent);
	lv_obj_set_size(m_lottie_obj, lv_pct(100), lv_pct(100));
	lv_obj_set_style_pad_all(m_lottie_obj, 0, 0);
	lv_obj_set_style_border_width(m_lottie_obj, 0, 0);
	lv_obj_move_background(m_lottie_obj);
	lv_lottie_set_buffer(m_lottie_obj, m_buffer_width, m_buffer_height, m_lottie_buffer);

	if (!m_source.empty()) {
		lv_lottie_set_src_file(m_lottie_obj, m_source.c_str());
		m_ready = true;
		m_base_duration_ms = 0;
		applyAnimationSpeed();
	}
#else
	m_last_error = "Lottie support is disabled (CONFIG_LV_USE_LOTTIE)";
	m_ready = false;
#endif
}

void LottieProvider::setSource(const std::string& source) {
	m_source = source;
	m_last_error.clear();
	m_base_duration_ms = 0;

#if LV_USE_LOTTIE
	if (m_lottie_obj != nullptr && !m_source.empty()) {
		lv_lottie_set_src_file(m_lottie_obj, m_source.c_str());
		m_ready = true;
		applyAnimationSpeed();
		return;
	}
	m_ready = !m_source.empty();
#else
	if (!m_source.empty()) {
		m_last_error = "Lottie support is disabled (CONFIG_LV_USE_LOTTIE)";
	}
	m_ready = false;
#endif
}

void LottieProvider::setAnimationSpeed(int32_t speed) {
	m_animation_speed = std::clamp(speed, static_cast<int32_t>(0), static_cast<int32_t>(100));
	applyAnimationSpeed();
}

size_t LottieProvider::getMemoryUsage() const {
	return m_source.size() + (static_cast<size_t>(m_buffer_width) * static_cast<size_t>(m_buffer_height) * 4U);
}

void LottieProvider::applyAnimationSpeed() {
#if LV_USE_LOTTIE
	if (m_lottie_obj == nullptr) {
		return;
	}

	lv_anim_t* anim = lv_lottie_get_anim(m_lottie_obj);
	if (anim == nullptr) {
		return;
	}

	if (m_base_duration_ms == 0) {
		m_base_duration_ms = (anim->duration > 0U) ? anim->duration : 1000U;
	}

	uint32_t const speed = static_cast<uint32_t>(std::max(m_animation_speed, static_cast<int32_t>(1)));
	uint32_t const duration = std::max(static_cast<uint32_t>(1), (m_base_duration_ms * 50U) / speed);
	lv_anim_set_duration(anim, duration);
#endif
}

} // namespace flx::ui::wallpaper