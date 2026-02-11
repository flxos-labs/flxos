#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace System::Apps {

/**
 * @brief Typed key-value data container for inter-app communication
 *
 * Used to pass structured data between apps, in intents, and in event payloads.
 *
 *
 * Features:
 * - Float support
 * - Binary blob support (std::vector<uint8_t>)
 * - Nested Bundle support for complex structured data
 * - Introspection APIs: size(), clear(), hasKey(), keys()
 */
class Bundle final {
public:

	enum class Type : uint8_t {
		Bool,
		Int32,
		Int64,
		Float,
		String,
		Blob,
		Bundle,
	};

	Bundle() = default;
	Bundle(const Bundle& other);
	Bundle& operator=(const Bundle& other);
	Bundle(Bundle&&) = default;
	Bundle& operator=(Bundle&&) = default;
	~Bundle() = default;

	// === Typed putters ===

	void putBool(const std::string& key, bool value);
	void putInt32(const std::string& key, int32_t value);
	void putInt64(const std::string& key, int64_t value);
	void putFloat(const std::string& key, float value);
	void putString(const std::string& key, const std::string& value);
	void putBlob(const std::string& key, const std::vector<uint8_t>& value);
	void putBlob(const std::string& key, std::vector<uint8_t>&& value);
	void putBundle(const std::string& key, const Bundle& value);

	// === Typed getters (undefined behavior if key missing — use has*() first) ===

	bool getBool(const std::string& key) const;
	int32_t getInt32(const std::string& key) const;
	int64_t getInt64(const std::string& key) const;
	float getFloat(const std::string& key) const;
	std::string getString(const std::string& key) const;
	const std::vector<uint8_t>& getBlob(const std::string& key) const;
	const Bundle& getBundle(const std::string& key) const;

	// === Default-value getters (safe, no UB if key missing) ===

	bool getBoolOr(const std::string& key, bool defaultValue) const;
	int32_t getInt32Or(const std::string& key, int32_t defaultValue) const;
	int64_t getInt64Or(const std::string& key, int64_t defaultValue) const;
	float getFloatOr(const std::string& key, float defaultValue) const;
	std::string getStringOr(const std::string& key, const std::string& defaultValue) const;

	// === Type-checked presence queries ===

	bool hasBool(const std::string& key) const;
	bool hasInt32(const std::string& key) const;
	bool hasInt64(const std::string& key) const;
	bool hasFloat(const std::string& key) const;
	bool hasString(const std::string& key) const;
	bool hasBlob(const std::string& key) const;
	bool hasBundle(const std::string& key) const;

	// === Safe optional getters (return true if found + correct type) ===

	bool optBool(const std::string& key, bool& out) const;
	bool optInt32(const std::string& key, int32_t& out) const;
	bool optInt64(const std::string& key, int64_t& out) const;
	bool optFloat(const std::string& key, float& out) const;
	bool optString(const std::string& key, std::string& out) const;

	// === Introspection ===

	bool hasKey(const std::string& key) const;
	size_t size() const;
	bool empty() const;
	void clear();
	std::vector<std::string> keys() const;

private:

	struct Value {
		Type type;
		union {
			bool valueBool;
			int32_t valueInt32;
			int64_t valueInt64;
			float valueFloat;
		};
		std::string valueString;
		std::vector<uint8_t> valueBlob;
		std::unique_ptr<Bundle> valueBundle;

		Value() : type(Type::Bool), valueBool(false) {}
		Value(const Value& other);
		Value& operator=(const Value& other);
		Value(Value&&) = default;
		Value& operator=(Value&&) = default;
		~Value() = default;
	};

	std::unordered_map<std::string, Value> m_entries;

	const Value* findEntry(const std::string& key, Type expectedType) const;
};

} // namespace System::Apps
