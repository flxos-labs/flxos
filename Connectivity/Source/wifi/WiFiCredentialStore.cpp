#include "flx/connectivity/wifi/WiFiCredentialStore.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <flx/core/Logger.hpp>
#include <flx/core/PathUtils.hpp>
#include <flx/core/Value.hpp>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include <esp_mac.h>
#include <esp_random.h>
#include <mbedtls/base64.h>
#include <psa/crypto.h>

static constexpr std::string_view TAG = "WiFiCredentialStore";

// JSON field names
static constexpr const char* KEY_SSID = "ssid";
static constexpr const char* KEY_AUTO_CONNECT = "autoConnect";
static constexpr const char* KEY_PRIORITY = "priority";
static constexpr const char* KEY_LAST_CONNECTED_MS = "lastConnectedMs";

namespace flx::connectivity {

// ──────────────────────────────────────────────────────────────────────────────
// FNV-1a hash → 6-hex filename (filesystem-safe for any SSID content)
// ──────────────────────────────────────────────────────────────────────────────
std::string WiFiCredentialStore::ssidToFilename(const std::string& ssid) {
	// FNV-1a 32-bit
	uint32_t hash = 2166136261u;
	for (unsigned char c: ssid) {
		hash ^= static_cast<uint32_t>(c);
		hash *= 16777619u;
	}
	char buf[16];
	snprintf(buf, sizeof(buf), "%08" PRIx32, hash);
	return std::string(buf);
}

std::string WiFiCredentialStore::filePath(const std::string& ssid) {
	return std::string(DATA_DIR) + "/" + ssidToFilename(ssid) + ".json";
}

// ──────────────────────────────────────────────────────────────────────────────
// Encryption key and helpers
// ──────────────────────────────────────────────────────────────────────────────
void WiFiCredentialStore::getEncryptionKey(uint8_t key[32]) {
	uint8_t mac[6] = {};
	esp_read_mac(mac, ESP_MAC_WIFI_STA);

	psa_crypto_init();

	size_t hash_length = 0;
	const char* salt = "flxos_wifi_secure_salt_2026";
	size_t salt_len = strlen(salt);
	std::vector<uint8_t> input(6 + salt_len);
	memcpy(input.data(), mac, 6);
	memcpy(input.data() + 6, salt, salt_len);

	psa_hash_compute(PSA_ALG_SHA_256, input.data(), input.size(), key, 32, &hash_length);
}

bool WiFiCredentialStore::encryptPassword(const std::string& plaintext, std::string& out_ciphertext_b64, std::string& out_iv_b64) {
	if (plaintext.empty()) {
		out_ciphertext_b64 = "";
		out_iv_b64 = "";
		return true;
	}

	uint8_t key[32];
	getEncryptionKey(key);

	// Setup key attributes
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 256);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_CBC_PKCS7);

	psa_key_id_t key_id = 0;
	if (psa_import_key(&attributes, key, sizeof(key), &key_id) != PSA_SUCCESS) {
		return false;
	}

	size_t output_size = plaintext.size() + 32;
	std::vector<uint8_t> output_buf(output_size);
	size_t output_len = 0;

	psa_status_t status = psa_cipher_encrypt(
		key_id,
		PSA_ALG_CBC_PKCS7,
		reinterpret_cast<const uint8_t*>(plaintext.data()),
		plaintext.size(),
		output_buf.data(),
		output_buf.size(),
		&output_len);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS || output_len < 16) {
		return false;
	}

	// In PSA CBC PKCS7, the IV (16 bytes) is prepended to the ciphertext.
	unsigned char iv_b64_buf[32] = {};
	size_t written = 0;
	if (mbedtls_base64_encode(iv_b64_buf, sizeof(iv_b64_buf), &written, output_buf.data(), 16) != 0) {
		return false;
	}
	out_iv_b64 = std::string(reinterpret_cast<char*>(iv_b64_buf), written);

	size_t ciphertext_len = output_len - 16;
	size_t ciphertext_b64_len_needed = (ciphertext_len + 2) / 3 * 4 + 5;
	std::vector<unsigned char> ciphertext_b64_buf(ciphertext_b64_len_needed);
	written = 0;
	if (mbedtls_base64_encode(ciphertext_b64_buf.data(), ciphertext_b64_buf.size(), &written, output_buf.data() + 16, ciphertext_len) != 0) {
		return false;
	}
	out_ciphertext_b64 = std::string(reinterpret_cast<char*>(ciphertext_b64_buf.data()), written);

	return true;
}

bool WiFiCredentialStore::decryptPassword(const std::string& ciphertext_b64, const std::string& iv_b64, std::string& out_plaintext) {
	if (ciphertext_b64.empty()) {
		out_plaintext = "";
		return true;
	}

	uint8_t key[32];
	getEncryptionKey(key);

	// Decode IV
	uint8_t iv[16] = {};
	size_t decoded_iv_len = 0;
	if (mbedtls_base64_decode(iv, sizeof(iv), &decoded_iv_len, reinterpret_cast<const unsigned char*>(iv_b64.data()), iv_b64.size()) != 0 || decoded_iv_len != 16) {
		return false;
	}

	// Decode ciphertext
	size_t decoded_ciphertext_len_needed = ciphertext_b64.size() * 3 / 4 + 5;
	std::vector<uint8_t> ciphertext(decoded_ciphertext_len_needed);
	size_t decoded_ciphertext_len = 0;
	if (mbedtls_base64_decode(ciphertext.data(), ciphertext.size(), &decoded_ciphertext_len, reinterpret_cast<const unsigned char*>(ciphertext_b64.data()), ciphertext_b64.size()) != 0) {
		return false;
	}

	if (decoded_ciphertext_len == 0 || (decoded_ciphertext_len % 16) != 0) {
		return false;
	}

	// Concatenate IV + Ciphertext for psa_cipher_decrypt
	std::vector<uint8_t> psa_input(16 + decoded_ciphertext_len);
	memcpy(psa_input.data(), iv, 16);
	memcpy(psa_input.data() + 16, ciphertext.data(), decoded_ciphertext_len);

	// Setup key attributes
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 256);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_CBC_PKCS7);

	psa_key_id_t key_id = 0;
	if (psa_import_key(&attributes, key, sizeof(key), &key_id) != PSA_SUCCESS) {
		return false;
	}

	std::vector<uint8_t> plaintext_buf(decoded_ciphertext_len + 16);
	size_t plaintext_len = 0;

	psa_status_t status = psa_cipher_decrypt(
		key_id,
		PSA_ALG_CBC_PKCS7,
		psa_input.data(),
		psa_input.size(),
		plaintext_buf.data(),
		plaintext_buf.size(),
		&plaintext_len);

	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		return false;
	}

	out_plaintext = std::string(reinterpret_cast<char*>(plaintext_buf.data()), plaintext_len);
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// JSON serialisation helpers (hand-rolled to avoid heavy deps on embedded)
// ──────────────────────────────────────────────────────────────────────────────
static std::string escapeJson(const std::string& s) {
	std::string out;
	out.reserve(s.size() + 4);
	for (char c: s) {
		switch (c) {
			case '"':
				out += "\\\"";
				break;
			case '\\':
				out += "\\\\";
				break;
			case '\n':
				out += "\\n";
				break;
			case '\r':
				out += "\\r";
				break;
			case '\t':
				out += "\\t";
				break;
			default:
				out.push_back(c);
				break;
		}
	}
	return out;
}

bool WiFiCredentialStore::writeCredential(const std::string& path, const WiFiCredential& cred) {
	// Ensure parent directory exists
	if (!flx::core::ensureDirectoryExists(DATA_DIR)) {
		Log::error(TAG, "Failed to create credential directory");
		return false;
	}

	std::string password_enc;
	std::string password_iv;
	if (!encryptPassword(cred.password, password_enc, password_iv)) {
		Log::error(TAG, "Failed to encrypt password for SSID: %s", cred.ssid.c_str());
		return false;
	}

	FILE* f = fopen(path.c_str(), "w");
	if (!f) {
		Log::error(TAG, "Failed to open %s for writing: %s", path.c_str(), strerror(errno));
		return false;
	}

	// Build hex representation of BSSID
	char bssid_hex[18] = {};
	snprintf(bssid_hex, sizeof(bssid_hex), "%02x:%02x:%02x:%02x:%02x:%02x",
		cred.lastBssid[0], cred.lastBssid[1], cred.lastBssid[2],
		cred.lastBssid[3], cred.lastBssid[4], cred.lastBssid[5]);

	// Build compact JSON manually
	fprintf(f,
		"{\"ssid\":\"%s\",\"passwordEnc\":\"%s\",\"passwordIv\":\"%s\",\"autoConnect\":%s,\"priority\":%d,\"lastConnectedMs\":%" PRId64 ",\"lastChannel\":%d,\"lastBssid\":\"%s\"}",
		escapeJson(cred.ssid).c_str(),
		escapeJson(password_enc).c_str(),
		escapeJson(password_iv).c_str(),
		cred.autoConnect ? "true" : "false",
		cred.priority,
		cred.lastConnectedMs,
		static_cast<int>(cred.lastChannel),
		bssid_hex);

	fclose(f);
	return true;
}

bool WiFiCredentialStore::readCredential(const std::string& path, WiFiCredential& out) {
	FILE* f = fopen(path.c_str(), "r");
	if (!f) {
		return false;
	}

	// Read entire file into string
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (len <= 0 || len > 4096) {
		fclose(f);
		Log::warn(TAG, "Credential file too large or empty: %s", path.c_str());
		return false;
	}

	std::string content(static_cast<size_t>(len), '\0');
	size_t read_bytes = fread(&content[0], 1, static_cast<size_t>(len), f);
	fclose(f);
	content.resize(read_bytes);

	auto doc = flx::core::FlxValueDocument::parseJson(std::move(content));
	if (!doc) {
		Log::error(TAG, "Failed to parse JSON: %s", path.c_str());
		return false;
	}

	auto root = doc->root();
	if (!root.isMap()) {
		return false;
	}

	out.ssid = root.child(KEY_SSID).asString();

	// Check if legacy plaintext password exists, and decrypt if not
	bool needs_migration = false;
	if (root.hasChild("password")) {
		out.password = root.child("password").asString();
		needs_migration = true;
	} else {
		std::string password_enc = root.child("passwordEnc").asString();
		std::string password_iv = root.child("passwordIv").asString();
		if (!decryptPassword(password_enc, password_iv, out.password)) {
			Log::error(TAG, "Failed to decrypt password for SSID: %s", out.ssid.c_str());
			return false;
		}
	}

	out.autoConnect = root.child(KEY_AUTO_CONNECT).asBool(true);
	out.priority = static_cast<int>(root.child(KEY_PRIORITY).asInt64(0));
	out.lastConnectedMs = root.child(KEY_LAST_CONNECTED_MS).asInt64(0);

	out.lastChannel = static_cast<int32_t>(root.child("lastChannel").asInt64(0));

	std::string bssid_str = root.child("lastBssid").asString();
	memset(out.lastBssid, 0, 6);
	if (!bssid_str.empty()) {
		unsigned int b[6] = {};
		if (sscanf(bssid_str.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) == 6) {
			for (int i = 0; i < 6; ++i) {
				out.lastBssid[i] = static_cast<uint8_t>(b[i]);
			}
		}
	}

	if (needs_migration && !out.ssid.empty()) {
		Log::info(TAG, "Migrating plaintext credentials to encrypted for SSID: %s", out.ssid.c_str());
		writeCredential(path, out);
	}

	return !out.ssid.empty();
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────
esp_err_t WiFiCredentialStore::save(const WiFiCredential& cred) {
	if (cred.ssid.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	const std::string path = filePath(cred.ssid);
	return writeCredential(path, cred) ? ESP_OK : ESP_FAIL;
}

bool WiFiCredentialStore::load(const std::string& ssid, WiFiCredential& out) const {
	if (ssid.empty()) return false;
	std::lock_guard<std::mutex> lock(m_mutex);
	return readCredential(filePath(ssid), out);
}

esp_err_t WiFiCredentialStore::remove(const std::string& ssid) {
	if (ssid.empty()) return ESP_ERR_INVALID_ARG;
	std::lock_guard<std::mutex> lock(m_mutex);
	const std::string path = filePath(ssid);
	struct stat st {};
	if (stat(path.c_str(), &st) != 0) {
		return ESP_ERR_NOT_FOUND;
	}
	return ::remove(path.c_str()) == 0 ? ESP_OK : ESP_FAIL;
}

bool WiFiCredentialStore::contains(const std::string& ssid) const {
	if (ssid.empty()) return false;
	std::lock_guard<std::mutex> lock(m_mutex);
	struct stat st {};
	return stat(filePath(ssid).c_str(), &st) == 0;
}

std::vector<WiFiCredential> WiFiCredentialStore::loadAll() const {
	std::lock_guard<std::mutex> lock(m_mutex);

	std::vector<WiFiCredential> results;

	DIR* dir = opendir(DATA_DIR);
	if (!dir) {
		// Directory doesn't exist yet — no credentials saved
		return results;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) {
			continue;
		}
		std::string filename = entry->d_name;
		if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".json") {
			continue;
		}
		std::string path = std::string(DATA_DIR) + "/" + filename;
		WiFiCredential cred;
		if (readCredential(path, cred)) {
			results.push_back(std::move(cred));
		}
	}
	closedir(dir);

	// Sort: priority DESC, then lastConnectedMs DESC (most recent first)
	std::sort(results.begin(), results.end(), [](const WiFiCredential& a, const WiFiCredential& b) {
		if (a.priority != b.priority) {
			return a.priority > b.priority;
		}
		return a.lastConnectedMs > b.lastConnectedMs;
	});

	return results;
}

void WiFiCredentialStore::updateLastConnected(const std::string& ssid) {
	if (ssid.empty()) return;
	std::lock_guard<std::mutex> lock(m_mutex);
	WiFiCredential cred;
	const std::string path = filePath(ssid);
	if (!readCredential(path, cred)) {
		// Not saved — nothing to update
		return;
	}
	// Use esp_timer for monotonic ms since epoch approximation
	struct timespec ts {};
	clock_gettime(CLOCK_REALTIME, &ts);
	cred.lastConnectedMs = static_cast<int64_t>(ts.tv_sec) * 1000LL + ts.tv_nsec / 1000000LL;
	writeCredential(path, cred);
	Log::debug(TAG, "Updated lastConnected for SSID: %s", ssid.c_str());
}

size_t WiFiCredentialStore::count() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	size_t n = 0;
	DIR* dir = opendir(DATA_DIR);
	if (!dir) return 0;
	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		std::string name = entry->d_name;
		if (name.size() >= 5 && name.substr(name.size() - 5) == ".json") {
			++n;
		}
	}
	closedir(dir);
	return n;
}

} // namespace flx::connectivity
