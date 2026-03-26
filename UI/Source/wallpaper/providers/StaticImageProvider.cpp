#include <flx/ui/wallpaper/providers/StaticImageProvider.hpp>

namespace flx::ui::wallpaper {

void StaticImageProvider::initialize() {
	m_last_error.clear();
	m_ready = false;
}

void StaticImageProvider::destroy() {
	if (m_image_obj != nullptr) {
		lv_obj_delete(m_image_obj);
		m_image_obj = nullptr;
	}
	if (!m_image_path.empty() && lv_image_cache_is_enabled()) {
		lv_image_cache_drop(m_image_path.c_str());
	}
	m_image_path.clear();
	m_ready = false;
	m_last_error.clear();
}

void StaticImageProvider::render(lv_obj_t* parent, uint32_t /*elapsed_ms*/) {
	if (parent == nullptr) {
		m_last_error = "Parent object is null";
		m_ready = false;
		return;
	}

	if (m_parent != parent) {
		if (m_image_obj != nullptr) {
			lv_obj_delete(m_image_obj);
			m_image_obj = nullptr;
		}
		m_parent = parent;
	}

	if (m_image_obj == nullptr) {
		m_image_obj = lv_image_create(parent);
		lv_obj_set_size(m_image_obj, lv_pct(100), lv_pct(100));
		lv_obj_set_style_pad_all(m_image_obj, 0, 0);
		lv_obj_set_style_border_width(m_image_obj, 0, 0);
		lv_image_set_inner_align(m_image_obj, LV_IMAGE_ALIGN_COVER);
		lv_obj_move_background(m_image_obj);
		if (!m_image_path.empty()) {
			lv_image_set_src(m_image_obj, m_image_path.c_str());
			m_ready = true;
		}
	}
}

void StaticImageProvider::setSource(const std::string& source) {
	if (!m_image_path.empty() && lv_image_cache_is_enabled()) {
		lv_image_cache_drop(m_image_path.c_str());
	}

	m_image_path = source;
	m_last_error.clear();
	if (m_image_obj != nullptr) {
		if (m_image_path.empty()) {
			m_ready = false;
			return;
		}
		lv_image_set_src(m_image_obj, m_image_path.c_str());
		m_ready = true;
	}
}

void StaticImageProvider::setAnimationSpeed(int32_t /*speed*/) {
	// Static wallpapers do not support animation speed.
}

size_t StaticImageProvider::getMemoryUsage() const {
	return m_image_path.size();
}

} // namespace flx::ui::wallpaper
