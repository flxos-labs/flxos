#include <flx/ui/wallpaper/providers/DynamicProvider.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace flx::ui::wallpaper {

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DynamicProvider::initialize() {
	m_last_error.clear();
	m_ready = false;
	m_total_elapsed_ms = 0;
	m_animation_speed = 50;
}

void DynamicProvider::destroy() {
	if (m_canvas != nullptr) {
		lv_obj_delete(m_canvas);
		m_canvas = nullptr;
	}
	if (m_canvas_buf != nullptr) {
		std::free(m_canvas_buf);
		m_canvas_buf = nullptr;
	}
	m_parent = nullptr;
	m_source.clear();
	m_last_error.clear();
	m_total_elapsed_ms = 0;
	m_ready = false;
}

// ---------------------------------------------------------------------------
// IWallpaperProvider
// ---------------------------------------------------------------------------

void DynamicProvider::render(lv_obj_t* parent, uint32_t elapsed_ms) {
	if (parent == nullptr) {
		m_last_error = "Parent object is null";
		m_ready = false;
		return;
	}

	if (m_parent != parent) {
		destroy();
		m_parent = parent;
	}

	if (m_canvas == nullptr) {
		createCanvas(parent);
		if (m_canvas == nullptr) {
			return;
		}
	}

	// Scale elapsed time by animation speed (0-100%)
	uint32_t const speed_pct = static_cast<uint32_t>(std::max(m_animation_speed, static_cast<int32_t>(1)));
	uint32_t const scaled_ms = (elapsed_ms * speed_pct) / 50U; // 50% = normal speed
	m_total_elapsed_ms += scaled_ms;

	if (m_algorithm == "perlin") {
		renderPerlin(m_total_elapsed_ms);
	} else if (m_algorithm == "gradient") {
		renderGradient(m_total_elapsed_ms);
	} else {
		renderPlasma(m_total_elapsed_ms);
	}

	lv_obj_invalidate(m_canvas);
	m_ready = true;
}

void DynamicProvider::setSource(const std::string& source) {
	m_source = source;
	m_last_error.clear();
	parseSource(source);
	m_total_elapsed_ms = 0;
	m_ready = !m_source.empty();
}

void DynamicProvider::setAnimationSpeed(int32_t speed) {
	m_animation_speed = std::clamp(speed, static_cast<int32_t>(0), static_cast<int32_t>(100));
}

size_t DynamicProvider::getMemoryUsage() const {
	return static_cast<size_t>(CANVAS_W) * static_cast<size_t>(CANVAS_H) * 4U;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void DynamicProvider::parseSource(const std::string& source) {
	// Expected format: "algo://algorithm_name"
	const std::string prefix = "algo://";
	if (source.rfind(prefix, 0) == 0) {
		m_algorithm = source.substr(prefix.size());
	} else if (!source.empty()) {
		// Treat bare names like "plasma", "perlin", "gradient" directly
		m_algorithm = source;
	} else {
		m_algorithm = "plasma";
	}
}

void DynamicProvider::createCanvas(lv_obj_t* parent) {
	size_t const buf_bytes = static_cast<size_t>(CANVAS_W) *
	                         static_cast<size_t>(CANVAS_H) * 4U; // ARGB8888
	m_canvas_buf = std::malloc(buf_bytes);
	if (m_canvas_buf == nullptr) {
		m_last_error = "Failed to allocate canvas buffer";
		m_ready = false;
		return;
	}
	std::memset(m_canvas_buf, 0, buf_bytes);

	m_canvas = lv_canvas_create(parent);
	lv_canvas_set_buffer(m_canvas, m_canvas_buf, CANVAS_W, CANVAS_H, LV_COLOR_FORMAT_ARGB8888);
	lv_obj_set_size(m_canvas, lv_pct(100), lv_pct(100));
	lv_obj_set_style_pad_all(m_canvas, 0, 0);
	lv_obj_set_style_border_width(m_canvas, 0, 0);
	// Scale canvas up to fill parent
	lv_image_set_inner_align(m_canvas, LV_IMAGE_ALIGN_STRETCH);
	lv_obj_move_background(m_canvas);
}

void DynamicProvider::drawPixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b) {
	if (m_canvas_buf == nullptr || x < 0 || y < 0 || x >= CANVAS_W || y >= CANVAS_H) {
		return;
	}
	// ARGB8888 layout: B G R A (little-endian)
	size_t const offset = (static_cast<size_t>(y) * static_cast<size_t>(CANVAS_W) +
	                       static_cast<size_t>(x)) * 4U;
	auto* buf = static_cast<uint8_t*>(m_canvas_buf);
	buf[offset + 0] = b;
	buf[offset + 1] = g;
	buf[offset + 2] = r;
	buf[offset + 3] = 0xFF; // alpha
}

// ---------------------------------------------------------------------------
// Plasma algorithm
// ---------------------------------------------------------------------------

void DynamicProvider::renderPlasma(uint32_t elapsed_ms) {
	float const t = static_cast<float>(elapsed_ms) * 0.001f;
	for (int32_t y = 0; y < CANVAS_H; ++y) {
		for (int32_t x = 0; x < CANVAS_W; ++x) {
			float const fx = static_cast<float>(x) / static_cast<float>(CANVAS_W);
			float const fy = static_cast<float>(y) / static_cast<float>(CANVAS_H);

			float v = std::sin(fx * 6.0f + t);
			v += std::sin(fy * 6.0f - t);
			v += std::sin((fx + fy) * 4.0f + t * 0.5f);
			v += std::sin(std::sqrt(fx * fx + fy * fy) * 8.0f - t);
			v = (v + 4.0f) / 8.0f; // normalise to [0,1]

			// Hue rotation mapped to RGB
			float hue = v * 360.0f;
			float const s = 0.9f;
			float const l = 0.5f;

			// HSL to RGB
			float const c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
			float const x2 = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
			float const m2 = l - c * 0.5f;
			float r2 {}, g2 {}, b2 {};
			if (hue < 60.0f) { r2 = c; g2 = x2; b2 = 0; }
			else if (hue < 120.0f) { r2 = x2; g2 = c; b2 = 0; }
			else if (hue < 180.0f) { r2 = 0; g2 = c; b2 = x2; }
			else if (hue < 240.0f) { r2 = 0; g2 = x2; b2 = c; }
			else if (hue < 300.0f) { r2 = x2; g2 = 0; b2 = c; }
			else { r2 = c; g2 = 0; b2 = x2; }

			// Add lightness offset to produce final [0,1] RGB
			r2 += m2; g2 += m2; b2 += m2;
			auto clamp8 = [](float f) -> uint8_t {
				return static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, f)) * 255.0f);
			};
			drawPixel(x, y, clamp8(r2), clamp8(g2), clamp8(b2));
		}
	}
}

// ---------------------------------------------------------------------------
// Perlin-style smooth noise
// ---------------------------------------------------------------------------

float DynamicProvider::smoothNoise(float x, float y) {
	// Value noise via a deterministic integer hash + bilinear interpolation
	auto ix = static_cast<int32_t>(std::floor(x));
	auto iy = static_cast<int32_t>(std::floor(y));
	float const fx = x - std::floor(x);
	float const fy = y - std::floor(y);
	// Smooth step (Hermite curve)
	float const ux = fx * fx * (3.0f - 2.0f * fx);
	float const uy = fy * fy * (3.0f - 2.0f * fy);

	// Purely functional hash — no table required
	auto noise_at = [](int32_t xi, int32_t yi) -> float {
		uint32_t h = static_cast<uint32_t>(xi * 374761393 + yi * 668265263);
		h = (h ^ (h >> 13)) * 1274126177U;
		h ^= (h >> 16);
		return static_cast<float>(h & 0xFFFFU) / 65535.0f;
	};

	float const v00 = noise_at(ix, iy);
	float const v10 = noise_at(ix + 1, iy);
	float const v01 = noise_at(ix, iy + 1);
	float const v11 = noise_at(ix + 1, iy + 1);

	return v00 + ux * (v10 - v00) + uy * (v01 - v00) + ux * uy * (v00 - v10 - v01 + v11);
}

void DynamicProvider::renderPerlin(uint32_t elapsed_ms) {
	float const t = static_cast<float>(elapsed_ms) * 0.0004f;
	for (int32_t y = 0; y < CANVAS_H; ++y) {
		for (int32_t x = 0; x < CANVAS_W; ++x) {
			float const fx = static_cast<float>(x) * 0.06f;
			float const fy = static_cast<float>(y) * 0.06f;

			// Layered octaves
			float n = smoothNoise(fx + t, fy) * 0.5f;
			n += smoothNoise(fx * 2.0f + t * 1.5f, fy * 2.0f) * 0.25f;
			n += smoothNoise(fx * 4.0f + t * 2.0f, fy * 4.0f) * 0.125f;
			n = std::max(0.0f, std::min(1.0f, n * 1.14f)); // normalise

			// Map noise to a cool blue-teal-white palette
			auto lerp_chan = [](uint8_t a, uint8_t b, float t2) -> uint8_t {
				return static_cast<uint8_t>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t2);
			};
			uint8_t r, g, b;
			if (n < 0.5f) {
				float t2 = n * 2.0f;
				r = lerp_chan(10, 30, t2);
				g = lerp_chan(20, 100, t2);
				b = lerp_chan(80, 180, t2);
			} else {
				float t2 = (n - 0.5f) * 2.0f;
				r = lerp_chan(30, 220, t2);
				g = lerp_chan(100, 230, t2);
				b = lerp_chan(180, 255, t2);
			}
			drawPixel(x, y, r, g, b);
		}
	}
}

// ---------------------------------------------------------------------------
// Gradient Waves algorithm
// ---------------------------------------------------------------------------

void DynamicProvider::renderGradient(uint32_t elapsed_ms) {
	float const t = static_cast<float>(elapsed_ms) * 0.0008f;
	for (int32_t y = 0; y < CANVAS_H; ++y) {
		float const fy = static_cast<float>(y) / static_cast<float>(CANVAS_H);
		for (int32_t x = 0; x < CANVAS_W; ++x) {
			float const fx = static_cast<float>(x) / static_cast<float>(CANVAS_W);

			// Two gradient waves with phase offset
			float wave1 = (std::sin(fx * static_cast<float>(M_PI) * 2.0f + t) + 1.0f) * 0.5f;
			float wave2 = (std::sin(fy * static_cast<float>(M_PI) * 2.0f + t * 1.3f) + 1.0f) * 0.5f;
			float blend = (wave1 + wave2) * 0.5f;

			// Colour A → B → C cycle
			float hue = blend * 240.0f + t * 20.0f;
			hue = std::fmod(hue, 360.0f);

			// Simple HSV to RGB
			float const h6 = hue / 60.0f;
			auto i = static_cast<int32_t>(h6);
			float const f = h6 - static_cast<float>(i);
			float const v = 0.9f;
			float const s = 0.8f;
			float const p = v * (1.0f - s);
			float const q = v * (1.0f - s * f);
			float const t2 = v * (1.0f - s * (1.0f - f));
			float r {}, g {}, b {};
			switch (i % 6) {
				case 0: r = v; g = t2; b = p; break;
				case 1: r = q; g = v; b = p; break;
				case 2: r = p; g = v; b = t2; break;
				case 3: r = p; g = q; b = v; break;
				case 4: r = t2; g = p; b = v; break;
				default: r = v; g = p; b = q; break;
			}
			auto to8 = [](float f2) -> uint8_t {
				return static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, f2)) * 255.0f);
			};
			drawPixel(x, y, to8(r), to8(g), to8(b));
		}
	}
}

} // namespace flx::ui::wallpaper
