#pragma once

#include "AppManifest.hpp"
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace flx::apps {

/**
 * @brief Central registry for all app manifests
 *
 * flx::Singleton that manages app registration and lookup. Apps register their
 * manifests here, and the launcher/AppManager queries it to discover apps.
 *
 * Supports advanced filtering by category, capability, and MIME type.
 * Also provides lookup by visibility.
 */
class AppRegistry {
public:

	static AppRegistry& getInstance();

	// === Registration ===
	void addApp(const AppManifest& manifest);
	bool removeApp(const std::string& appId);

	// === Basic Queries ===
	std::vector<AppManifest> getAll() const;
	std::optional<AppManifest> findById(const std::string& appId) const;

	// === Advanced Filtering ===
	std::vector<AppManifest> getByCategory(AppCategory category) const;
	std::vector<AppManifest> getByCapability(AppCapability capability) const;
	std::vector<AppManifest> getForMimeType(const std::string& mimeType) const;
	std::vector<AppManifest> getVisible() const; // Excludes Hidden-flagged apps

	// === Info ===
	size_t count() const;
	bool hasApp(const std::string& appId) const;

private:

	AppRegistry() = default;
	~AppRegistry() = default;
	AppRegistry(const AppRegistry&) = delete;
	AppRegistry& operator=(const AppRegistry&) = delete;

	std::vector<AppManifest> m_manifests;
	std::unordered_map<std::string, size_t> m_idIndex; // appId → index in m_manifests
	mutable std::mutex m_mutex;

	// Insert sorted by sortPriority
	void insertSorted(const AppManifest& manifest);

	// Simple wildcard MIME matching (e.g. "text/*" matches "text/plain")
	static bool matchesMimeType(const std::string& pattern, const std::string& type);
};

} // namespace flx::apps
