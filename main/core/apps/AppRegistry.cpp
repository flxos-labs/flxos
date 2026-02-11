#include "AppRegistry.hpp"
#include "esp_log.h"
#include <algorithm>

static const char* TAG = "AppRegistry";

namespace System::Apps {

AppRegistry& AppRegistry::getInstance() {
	static AppRegistry instance;
	return instance;
}

void AppRegistry::addApp(const AppManifest& manifest) {
	std::lock_guard<std::mutex> lock(m_mutex);

	// Check for duplicates
	for (const auto& existing: m_manifests) {
		if (existing.appId == manifest.appId) {
			ESP_LOGW(TAG, "App already registered: %s", manifest.appId.c_str());
			return;
		}
	}

	insertSorted(manifest);
	ESP_LOGI(TAG, "Registered app: %s (%s) [priority=%d]", manifest.appName.c_str(), manifest.appId.c_str(), manifest.sortPriority);
}

bool AppRegistry::removeApp(const std::string& appId) {
	std::lock_guard<std::mutex> lock(m_mutex);

	auto it = std::remove_if(m_manifests.begin(), m_manifests.end(), [&appId](const AppManifest& m) { return m.appId == appId; });

	if (it != m_manifests.end()) {
		m_manifests.erase(it, m_manifests.end());
		ESP_LOGI(TAG, "Removed app: %s", appId.c_str());
		return true;
	}

	ESP_LOGW(TAG, "App not found for removal: %s", appId.c_str());
	return false;
}

const std::vector<AppManifest>& AppRegistry::getAll() const {
	return m_manifests;
}

std::optional<AppManifest> AppRegistry::findById(const std::string& appId) const {
	std::lock_guard<std::mutex> lock(m_mutex);

	for (const auto& manifest: m_manifests) {
		if (manifest.appId == appId) {
			return manifest;
		}
	}
	return std::nullopt;
}

std::vector<AppManifest> AppRegistry::getByCategory(AppCategory category) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<AppManifest> result;

	for (const auto& manifest: m_manifests) {
		if (manifest.category == category) {
			result.push_back(manifest);
		}
	}
	return result;
}

std::vector<AppManifest> AppRegistry::getByCapability(AppCapability capability) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<AppManifest> result;

	for (const auto& manifest: m_manifests) {
		if (hasCapability(manifest.capabilities, capability)) {
			result.push_back(manifest);
		}
	}
	return result;
}

std::vector<AppManifest> AppRegistry::getForMimeType(const std::string& mimeType) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<AppManifest> result;

	for (const auto& manifest: m_manifests) {
		for (const auto& pattern: manifest.supportedMimeTypes) {
			if (matchesMimeType(pattern, mimeType)) {
				result.push_back(manifest);
				break;
			}
		}
	}
	return result;
}

std::vector<AppManifest> AppRegistry::getVisible() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<AppManifest> result;

	for (const auto& manifest: m_manifests) {
		if (!(manifest.flags & AppFlags::Hidden)) {
			result.push_back(manifest);
		}
	}
	return result;
}

size_t AppRegistry::count() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_manifests.size();
}

bool AppRegistry::hasApp(const std::string& appId) const {
	std::lock_guard<std::mutex> lock(m_mutex);

	for (const auto& manifest: m_manifests) {
		if (manifest.appId == appId) {
			return true;
		}
	}
	return false;
}

void AppRegistry::insertSorted(const AppManifest& manifest) {
	auto pos = std::lower_bound(m_manifests.begin(), m_manifests.end(), manifest, [](const AppManifest& a, const AppManifest& b) {
		return a.sortPriority < b.sortPriority;
	});
	m_manifests.insert(pos, manifest);
}

bool AppRegistry::matchesMimeType(const std::string& pattern, const std::string& type) {
	// Exact match
	if (pattern == type) return true;

	// Wildcard match: "text/*" matches "text/plain"
	auto slashPos = pattern.find('/');
	if (slashPos != std::string::npos && pattern.substr(slashPos + 1) == "*") {
		auto patternPrefix = pattern.substr(0, slashPos);
		auto typeSlash = type.find('/');
		if (typeSlash != std::string::npos) {
			return type.substr(0, typeSlash) == patternPrefix;
		}
	}

	// Universal wildcard
	if (pattern == "*/*") return true;

	return false;
}

} // namespace System::Apps
