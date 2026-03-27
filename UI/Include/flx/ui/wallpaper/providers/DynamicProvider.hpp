#pragma once

#include <flx/ui/wallpaper/IWallpaperProvider.hpp>
#include <cstdint>
#include <string>

namespace flx::ui::wallpaper {

/**
 * @brief Generative / algorithmic wallpaper provider.
 *
 * Renders algorithmically generated content into an LVGL canvas each frame.
 * Supported algorithms:
 *   - "plasma"     — sinusoidal interference colour pattern
 *   - "perlin"     — smooth pseudo-random noise field
 *   - "gradient"   — animated colour gradient waves
 *
 * The source string uses an algorithm-URI format:
 *   @code
 *   "algo://plasma"
 *   "algo://perlin"
 *   "algo://gradient"
 *   @endcode
 *
 * A reduced-resolution internal canvas is scaled up to the display size to
 * keep CPU load manageable on ESP32 targets.
 */
class DynamicProvider : public IWallpaperProvider {
public:

	void initialize() override;
	void destroy() override;
	void render(lv_obj_t* parent, uint32_t elapsed_ms) override;
	void setSource(const std::string& source) override;
	void setAnimationSpeed(int32_t speed) override;
	bool isAnimated() const override { return true; }
	bool isReady() const override { return m_ready; }
	std::string getType() const override { return "dynamic"; }
	size_t getMemoryUsage() const override;
	std::string getLastError() const override { return m_last_error; }

private:

	// Canvas resolution — reduced to save CPU/memory.
	static constexpr int32_t CANVAS_W = 120;
	static constexpr int32_t CANVAS_H = 160;

	void parseSource(const std::string& source);
	void parseQueryParams(const std::string& query);
	void createCanvas(lv_obj_t* parent);
	void renderPlasma(uint32_t elapsed_ms);
	void renderPerlin(uint32_t elapsed_ms);
	void renderGradient(uint32_t elapsed_ms);
	void drawPixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b);
	void applyPalette(float& r, float& g, float& b) const;
	static float smoothNoise(float x, float y);

	lv_obj_t* m_parent {nullptr};
	lv_obj_t* m_canvas {nullptr};
	void* m_canvas_buf {nullptr};
	std::string m_algorithm {"plasma"};
	std::string m_source;
	std::string m_last_error;
	std::string m_palette {"vivid"};

	uint32_t m_total_elapsed_ms {0};
	int32_t m_animation_speed {50};
	int32_t m_param_speed {100};
	int32_t m_param_particles {96};
	int32_t m_param_noise_scale {100};
	bool m_ready {false};
};

} // namespace flx::ui::wallpaper
