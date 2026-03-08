#include <flx/apps/ContentResolver.hpp>
#include <flx/core/Logger.hpp>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <algorithm>

namespace flx::apps {

// Use a static mutex for ContentResolver map protection, as the singleton 
// constructor might be called early before FreeRTOS is up if we aren't careful, 
// or we just initialize lazily.
static SemaphoreHandle_t s_resolverMutex = nullptr;

static void ensureMutex() {
	if (!s_resolverMutex) {
		s_resolverMutex = xSemaphoreCreateMutex();
	}
}

std::string ContentResolver::extractAuthority(const std::string& uri) const {
	// Simple URI parsing for "content://authority/path"
	const std::string prefix = "content://";
	if (uri.find(prefix) != 0) {
		return std::string();
	}
	size_t start = prefix.length();
	size_t end = uri.find('/', start);
	if (end == std::string::npos) {
		return uri.substr(start);
	}
	return uri.substr(start, end - start);
}

void ContentResolver::registerProvider(const std::string& authority, std::shared_ptr<ContentProvider> provider) {
	if (!provider || authority.empty()) return;
	ensureMutex();
	xSemaphoreTake(s_resolverMutex, portMAX_DELAY);
	m_providers[authority] = std::move(provider);
	xSemaphoreGive(s_resolverMutex);
	Log::info("ContentResolver", "Registered provider for authority: %s", authority.c_str());
}

void ContentResolver::unregisterProvider(const std::string& authority) {
	ensureMutex();
	xSemaphoreTake(s_resolverMutex, portMAX_DELAY);
	m_providers.erase(authority);
	xSemaphoreGive(s_resolverMutex);
}

std::shared_ptr<ContentProvider> ContentResolver::getProvider(const std::string& authority) const {
	if (authority.empty()) return nullptr;
	ensureMutex();
	xSemaphoreTake(s_resolverMutex, portMAX_DELAY);
	auto it = m_providers.find(authority);
	auto provider = (it != m_providers.end()) ? it->second : nullptr;
	xSemaphoreGive(s_resolverMutex);
	return provider;
}

std::vector<flx::core::Bundle> ContentResolver::query(const std::string& uri, const flx::core::Bundle& filter) {
	std::string authority = extractAuthority(uri);
	auto provider = getProvider(authority);
	if (provider) {
		return provider->query(uri, filter);
	}
	Log::warn("ContentResolver", "No provider found for query URI: %s", uri.c_str());
	return {};
}

std::string ContentResolver::insert(const std::string& uri, const flx::core::Bundle& values) {
	std::string authority = extractAuthority(uri);
	auto provider = getProvider(authority);
	if (provider) {
		return provider->insert(uri, values);
	}
	Log::warn("ContentResolver", "No provider found for insert URI: %s", uri.c_str());
	return std::string();
}

int ContentResolver::update(const std::string& uri, const flx::core::Bundle& values, const flx::core::Bundle& filter) {
	std::string authority = extractAuthority(uri);
	auto provider = getProvider(authority);
	if (provider) {
		return provider->update(uri, values, filter);
	}
	Log::warn("ContentResolver", "No provider found for update URI: %s", uri.c_str());
	return 0;
}

int ContentResolver::remove(const std::string& uri, const flx::core::Bundle& filter) {
	std::string authority = extractAuthority(uri);
	auto provider = getProvider(authority);
	if (provider) {
		return provider->remove(uri, filter);
	}
	Log::warn("ContentResolver", "No provider found for remove URI: %s", uri.c_str());
	return 0;
}

} // namespace flx::apps
