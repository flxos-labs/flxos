#pragma once

#include <flx/core/Singleton.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace flx::system {
class WallpaperManager;
} // namespace flx::system

namespace flx::ui::wallpaper {

/**
 * @brief Metadata record for a single wallpaper preset.
 */
struct WallpaperPreset {
	std::string id; ///< Unique preset identifier, e.g. "plasma"
	std::string name; ///< Human-readable display name
	std::string description; ///< Short description shown in the preset browser
	std::string type; ///< Provider type: "static" | "animated" | "lottie" | "dynamic"
	std::string source; ///< File path or algorithm URI passed to the provider
	std::string effects; ///< JSON string of effect configuration (may be empty)
	std::string thumbnail; ///< Path to preview image (may be empty)
	bool is_builtin {false}; ///< True for shipped presets; false for user-created ones
};

/**
 * @brief Registry and loader for wallpaper presets.
 *
 * Built-in presets are registered programmatically at startup.
 * User presets are persisted to the filesystem in a simple JSON format.
 *
 * All access is read-only from the UI thread once presets are loaded.
 */
class PresetLibrary : public flx::Singleton<PresetLibrary> {
	friend class flx::Singleton<PresetLibrary>;

public:

	/** Load built-in presets and scan user-preset directory. */
	void loadPresets();

	/** Return the preset with the given id, or nullptr if not found. */
	const WallpaperPreset* getPreset(const std::string& id) const;

	/** Return all loaded presets in insertion order. */
	std::vector<const WallpaperPreset*> listPresets() const;

	/** Return only presets of a specific type (e.g. "dynamic"). */
	std::vector<const WallpaperPreset*> listPresetsByType(const std::string& type) const;

	/**
	 * Apply the preset with the given id through @p manager.
	 * @return true on success; false if the preset does not exist.
	 */
	bool applyPreset(const std::string& id, flx::system::WallpaperManager* manager) const;

	/** Persist a user-created preset and add it to the in-memory registry. */
	bool saveUserPreset(const WallpaperPreset& preset);

	/** Remove a user preset by id. Built-in presets cannot be removed. */
	bool deleteUserPreset(const std::string& id);

	bool isEmpty() const { return m_presets.empty(); }
	size_t size() const { return m_presets.size(); }

private:

	PresetLibrary() = default;
	~PresetLibrary() = default;

	void registerBuiltinPresets();

	std::map<std::string, WallpaperPreset> m_presets;
	std::vector<std::string> m_order; ///< Insertion order for deterministic listing
};

} // namespace flx::ui::wallpaper
