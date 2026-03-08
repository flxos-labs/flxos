#pragma once

#include "ContentProvider.hpp"
#include <flx/core/Singleton.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace flx::apps {

/**
 * @brief Registry and router for ContentProviders
 *
 * Clients query this resolver using URIs (e.g. "content://contacts/all").
 * The resolver maps the "authority" part of the URI ("contacts") to a registered
 * ContentProvider and delegates the request to it.
 */
class ContentResolver : public flx::Singleton<ContentResolver> {
	friend class flx::Singleton<ContentResolver>;

public:

	/**
	 * @brief Register a provider for a specific authority
	 * @param authority The domain this provider serves (e.g. "contacts")
	 * @param provider The provider implementation
	 */
	void registerProvider(const std::string& authority, std::shared_ptr<ContentProvider> provider);

	/**
	 * @brief Unregister a previously registered provider
	 */
	void unregisterProvider(const std::string& authority);

	/**
	 * @brief Get the provider for an authority directly (if manual use is needed)
	 */
	std::shared_ptr<ContentProvider> getProvider(const std::string& authority) const;

	// === Delegated Operations ===

	/**
	 * @brief Delegate query to the appropriate provider based on URI
	 */
	std::vector<flx::core::Bundle> query(const std::string& uri, const flx::core::Bundle& filter = {});

	/**
	 * @brief Delegate insert to the appropriate provider based on URI
	 */
	std::string insert(const std::string& uri, const flx::core::Bundle& values);

	/**
	 * @brief Delegate update to the appropriate provider based on URI
	 */
	int update(const std::string& uri, const flx::core::Bundle& values, const flx::core::Bundle& filter);

	/**
	 * @brief Delegate remove to the appropriate provider based on URI
	 */
	int remove(const std::string& uri, const flx::core::Bundle& filter);

private:

	ContentResolver() = default;
	~ContentResolver() = default;

	std::string extractAuthority(const std::string& uri) const;

	std::unordered_map<std::string, std::shared_ptr<ContentProvider>> m_providers;
	void* m_mutex = nullptr;
};

} // namespace flx::apps
