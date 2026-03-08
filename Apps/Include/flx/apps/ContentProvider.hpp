#pragma once

#include <flx/core/Bundle.hpp>
#include <string>
#include <vector>

namespace flx::apps {

/**
 * @brief Base class for sharing structured data between applications
 *
 * Modeled after Android's ContentProvider pattern, this allows an app to expose
 * a dataset (like contacts, media, or settings) to other apps without tightly coupling
 * the consumer to the providing app's internal database structure.
 */
class ContentProvider {
public:

	virtual ~ContentProvider() = default;

	/**
	 * @brief Query rows matching a filter
	 * @param uri The URI to query (e.g. "content://contacts/all")
	 * @param filter Optional key-value constraints
	 * @return A list of Bundles, each representing a matching row/record
	 */
	virtual std::vector<flx::core::Bundle> query(const std::string& uri, const flx::core::Bundle& filter = {}) = 0;

	/**
	 * @brief Insert a new record
	 * @param uri The URI specifying the collection to insert into
	 * @param values The data to insert
	 * @return A URI referring to the newly inserted item, or empty string on failure
	 */
	virtual std::string insert(const std::string& uri, const flx::core::Bundle& values) { return std::string(); }

	/**
	 * @brief Update existing records
	 * @param uri The URI specifying the records to update
	 * @param values The new values to apply
	 * @param filter Constraints specifying which records to update
	 * @return The number of records modified
	 */
	virtual int update(const std::string& uri, const flx::core::Bundle& values, const flx::core::Bundle& filter) { return 0; }

	/**
	 * @brief Delete existing records
	 * @param uri The URI specifying the records to delete
	 * @param filter Constraints specifying which records to delete
	 * @return The number of records deleted
	 */
	virtual int remove(const std::string& uri, const flx::core::Bundle& filter) { return 0; }
};

} // namespace flx::apps
