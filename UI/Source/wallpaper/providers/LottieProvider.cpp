#include <flx/ui/wallpaper/providers/LottieProvider.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace {

bool hasSuffix(const std::string& value, const char* suffix) {
	size_t const suffixLen = std::char_traits<char>::length(suffix);
	return value.size() >= suffixLen && value.compare(value.size() - suffixLen, suffixLen, suffix) == 0;
}

} // namespace

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
	m_lottie_path.clear();
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
		if (!m_lottie_path.empty()) {
			lv_lottie_set_src_file(m_lottie_obj, m_lottie_path.c_str());
			m_ready = true;
			m_base_duration_ms = 0;
			applyAnimationSpeed();
		} else {
			m_ready = false;
			m_last_error = "parse_error:empty_source";
		}
	}
#else
	m_last_error = "Lottie support is disabled (CONFIG_LV_USE_LOTTIE)";
	m_ready = false;
#endif
}

void LottieProvider::setSource(const std::string& source) {
	m_source = source;
	m_lottie_path.clear();
	m_last_error.clear();
	m_base_duration_ms = 0;

#if LV_USE_LOTTIE
	if (!m_source.empty() && !prepareSourceForLoad(m_source)) {
		m_ready = false;
		return;
	}

	if (m_lottie_obj != nullptr && !m_source.empty()) {
		if (m_lottie_path.empty()) {
			m_ready = false;
			m_last_error = "parse_error:empty_source";
			return;
		}

		lv_lottie_set_src_file(m_lottie_obj, m_lottie_path.c_str());
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

std::string LottieProvider::sanitizeLottiePath(const std::string& source) const {
	if (source.empty()) {
		return {};
	}

	size_t queryPos = source.find('?');
	if (queryPos == std::string::npos) {
		return source;
	}

	return source.substr(0, queryPos);
}

bool LottieProvider::parseComplexityHints(const std::string& source, int32_t& layers, int32_t& shapes, int32_t& ops) const {
	layers = -1;
	shapes = -1;
	ops = -1;

	size_t queryPos = source.find('?');
	if (queryPos == std::string::npos || queryPos + 1 >= source.size()) {
		return false;
	}

	bool parsedAny = false;
	std::string query = source.substr(queryPos + 1);
	size_t start = 0;
	while (start < query.size()) {
		size_t end = query.find('&', start);
		if (end == std::string::npos) {
			end = query.size();
		}

		std::string pair = query.substr(start, end - start);
		size_t eq = pair.find('=');
		if (eq != std::string::npos && eq + 1 < pair.size()) {
			std::string key = pair.substr(0, eq);
			std::string value = pair.substr(eq + 1);
			char* parseEnd = nullptr;
			long parsedValue = std::strtol(value.c_str(), &parseEnd, 10);
			if (parseEnd != nullptr && *parseEnd == '\0') {
				if (key == "layers") {
					layers = static_cast<int32_t>(parsedValue);
					parsedAny = true;
				} else if (key == "shapes") {
					shapes = static_cast<int32_t>(parsedValue);
					parsedAny = true;
				} else if (key == "ops") {
					ops = static_cast<int32_t>(parsedValue);
					parsedAny = true;
				}
			}
		}

		start = end + 1;
	}

	return parsedAny;
}

int32_t LottieProvider::complexityThresholdForCurrentTarget() const {
#if defined(CONFIG_IDF_TARGET_ESP32)
	return 900;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
	return 1500;
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
	return 2600;
#else
	return 1400;
#endif
}

int32_t LottieProvider::computeComplexityScore(const std::string& path, int32_t layers, int32_t shapes, int32_t ops) const {
	int32_t score = 0;

	if (layers > 0) {
		score += layers * 3;
	}
	if (shapes > 0) {
		score += shapes * 2;
	}
	if (ops > 0) {
		score += ops / 50;
	}

	FILE* f = std::fopen(path.c_str(), "rb");
	if (f != nullptr) {
		if (std::fseek(f, 0, SEEK_END) == 0) {
			long fileBytes = std::ftell(f);
			if (fileBytes > 0) {
				score += static_cast<int32_t>(fileBytes / 1024L);
			}
		}
		std::fclose(f);
	}

	return score;
}

bool LottieProvider::prepareSourceForLoad(const std::string& source) {
	std::string path = sanitizeLottiePath(source);
	if (path.empty()) {
		m_last_error = "parse_error:empty_source";
		return false;
	}

	if (!(hasSuffix(path, ".json") || hasSuffix(path, ".lottie"))) {
		m_last_error = "parse_error:unsupported_extension";
		return false;
	}

	FILE* check = std::fopen(path.c_str(), "rb");
	if (check == nullptr) {
		m_last_error = "parse_error:file_not_found";
		return false;
	}
	std::fclose(check);

	int32_t layers = -1;
	int32_t shapes = -1;
	int32_t ops = -1;
	parseComplexityHints(source, layers, shapes, ops);

	int32_t const complexity = computeComplexityScore(path, layers, shapes, ops);
	int32_t const threshold = complexityThresholdForCurrentTarget();
	if (complexity > threshold) {
		m_last_error = "complexity_exceeded:score=" + std::to_string(complexity) + ",threshold=" + std::to_string(threshold);
		return false;
	}

	m_lottie_path = path;
	return true;
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