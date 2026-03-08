#include <cstring>
#include <flx/core/Bundle.hpp>

namespace flx::core {

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

// ============================================================
// Serialization
// ============================================================

std::vector<uint8_t> Bundle::serialize() const {
	std::vector<uint8_t> out;
	uint32_t count = m_entries.size();
	out.insert(out.end(), reinterpret_cast<const uint8_t*>(&count), reinterpret_cast<const uint8_t*>(&count) + sizeof(count));

	for (const auto& [k, v]: m_entries) {
		uint16_t keyLen = k.size();
		out.insert(out.end(), reinterpret_cast<const uint8_t*>(&keyLen), reinterpret_cast<const uint8_t*>(&keyLen) + sizeof(keyLen));
		out.insert(out.end(), reinterpret_cast<const uint8_t*>(k.data()), reinterpret_cast<const uint8_t*>(k.data()) + k.size());
		uint8_t type = static_cast<uint8_t>(v.type);
		out.push_back(type);
		switch (v.type) {
			case Type::Bool:
				out.push_back(v.valueBool ? 1 : 0);
				break;
			case Type::Int32:
				out.insert(out.end(), reinterpret_cast<const uint8_t*>(&v.valueInt32), reinterpret_cast<const uint8_t*>(&v.valueInt32) + sizeof(int32_t));
				break;
			case Type::Int64:
				out.insert(out.end(), reinterpret_cast<const uint8_t*>(&v.valueInt64), reinterpret_cast<const uint8_t*>(&v.valueInt64) + sizeof(int64_t));
				break;
			case Type::Float:
				out.insert(out.end(), reinterpret_cast<const uint8_t*>(&v.valueFloat), reinterpret_cast<const uint8_t*>(&v.valueFloat) + sizeof(float));
				break;
			case Type::String: {
				uint32_t strLen = v.valueString.size();
				out.insert(out.end(), reinterpret_cast<const uint8_t*>(&strLen), reinterpret_cast<const uint8_t*>(&strLen) + sizeof(strLen));
				out.insert(out.end(), reinterpret_cast<const uint8_t*>(v.valueString.data()), reinterpret_cast<const uint8_t*>(v.valueString.data()) + v.valueString.size());
				break;
			}
			case Type::Blob: {
				uint32_t blobLen = v.valueBlob.size();
				out.insert(out.end(), reinterpret_cast<const uint8_t*>(&blobLen), reinterpret_cast<const uint8_t*>(&blobLen) + sizeof(blobLen));
				out.insert(out.end(), v.valueBlob.begin(), v.valueBlob.end());
				break;
			}
			case Type::Bundle: {
				auto sub = v.valueBundle ? v.valueBundle->serialize() : std::vector<uint8_t>();
				uint32_t subLen = sub.size();
				out.insert(out.end(), reinterpret_cast<const uint8_t*>(&subLen), reinterpret_cast<const uint8_t*>(&subLen) + sizeof(subLen));
				out.insert(out.end(), sub.begin(), sub.end());
				break;
			}
		}
	}
	return out;
}

Bundle Bundle::deserialize(const std::vector<uint8_t>& data) {
	Bundle result;
	if (data.empty()) return result;
	size_t offset = 0;
	if (offset + sizeof(uint32_t) > data.size()) return result;
	uint32_t count;
	std::memcpy(&count, &data[offset], sizeof(uint32_t));
	offset += sizeof(uint32_t);

	for (uint32_t i = 0; i < count; ++i) {
		if (offset + sizeof(uint16_t) > data.size()) break;
		uint16_t keyLen;
		std::memcpy(&keyLen, &data[offset], sizeof(uint16_t));
		offset += sizeof(uint16_t);

		if (keyLen > data.size() - offset) break;
		std::string key(reinterpret_cast<const char*>(&data[offset]), keyLen);
		offset += keyLen;

		if (offset + sizeof(uint8_t) > data.size()) break;
		Type type = static_cast<Type>(data[offset]);
		offset += sizeof(uint8_t);

		switch (type) {
			case Type::Bool: {
				if (offset + 1 > data.size()) return result;
				bool val = data[offset] != 0;
				offset += 1;
				result.putBool(key, val);
				break;
			}
			case Type::Int32: {
				if (offset + sizeof(int32_t) > data.size()) return result;
				int32_t val;
				std::memcpy(&val, &data[offset], sizeof(int32_t));
				offset += sizeof(int32_t);
				result.putInt32(key, val);
				break;
			}
			case Type::Int64: {
				if (offset + sizeof(int64_t) > data.size()) return result;
				int64_t val;
				std::memcpy(&val, &data[offset], sizeof(int64_t));
				offset += sizeof(int64_t);
				result.putInt64(key, val);
				break;
			}
			case Type::Float: {
				if (offset + sizeof(float) > data.size()) return result;
				float val;
				std::memcpy(&val, &data[offset], sizeof(float));
				offset += sizeof(float);
				result.putFloat(key, val);
				break;
			}
			case Type::String: {
				if (offset + sizeof(uint32_t) > data.size()) return result;
				uint32_t strLen;
				std::memcpy(&strLen, &data[offset], sizeof(uint32_t));
				offset += sizeof(uint32_t);
				if (strLen > data.size() - offset) return result;
				std::string val(reinterpret_cast<const char*>(&data[offset]), strLen);
				offset += strLen;
				result.putString(key, val);
				break;
			}
			case Type::Blob: {
				if (offset + sizeof(uint32_t) > data.size()) return result;
				uint32_t blobLen;
				std::memcpy(&blobLen, &data[offset], sizeof(uint32_t));
				offset += sizeof(uint32_t);
				if (blobLen > data.size() - offset) return result;
				std::vector<uint8_t> val(data.begin() + offset, data.begin() + offset + blobLen);
				offset += blobLen;
				result.putBlob(key, val);
				break;
			}
			case Type::Bundle: {
				if (offset + sizeof(uint32_t) > data.size()) return result;
				uint32_t subLen;
				std::memcpy(&subLen, &data[offset], sizeof(uint32_t));
				offset += sizeof(uint32_t);
				if (subLen > data.size() - offset) return result;
				std::vector<uint8_t> sub(data.begin() + offset, data.begin() + offset + subLen);
				offset += subLen;
				result.putBundle(key, Bundle::deserialize(sub));
				break;
			}
			default:
				return result; // Invalid type
		}
	}
	return result;
}

} // namespace flx::core
