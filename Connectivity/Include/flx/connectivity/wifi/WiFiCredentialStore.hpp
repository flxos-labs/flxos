#pragma once

#include <esp_err.h>
#include <flx/core/Singleton.hpp>
#include <mutex>
#include <string>
#include <vector>

namespace flx::connectivity {

/// A single stored WiFi network credential with metadata.
struct WiFiCredential {
    std::string ssid;
    std::string password;
    bool autoConnect = true;    ///< Attempt connection on boot / auto-scan
    int priority = 0;           ///< Higher = preferred when multiple known nets visible
    int64_t lastConnectedMs = 0;///< Epoch-ms of last successful connection; used for recency sort
};

/**
 * @brief Multi-network WiFi credential store backed by FatFS (/data partition).
 *
 * Storage layout:
 *   /data/connectivity/wifi/<fnv1a_ssid_hex>.json
 *
 * The SSID is hashed with 6-hex FNV-1a so the filename is always filesystem-safe,
 * working around the Tactility TODO (raw SSID filename breaks on SSIDs with "/" etc.).
 *
 * All methods are thread-safe via an internal mutex.
 * loadAll() returns credentials sorted by: priority DESC, lastConnectedMs DESC.
 */
class WiFiCredentialStore : public flx::Singleton<WiFiCredentialStore> {
    friend class flx::Singleton<WiFiCredentialStore>;

public:

    /// Save (or overwrite) credentials for the given SSID.
    esp_err_t save(const WiFiCredential& cred);

    /// Load credentials for a specific SSID. Returns false if not found.
    bool load(const std::string& ssid, WiFiCredential& out) const;

    /// Remove stored credentials for an SSID. Returns ESP_ERR_NOT_FOUND if absent.
    esp_err_t remove(const std::string& ssid);

    /// Returns true if credentials exist for the given SSID.
    bool contains(const std::string& ssid) const;

    /// Returns all stored credentials, sorted by priority DESC then lastConnectedMs DESC.
    std::vector<WiFiCredential> loadAll() const;

    /// Update the lastConnectedMs timestamp for the given SSID to now.
    void updateLastConnected(const std::string& ssid);

    /// Total number of stored networks.
    size_t count() const;

private:

    WiFiCredentialStore() = default;
    ~WiFiCredentialStore() = default;

    static constexpr const char* DATA_DIR = "/data/connectivity/wifi";

    /// 6-hex-char FNV-1a hash of the SSID — always filesystem-safe.
    static std::string ssidToFilename(const std::string& ssid);

    /// Full file path for a given SSID.
    static std::string filePath(const std::string& ssid);

    /// Low-level JSON read/write helpers
    static bool readCredential(const std::string& path, WiFiCredential& out);
    static bool writeCredential(const std::string& path, const WiFiCredential& cred);

    mutable std::mutex m_mutex;
};

} // namespace flx::connectivity
