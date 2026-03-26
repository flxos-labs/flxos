#pragma once

#include <flx/ui/wallpaper/IWallpaperProvider.hpp>
#include <cstddef>
#include <cstdint>
#include <string>

namespace flx::ui::wallpaper {

class LottieProvider : public IWallpaperProvider {
public:

	void initialize() override;
	void destroy() override;
	void render(lv_obj_t* parent, uint32_t elapsed_ms) override;
	void setSource(const std::string& source) override;
	void setAnimationSpeed(int32_t speed) override;
	bool isAnimated() const override { return true; }
	bool isReady() const override { return m_ready; }
	std::string getType() const override { return "lottie"; }
	size_t getMemoryUsage() const override;
	std::string getLastError() const override { return m_last_error; }

private:

	void applyAnimationSpeed();

	lv_obj_t* m_parent = nullptr;
	lv_obj_t* m_lottie_obj = nullptr;
	void* m_lottie_buffer = nullptr;
	int32_t m_buffer_width = 0;
	int32_t m_buffer_height = 0;
	std::string m_source;
	std::string m_last_error;
	int32_t m_animation_speed = 50;
	uint32_t m_base_duration_ms = 0;
	bool m_ready = false;
};

} // namespace flx::ui::wallpaper