#include <flx/core/Preferences.hpp>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <flx/core/Logger.hpp>
#include <nvs.h>
#include <utility>
#include <vector>

namespace flx::core {
namespace {

static constexpr const char* TAG = "Preferences";
static constexpr size_t MAX_NVS_NAME_LEN = NVS_KEY_NAME_MAX_SIZE - 1;

uint64_t fnv1a64(const std::string& input) {
	uint64_t hash = 1469598103934665603ULL;
	for (unsigned char ch: input) {
		hash ^= ch;
		hash *= 1099511628211ULL;
	}
	return hash;
}

bool isNvsSafeName(const std::string& value) {
	if (value.empty() || value.size() > MAX_NVS_NAME_LEN) {
		return false;
	}
	for (unsigned char ch: value) {
		if (!(std::isalnum(ch) || ch == '_')) {
			return false;
		}
	}
	return true;
}

std::string hashName(const std::string& value, char prefix) {
	char out[MAX_NVS_NAME_LEN + 1] = {};
	std::snprintf(out, sizeof(out), "%c%014llx", prefix, static_cast<unsigned long long>(fnv1a64(value) & 0x00FFFFFFFFFFFFFFULL));
	return out;
}

char namespacePrefix(const std::string& value) {
	if (value.rfind("app.", 0) == 0) {
		return 'a';
	}
	if (value.rfind("svc.", 0) == 0) {
		return 's';
	}
	return 'n';
}

std::string normalizeNamespace(const std::string& value) {
	if (isNvsSafeName(value)) {
		return value;
	}
	return hashName(value, namespacePrefix(value));
}

std::string normalizeKey(const std::string& value) {
	if (isNvsSafeName(value)) {
		return value;
	}
	return hashName(value, 'k');
}

bool openHandle(const Preferences& preferences, nvs_open_mode_t mode, nvs_handle_t& outHandle) {
	esp_err_t err = nvs_open(preferences.getStorageNamespace().c_str(), mode, &outHandle);
	if (err != ESP_OK) {
		if (err != ESP_ERR_NVS_NOT_FOUND) {
			Log::error(TAG, "Failed to open namespace '%s' (%s): %s", preferences.getNamespace().c_str(), preferences.getStorageNamespace().c_str(), esp_err_to_name(err));
		}
		return false;
	}
	return true;
}

bool commitOrLog(nvs_handle_t handle, const std::string& nameSpace, const std::string& key) {
	esp_err_t err = nvs_commit(handle);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to commit %s:%s: %s", nameSpace.c_str(), key.c_str(), esp_err_to_name(err));
		return false;
	}
	return true;
}

} // namespace

Preferences::Preferences(std::string nameSpace)
	: m_requestedNamespace(std::move(nameSpace)),
	  m_storageNamespace(normalizeNamespace(m_requestedNamespace)) {}

void Preferences::putBool(const std::string& key, bool value) {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READWRITE, handle)) {
		return;
	}

	const std::string storageKey = normalizeKey(key);
	esp_err_t err = nvs_set_u8(handle, storageKey.c_str(), value ? 1 : 0);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to set %s:%s: %s", m_requestedNamespace.c_str(), key.c_str(), esp_err_to_name(err));
		nvs_close(handle);
		return;
	}

	commitOrLog(handle, m_requestedNamespace, key);
	nvs_close(handle);
}

void Preferences::putInt32(const std::string& key, int32_t value) {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READWRITE, handle)) {
		return;
	}

	const std::string storageKey = normalizeKey(key);
	esp_err_t err = nvs_set_i32(handle, storageKey.c_str(), value);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to set %s:%s: %s", m_requestedNamespace.c_str(), key.c_str(), esp_err_to_name(err));
		nvs_close(handle);
		return;
	}

	commitOrLog(handle, m_requestedNamespace, key);
	nvs_close(handle);
}

void Preferences::putInt64(const std::string& key, int64_t value) {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READWRITE, handle)) {
		return;
	}

	const std::string storageKey = normalizeKey(key);
	esp_err_t err = nvs_set_i64(handle, storageKey.c_str(), value);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to set %s:%s: %s", m_requestedNamespace.c_str(), key.c_str(), esp_err_to_name(err));
		nvs_close(handle);
		return;
	}

	commitOrLog(handle, m_requestedNamespace, key);
	nvs_close(handle);
}

void Preferences::putString(const std::string& key, const std::string& value) {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READWRITE, handle)) {
		return;
	}

	const std::string storageKey = normalizeKey(key);
	esp_err_t err = nvs_set_str(handle, storageKey.c_str(), value.c_str());
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to set %s:%s: %s", m_requestedNamespace.c_str(), key.c_str(), esp_err_to_name(err));
		nvs_close(handle);
		return;
	}

	commitOrLog(handle, m_requestedNamespace, key);
	nvs_close(handle);
}

bool Preferences::optBool(const std::string& key, bool& out) const {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READONLY, handle)) {
		return false;
	}

	uint8_t value = 0;
	esp_err_t err = nvs_get_u8(handle, normalizeKey(key).c_str(), &value);
	nvs_close(handle);
	if (err != ESP_OK) {
		return false;
	}

	out = (value != 0);
	return true;
}

bool Preferences::optInt32(const std::string& key, int32_t& out) const {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READONLY, handle)) {
		return false;
	}

	esp_err_t err = nvs_get_i32(handle, normalizeKey(key).c_str(), &out);
	nvs_close(handle);
	return err == ESP_OK;
}

bool Preferences::optInt64(const std::string& key, int64_t& out) const {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READONLY, handle)) {
		return false;
	}

	esp_err_t err = nvs_get_i64(handle, normalizeKey(key).c_str(), &out);
	nvs_close(handle);
	return err == ESP_OK;
}

bool Preferences::optString(const std::string& key, std::string& out) const {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READONLY, handle)) {
		return false;
	}

	const std::string storageKey = normalizeKey(key);
	size_t required = 0;
	esp_err_t err = nvs_get_str(handle, storageKey.c_str(), nullptr, &required);
	if (err != ESP_OK) {
		nvs_close(handle);
		return false;
	}

	std::vector<char> buffer(required);
	err = nvs_get_str(handle, storageKey.c_str(), buffer.data(), &required);
	nvs_close(handle);
	if (err != ESP_OK) {
		return false;
	}

	out.assign(buffer.data());
	return true;
}

void Preferences::putFloat(const std::string& key, float value) {
	uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));

	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READWRITE, handle)) {
		return;
	}

	const std::string storageKey = normalizeKey(key);
	esp_err_t err = nvs_set_u32(handle, storageKey.c_str(), bits);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to set %s:%s: %s", m_requestedNamespace.c_str(), key.c_str(), esp_err_to_name(err));
		nvs_close(handle);
		return;
	}

	commitOrLog(handle, m_requestedNamespace, key);
	nvs_close(handle);
}

void Preferences::putBlob(const std::string& key, const std::vector<uint8_t>& data) {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READWRITE, handle)) {
		return;
	}

	static const uint8_t emptyByte = 0;
	const void* blobPtr = data.empty() ? static_cast<const void*>(&emptyByte) : static_cast<const void*>(data.data());
	const std::string storageKey = normalizeKey(key);
	esp_err_t err = nvs_set_blob(handle, storageKey.c_str(), blobPtr, data.size());
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to set %s:%s: %s", m_requestedNamespace.c_str(), key.c_str(), esp_err_to_name(err));
		nvs_close(handle);
		return;
	}

	commitOrLog(handle, m_requestedNamespace, key);
	nvs_close(handle);
}

bool Preferences::optFloat(const std::string& key, float& out) const {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READONLY, handle)) {
		return false;
	}

	uint32_t bits = 0;
	esp_err_t err = nvs_get_u32(handle, normalizeKey(key).c_str(), &bits);
	nvs_close(handle);
	if (err != ESP_OK) {
		return false;
	}

	std::memcpy(&out, &bits, sizeof(out));
	return true;
}

bool Preferences::optBlob(const std::string& key, std::vector<uint8_t>& out) const {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READONLY, handle)) {
		return false;
	}

	const std::string storageKey = normalizeKey(key);
	size_t len = 0;
	esp_err_t err = nvs_get_blob(handle, storageKey.c_str(), nullptr, &len);
	if (err != ESP_OK) {
		nvs_close(handle);
		return false;
	}

	out.resize(len);
	if (len > 0) {
		err = nvs_get_blob(handle, storageKey.c_str(), out.data(), &len);
		if (err != ESP_OK) {
			nvs_close(handle);
			return false;
		}
	}

	nvs_close(handle);
	return true;
}

bool Preferences::getBoolOr(const std::string& key, bool def) const {
	bool out = def;
	optBool(key, out);
	return out;
}

int32_t Preferences::getInt32Or(const std::string& key, int32_t def) const {
	int32_t out = def;
	optInt32(key, out);
	return out;
}

std::string Preferences::getStringOr(const std::string& key, const std::string& def) const {
	std::string out = def;
	optString(key, out);
	return out;
}

bool Preferences::hasKey(const std::string& key) const {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READONLY, handle)) {
		return false;
	}

	nvs_type_t type = NVS_TYPE_ANY;
	esp_err_t err = nvs_find_key(handle, normalizeKey(key).c_str(), &type);
	nvs_close(handle);
	return err == ESP_OK;
}

bool Preferences::erase(const std::string& key) {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READWRITE, handle)) {
		return false;
	}

	esp_err_t err = nvs_erase_key(handle, normalizeKey(key).c_str());
	if (err != ESP_OK) {
		nvs_close(handle);
		return false;
	}

	bool committed = commitOrLog(handle, m_requestedNamespace, key);
	nvs_close(handle);
	return committed;
}

bool Preferences::eraseAll() {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READWRITE, handle)) {
		return false;
	}

	esp_err_t err = nvs_erase_all(handle);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to erase namespace %s: %s", m_requestedNamespace.c_str(), esp_err_to_name(err));
		nvs_close(handle);
		return false;
	}

	bool committed = commitOrLog(handle, m_requestedNamespace, "*");
	nvs_close(handle);
	return committed;
}

size_t Preferences::usedEntries() const {
	nvs_handle_t handle = 0;
	if (!openHandle(*this, NVS_READONLY, handle)) {
		return 0;
	}

	size_t entries = 0;
	esp_err_t err = nvs_get_used_entry_count(handle, &entries);
	nvs_close(handle);
	return err == ESP_OK ? entries : 0;
}

size_t Preferences::freeEntries() const {
	nvs_stats_t stats {};
	esp_err_t err = nvs_get_stats(nullptr, &stats);
	return err == ESP_OK ? stats.free_entries : 0;
}

} // namespace flx::core
