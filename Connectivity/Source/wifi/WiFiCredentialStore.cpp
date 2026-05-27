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

static constexpr std::string_view TAG = "WiFiCredentialStore";

// JSON field names
static constexpr const char* KEY_SSID = "ssid";
static constexpr const char* KEY_PASSWORD = "password";
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

	FILE* f = fopen(path.c_str(), "w");
	if (!f) {
		Log::error(TAG, "Failed to open %s for writing: %s", path.c_str(), strerror(errno));
		return false;
	}

	// Build compact JSON manually – keeps it on the stack/heap without heavy libraries
	fprintf(f,
		"{\"ssid\":\"%s\",\"password\":\"%s\",\"autoConnect\":%s,\"priority\":%d,\"lastConnectedMs\":%" PRId64 "}",
		escapeJson(cred.ssid).c_str(),
		escapeJson(cred.password).c_str(),
		cred.autoConnect ? "true" : "false",
		cred.priority,
		cred.lastConnectedMs);

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
	fread(&content[0], 1, static_cast<size_t>(len), f);
	fclose(f);

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
	out.password = root.child(KEY_PASSWORD).asString();
	out.autoConnect = root.child(KEY_AUTO_CONNECT).asBool(true);
	out.priority = static_cast<int>(root.child(KEY_PRIORITY).asInt64(0));
	out.lastConnectedMs = root.child(KEY_LAST_CONNECTED_MS).asInt64(0);

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
