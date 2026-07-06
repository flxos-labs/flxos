#pragma once

#include <string>

namespace flx::connectivity {

class WiFiProvisioning {
public:

	/// Scan /data/provisioning/wifi/ and /sdcard/provisioning/wifi/ for .json credential files.
	/// Import any that aren't already in the credential store.
	/// If "autoRemove" is set in the file, delete it after import.
	/// Returns true if at least one network was newly imported.
	static bool importFromBootMedia();

private:

	static bool importFromDirectory(const char* dir);
	static bool importFile(const std::string& path);
};

} // namespace flx::connectivity
