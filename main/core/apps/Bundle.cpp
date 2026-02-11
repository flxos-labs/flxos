#include "Bundle.hpp"

namespace System::Apps {

// ============================================================
// Value copy constructor & assignment (deep copy nested Bundle)
// ============================================================

Bundle::Value::Value(const Value& other)
	: type(other.type),
	  valueString(other.valueString),
	  valueBlob(other.valueBlob) {
	switch (type) {
		case Type::Bool:
			valueBool = other.valueBool;
			break;
		case Type::Int32:
			valueInt32 = other.valueInt32;
			break;
		case Type::Int64:
			valueInt64 = other.valueInt64;
			break;
		case Type::Float:
			valueFloat = other.valueFloat;
			break;
		default:
			valueBool = false;
			break;
	}
	if (other.valueBundle) {
		valueBundle = std::make_unique<Bundle>(*other.valueBundle);
	}
}

Bundle::Value& Bundle::Value::operator=(const Value& other) {
	if (this == &other) return *this;
	type = other.type;
	valueString = other.valueString;
	valueBlob = other.valueBlob;
	switch (type) {
		case Type::Bool:
			valueBool = other.valueBool;
			break;
		case Type::Int32:
			valueInt32 = other.valueInt32;
			break;
		case Type::Int64:
			valueInt64 = other.valueInt64;
			break;
		case Type::Float:
			valueFloat = other.valueFloat;
			break;
		default:
			valueBool = false;
			break;
	}
	if (other.valueBundle) {
		valueBundle = std::make_unique<Bundle>(*other.valueBundle);
	} else {
		valueBundle.reset();
	}
	return *this;
}

// ============================================================
// Bundle copy constructor & assignment
// ============================================================

Bundle::Bundle(const Bundle& other) {
	m_entries = other.m_entries;
}

Bundle& Bundle::operator=(const Bundle& other) {
	if (this != &other) {
		m_entries = other.m_entries;
	}
	return *this;
}

// ============================================================
// Typed putters
// ============================================================

void Bundle::putBool(const std::string& key, bool value) {
	Value v;
	v.type = Type::Bool;
	v.valueBool = value;
	m_entries[key] = std::move(v);
}

void Bundle::putInt32(const std::string& key, int32_t value) {
	Value v;
	v.type = Type::Int32;
	v.valueInt32 = value;
	m_entries[key] = std::move(v);
}

void Bundle::putInt64(const std::string& key, int64_t value) {
	Value v;
	v.type = Type::Int64;
	v.valueInt64 = value;
	m_entries[key] = std::move(v);
}

void Bundle::putFloat(const std::string& key, float value) {
	Value v;
	v.type = Type::Float;
	v.valueFloat = value;
	m_entries[key] = std::move(v);
}

void Bundle::putString(const std::string& key, const std::string& value) {
	Value v;
	v.type = Type::String;
	v.valueString = value;
	m_entries[key] = std::move(v);
}

void Bundle::putBlob(const std::string& key, const std::vector<uint8_t>& value) {
	Value v;
	v.type = Type::Blob;
	v.valueBlob = value;
	m_entries[key] = std::move(v);
}

void Bundle::putBlob(const std::string& key, std::vector<uint8_t>&& value) {
	Value v;
	v.type = Type::Blob;
	v.valueBlob = std::move(value);
	m_entries[key] = std::move(v);
}

void Bundle::putBundle(const std::string& key, const Bundle& value) {
	Value v;
	v.type = Type::Bundle;
	v.valueBundle = std::make_unique<Bundle>(value);
	m_entries[key] = std::move(v);
}

// ============================================================
// Typed getters
// ============================================================

bool Bundle::getBool(const std::string& key) const {
	auto it = m_entries.find(key);
	return it != m_entries.end() ? it->second.valueBool : false;
}

int32_t Bundle::getInt32(const std::string& key) const {
	auto it = m_entries.find(key);
	return it != m_entries.end() ? it->second.valueInt32 : 0;
}

int64_t Bundle::getInt64(const std::string& key) const {
	auto it = m_entries.find(key);
	return it != m_entries.end() ? it->second.valueInt64 : 0;
}

float Bundle::getFloat(const std::string& key) const {
	auto it = m_entries.find(key);
	return it != m_entries.end() ? it->second.valueFloat : 0.0f;
}

std::string Bundle::getString(const std::string& key) const {
	auto it = m_entries.find(key);
	return it != m_entries.end() ? it->second.valueString : "";
}

const std::vector<uint8_t>& Bundle::getBlob(const std::string& key) const {
	static const std::vector<uint8_t> empty;
	auto it = m_entries.find(key);
	return it != m_entries.end() ? it->second.valueBlob : empty;
}

const Bundle& Bundle::getBundle(const std::string& key) const {
	static const Bundle empty;
	auto it = m_entries.find(key);
	if (it != m_entries.end() && it->second.valueBundle) {
		return *it->second.valueBundle;
	}
	return empty;
}

// ============================================================
// Type-checked presence queries
// ============================================================

const Bundle::Value* Bundle::findEntry(const std::string& key, Type expectedType) const {
	auto it = m_entries.find(key);
	if (it != m_entries.end() && it->second.type == expectedType) {
		return &it->second;
	}
	return nullptr;
}

bool Bundle::hasBool(const std::string& key) const { return findEntry(key, Type::Bool) != nullptr; }
bool Bundle::hasInt32(const std::string& key) const { return findEntry(key, Type::Int32) != nullptr; }
bool Bundle::hasInt64(const std::string& key) const { return findEntry(key, Type::Int64) != nullptr; }
bool Bundle::hasFloat(const std::string& key) const { return findEntry(key, Type::Float) != nullptr; }
bool Bundle::hasString(const std::string& key) const { return findEntry(key, Type::String) != nullptr; }
bool Bundle::hasBlob(const std::string& key) const { return findEntry(key, Type::Blob) != nullptr; }
bool Bundle::hasBundle(const std::string& key) const { return findEntry(key, Type::Bundle) != nullptr; }

// ============================================================
// Safe optional getters
// ============================================================

bool Bundle::optBool(const std::string& key, bool& out) const {
	auto* v = findEntry(key, Type::Bool);
	if (v) {
		out = v->valueBool;
		return true;
	}
	return false;
}

bool Bundle::optInt32(const std::string& key, int32_t& out) const {
	auto* v = findEntry(key, Type::Int32);
	if (v) {
		out = v->valueInt32;
		return true;
	}
	return false;
}

bool Bundle::optInt64(const std::string& key, int64_t& out) const {
	auto* v = findEntry(key, Type::Int64);
	if (v) {
		out = v->valueInt64;
		return true;
	}
	return false;
}

bool Bundle::optFloat(const std::string& key, float& out) const {
	auto* v = findEntry(key, Type::Float);
	if (v) {
		out = v->valueFloat;
		return true;
	}
	return false;
}

bool Bundle::optString(const std::string& key, std::string& out) const {
	auto* v = findEntry(key, Type::String);
	if (v) {
		out = v->valueString;
		return true;
	}
	return false;
}

// ============================================================
// Default-value getters
// ============================================================

bool Bundle::getBoolOr(const std::string& key, bool defaultValue) const {
	auto* v = findEntry(key, Type::Bool);
	return v ? v->valueBool : defaultValue;
}

int32_t Bundle::getInt32Or(const std::string& key, int32_t defaultValue) const {
	auto* v = findEntry(key, Type::Int32);
	return v ? v->valueInt32 : defaultValue;
}

int64_t Bundle::getInt64Or(const std::string& key, int64_t defaultValue) const {
	auto* v = findEntry(key, Type::Int64);
	return v ? v->valueInt64 : defaultValue;
}

float Bundle::getFloatOr(const std::string& key, float defaultValue) const {
	auto* v = findEntry(key, Type::Float);
	return v ? v->valueFloat : defaultValue;
}

std::string Bundle::getStringOr(const std::string& key, const std::string& defaultValue) const {
	auto* v = findEntry(key, Type::String);
	return v ? v->valueString : defaultValue;
}

// ============================================================
// Introspection
// ============================================================

bool Bundle::hasKey(const std::string& key) const {
	return m_entries.find(key) != m_entries.end();
}

size_t Bundle::size() const { return m_entries.size(); }
bool Bundle::empty() const { return m_entries.empty(); }
void Bundle::clear() { m_entries.clear(); }

std::vector<std::string> Bundle::keys() const {
	std::vector<std::string> result;
	result.reserve(m_entries.size());
	for (const auto& [key, _]: m_entries) {
		result.push_back(key);
	}
	return result;
}

} // namespace System::Apps
