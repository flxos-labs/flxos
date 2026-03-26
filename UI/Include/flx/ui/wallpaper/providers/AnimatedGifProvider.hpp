#pragma once

#include <flx/ui/wallpaper/IWallpaperProvider.hpp>
#include <string>

namespace flx::ui::wallpaper {

class AnimatedGifProvider : public IWallpaperProvider {
public:

	void initialize() override;
	void destroy() override;
	void render(lv_obj_t* parent, uint32_t elapsed_ms) override;
	void setSource(const std::string& source) override;
	void setAnimationSpeed(int32_t speed) override;
	bool isAnimated() const override { return true; }
	bool isReady() const override { return m_ready; }
	std::string getType() const override { return "animated"; }
	size_t getMemoryUsage() const override;
	std::string getLastError() const override { return m_last_error; }

private:

	lv_obj_t* m_parent = nullptr;
	lv_obj_t* m_gif_obj = nullptr;
	std::string m_source;
	std::string m_last_error;
	int32_t m_animation_speed = 50;
	bool m_ready = false;
};

} // namespace flx::ui::wallpaper