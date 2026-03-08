#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace flx::core {

class Preferences {
public:

	explicit Preferences(std::string nameSpace);

	void putBool(const std::string& key, bool value);
	void putInt32(const std::string& key, int32_t value);
	void putInt64(const std::string& key, int64_t value);
	void putString(const std::string& key, const std::string& value);

	bool optBool(const std::string& key, bool& out) const;
	bool optInt32(const std::string& key, int32_t& out) const;
	bool optInt64(const std::string& key, int64_t& out) const;
	bool optString(const std::string& key, std::string& out) const;

	void putFloat(const std::string& key, float value);
	void putBlob(const std::string& key, const std::vector<uint8_t>& data);
	bool optFloat(const std::string& key, float& out) const;
	bool optBlob(const std::string& key, std::vector<uint8_t>& out) const;

	bool getBoolOr(const std::string& key, bool def) const;
	int32_t getInt32Or(const std::string& key, int32_t def) const;
	std::string getStringOr(const std::string& key, const std::string& def) const;

	bool hasKey(const std::string& key) const;
	bool erase(const std::string& key);
	bool eraseAll();

	size_t usedEntries() const;
	size_t freeEntries() const;

	const std::string& getNamespace() const { return m_requestedNamespace; }
	const std::string& getStorageNamespace() const { return m_storageNamespace; }

private:

	std::string m_requestedNamespace;
	std::string m_storageNamespace;
};

} // namespace flx::core
