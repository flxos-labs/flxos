#pragma once

#include <map>
#include <string>
#include <vector>

namespace flx::system {
class WallpaperManager;
}

namespace flx::ui::wallpaper {

struct WallpaperPreset {
	std::string id;
	std::string name;
	std::string description;
	std::string type;
	std::string source;
	std::string effects;
	std::string thumbnail;
	bool is_builtin = false;
};

class PresetLibrary {
public:
	void loadPresets();
	const WallpaperPreset* getPreset(const std::string& id) const;
	std::vector<const WallpaperPreset*> listPresets() const;
	std::vector<const WallpaperPreset*> listPresetsByType(const std::string& type) const;
	bool applyPreset(const std::string& id, flx::system::WallpaperManager* manager) const;

	bool saveUserPreset(const WallpaperPreset& preset);
	bool deleteUserPreset(const std::string& id);

private:
	void registerBuiltinPresets();

	std::map<std::string, WallpaperPreset> m_presets;
	std::vector<std::string> m_order;
};

} // namespace flx::ui::wallpaper
