#include "flx/connectivity/wifi/WiFiProvisioning.hpp"
#include "flx/connectivity/wifi/WiFiCredentialStore.hpp"
#include <Config.hpp>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <flx/core/Logger.hpp>
#include <flx/core/PathUtils.hpp>
#include <flx/core/Value.hpp>
#include <sstream>
#include <sys/stat.h>

static constexpr const char* TAG = "WiFiProvisioning";

namespace flx::connectivity {

bool WiFiProvisioning::importFromBootMedia() {
	Log::info(TAG, "Checking for boot-media WiFi provisioning files...");
	bool imported = false;
	if (importFromDirectory("/data/provisioning/wifi")) {
		imported = true;
	}
#if FLXOS_SD_CARD_ENABLED
	if (importFromDirectory("/sdcard/provisioning/wifi")) {
		imported = true;
	}
#endif
	return imported;
}

bool WiFiProvisioning::importFromDirectory(const char* dir) {
	DIR* d = opendir(dir);
	if (!d) {
		// Directory doesn't exist — no provisioning files
		return false;
	}

	bool any_imported = false;
	struct dirent* entry;
	while ((entry = readdir(d)) != nullptr) {
		if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) {
			continue;
		}
		std::string filename = entry->d_name;
		if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".json") {
			continue;
		}
		std::string path = std::string(dir) + "/" + filename;
		if (importFile(path)) {
			Log::info(TAG, "Successfully imported provisioning file: %s", path.c_str());
			any_imported = true;
		}
	}
	closedir(d);
	return any_imported;
}

bool WiFiProvisioning::importFile(const std::string& path) {
	FILE* f = fopen(path.c_str(), "r");
	if (!f) {
		return false;
	}

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (len <= 0 || len > 4096) {
		fclose(f);
		return false;
	}

	std::string content(static_cast<size_t>(len), '\0');
	size_t read_bytes = fread(&content[0], 1, static_cast<size_t>(len), f);
	fclose(f);
	content.resize(read_bytes);

	Log::info(TAG, "Content read (%d bytes, ftell size %ld):", (int)read_bytes, len);
	for (size_t i = 0; i < read_bytes; i++) {
		unsigned char c = content[i];
		if (c >= 32 && c < 127) {
			putchar(c);
		} else {
			printf("\\x%02X", c);
		}
	}
	printf("\n");

	auto doc = flx::core::FlxValueDocument::parseJson(std::move(content));
	if (!doc) {
		Log::error(TAG, "Failed to parse provisioning JSON: %s", path.c_str());
		return false;
	}

	auto root = doc->root();
	if (!root.isMap()) {
		return false;
	}

	std::string ssid = root.child("ssid").asString();
	if (ssid.empty()) {
		Log::warn(TAG, "Provisioning file %s missing 'ssid'", path.c_str());
		return false;
	}

	// Skip if already in the store
	if (WiFiCredentialStore::getInstance().contains(ssid)) {
		Log::info(TAG, "SSID '%s' already exists in credential store, skipping provisioning", ssid.c_str());
		bool autoRemove = root.child("autoRemove").asBool(false);
		if (autoRemove) {
			::remove(path.c_str());
		}
		return false;
	}

	WiFiCredential cred;
	cred.ssid = ssid;
	cred.password = root.child("password").asString();
	cred.autoConnect = root.child("autoConnect").asBool(true);
	cred.priority = static_cast<int>(root.child("priority").asInt64(0));

	esp_err_t err = WiFiCredentialStore::getInstance().save(cred);
	if (err == ESP_OK) {
		Log::info(TAG, "Provisioned WiFi network '%s' from %s", ssid.c_str(), path.c_str());
		bool autoRemove = root.child("autoRemove").asBool(false);
		if (autoRemove) {
			if (::remove(path.c_str()) != 0) {
				Log::warn(TAG, "Failed to remove provisioning file: %s", path.c_str());
			} else {
				Log::info(TAG, "Removed provisioning file: %s", path.c_str());
			}
		}
		return true;
	} else {
		Log::error(TAG, "Failed to save provisioned credential for SSID: %s (err %d)", ssid.c_str(), err);
		return false;
	}
}

} // namespace flx::connectivity
