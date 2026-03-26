#pragma once

#include "lvgl.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace flx::ui::wallpaper {

class IWallpaperProvider {
public:

	virtual ~IWallpaperProvider() = default;

	virtual void initialize() = 0;
	virtual void destroy() = 0;
	virtual void render(lv_obj_t* parent, uint32_t elapsed_ms) = 0;
	virtual void setSource(const std::string& source) = 0;
	virtual void setAnimationSpeed(int32_t speed) = 0;
	virtual bool isAnimated() const = 0;
	virtual bool isReady() const = 0;
	virtual std::string getType() const = 0;
	virtual size_t getMemoryUsage() const = 0;
	virtual std::string getLastError() const = 0;
};

} // namespace flx::ui::wallpaper
