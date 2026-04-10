#include <flx/ui/wallpaper/PresetLibrary.hpp>

#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <flx/core/Logger.hpp>
#include <flx/core/Value.hpp>
#include <flx/system/managers/WallpaperManager.hpp>
#include <string>
#include <sys/stat.h>
#include <vector>

static constexpr const char* TAG = "PresetLibrary";

namespace flx::ui::wallpaper {

namespace {

static constexpr const char* BUILTIN_PRESET_ROOT = "/data/wallpapers/presets/builtin";

bool fileExists(const std::string& path) {
	struct stat st {};
	return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string readFileText(const std::string& path) {
	FILE* f = std::fopen(path.c_str(), "rb");
	if (f == nullptr) {
		return {};
	}
	if (std::fseek(f, 0, SEEK_END) != 0) {
		std::fclose(f);
		return {};
	}
	long const len = std::ftell(f);
	if (len <= 0) {
		std::fclose(f);
		return {};
	}
	if (std::fseek(f, 0, SEEK_SET) != 0) {
		std::fclose(f);
		return {};
	}

	std::string text(static_cast<size_t>(len), '\0');
	if (std::fread(text.data(), 1, static_cast<size_t>(len), f) != static_cast<size_t>(len)) {
		std::fclose(f);
		return {};
	}
	std::fclose(f);
	return text;
}
bool loadPresetConfig(const std::string& configPath, WallpaperPreset& outPreset) {
	if (!fileExists(configPath)) {
		return false;
	}

	std::string const raw = readFileText(configPath);
	if (raw.empty()) {
		return false;
	}

	auto document = flx::core::FlxValueDocument::parseAuto(std::move(raw));
	if (!document) {
		return false;
	}

	const flx::core::FlxValueView root = document->root();

	outPreset.id = root.child("id").asString();
	outPreset.name = root.child("name").asString();
	outPreset.description = root.child("description").asString();
	outPreset.type = root.child("type").asString("static");
	outPreset.source = root.child("source").asString();
	outPreset.is_builtin = true;

	const flx::core::FlxValueView effects = root.child("effects");
	if (effects.valid()) {
		outPreset.effects = effects.toJsonString();
	}

	const flx::core::FlxValueView metadata = root.child("metadata");
	if (metadata.valid()) {
		outPreset.thumbnail = metadata.child("thumbnail").asString();
	}

	if (outPreset.id.empty() || outPreset.type.empty()) {
		return false;
	}

	if (outPreset.name.empty()) {
		outPreset.name = outPreset.id;
	}

	return true;
}

} // namespace

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
	for (const auto& id: m_order) {
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
	for (const auto& id: m_order) {
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
	manager->setWallpaper(preset->source);
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
	bool loadedFromConfig = false;
	DIR* root = opendir(BUILTIN_PRESET_ROOT);
	if (root != nullptr) {
		std::vector<std::string> dirs;
		while (dirent* ent = readdir(root)) {
			std::string const name = ent->d_name;
			if (name == "." || name == "..") {
				continue;
			}
			dirs.push_back(name);
		}
		closedir(root);

		std::sort(dirs.begin(), dirs.end());
		for (const auto& dir: dirs) {
			WallpaperPreset preset;
			std::string const configPath = std::string(BUILTIN_PRESET_ROOT) + "/" + dir + "/config.json";
			if (!loadPresetConfig(configPath, preset)) {
				continue;
			}

			m_presets[preset.id] = preset;
			m_order.push_back(preset.id);
			loadedFromConfig = true;
		}
	}

	if (loadedFromConfig) {
		return;
	}

	// Only provide a static placeholder when no configs are present.
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
