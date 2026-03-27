#include <flx/ui/wallpaper/PresetLibrary.hpp>

#include <algorithm>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/WallpaperManager.hpp>

static constexpr const char* TAG = "PresetLibrary";

namespace flx::ui::wallpaper {

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PresetLibrary::loadPresets() {
	m_presets.clear();
	m_order.clear();
	registerBuiltinPresets();
	Log::info(TAG, "Loaded %zu presets", m_presets.size());
}

const WallpaperPreset* PresetLibrary::getPreset(const std::string& id) const {
	auto it = m_presets.find(id);
	return (it != m_presets.end()) ? &it->second : nullptr;
}

std::vector<const WallpaperPreset*> PresetLibrary::listPresets() const {
	std::vector<const WallpaperPreset*> result;
	result.reserve(m_order.size());
	for (const auto& id : m_order) {
		auto it = m_presets.find(id);
		if (it != m_presets.end()) {
			result.push_back(&it->second);
		}
	}
	return result;
}

std::vector<const WallpaperPreset*> PresetLibrary::listPresetsByType(
	const std::string& type) const {
	std::vector<const WallpaperPreset*> result;
	for (const auto& id : m_order) {
		auto it = m_presets.find(id);
		if (it != m_presets.end() && it->second.type == type) {
			result.push_back(&it->second);
		}
	}
	return result;
}

bool PresetLibrary::applyPreset(const std::string& id,
	flx::system::WallpaperManager* manager) const {
	if (manager == nullptr) {
		return false;
	}
	const WallpaperPreset* preset = getPreset(id);
	if (preset == nullptr) {
		Log::warn(TAG, "Preset not found: %s", id.c_str());
		return false;
	}
	manager->setWallpaper(preset->source, preset->type);
	Log::info(TAG, "Applied preset '%s' (type=%s)", id.c_str(), preset->type.c_str());
	return true;
}

bool PresetLibrary::saveUserPreset(const WallpaperPreset& preset) {
	if (preset.id.empty()) {
		return false;
	}
	WallpaperPreset p = preset;
	p.is_builtin = false;
	bool const is_new = (m_presets.find(p.id) == m_presets.end());
	m_presets[p.id] = p;
	if (is_new) {
		m_order.push_back(p.id);
	}
	Log::info(TAG, "Saved user preset '%s'", p.id.c_str());
	return true;
}

bool PresetLibrary::deleteUserPreset(const std::string& id) {
	auto it = m_presets.find(id);
	if (it == m_presets.end()) {
		return false;
	}
	if (it->second.is_builtin) {
		Log::warn(TAG, "Cannot delete built-in preset '%s'", id.c_str());
		return false;
	}
	m_presets.erase(it);
	auto ord_it = std::find(m_order.begin(), m_order.end(), id);
	if (ord_it != m_order.end()) {
		m_order.erase(ord_it);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Private: built-in preset registration
// ---------------------------------------------------------------------------

void PresetLibrary::registerBuiltinPresets() {
	// --- Dynamic: Plasma ---
	{
		WallpaperPreset p;
		p.id = "plasma";
		p.name = "Plasma";
		p.description = "Vibrant sinusoidal colour interference pattern";
		p.type = "dynamic";
		p.source = "algo://plasma";
		p.is_builtin = true;
		m_presets[p.id] = p;
		m_order.push_back(p.id);
	}
	// --- Dynamic: Gradient Waves ---
	{
		WallpaperPreset p;
		p.id = "gradient";
		p.name = "Gradient Waves";
		p.description = "Smooth animated colour gradient waves";
		p.type = "dynamic";
		p.source = "algo://gradient";
		p.is_builtin = true;
		m_presets[p.id] = p;
		m_order.push_back(p.id);
	}
	// --- Dynamic: Perlin Noise ---
	{
		WallpaperPreset p;
		p.id = "perlin";
		p.name = "Cloud Noise";
		p.description = "Smooth organic noise cloud pattern";
		p.type = "dynamic";
		p.source = "algo://perlin";
		p.is_builtin = true;
		m_presets[p.id] = p;
		m_order.push_back(p.id);
	}
	// --- Static placeholder (user-selected file) ---
	{
		WallpaperPreset p;
		p.id = "custom_static";
		p.name = "Custom Image";
		p.description = "Use a custom image from the filesystem";
		p.type = "static";
		p.source = "";
		p.is_builtin = true;
		m_presets[p.id] = p;
		m_order.push_back(p.id);
	}
}

} // namespace flx::ui::wallpaper
