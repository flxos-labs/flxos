#pragma once

#include "Bundle.hpp"
#include "Intent.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace flx::apps {

// Forward declarations
struct AppManifest;

/**
 * @brief Unique identifier for a specific app launch instance
 *
 * Used to track app launches and deliver results back to the launcher.
 * Value 0 is reserved as "invalid / no launch ID".
 */
using LaunchId = uint32_t;
constexpr LaunchId LAUNCH_ID_INVALID = 0;

/**
 * @brief Standard result codes for inter-app communication
 */
enum class ResultCode : int {
	Cancelled = 0, // User cancelled or back-pressed
	Ok = 1, // Operation completed successfully
	Error = -1, // Operation failed
};

/**
 * @brief Callback type for receiving results from child apps
 */
using ResultCallback = std::function<void(ResultCode resultCode, const Bundle& data)>;

/**
 * @brief Runtime context for a running app instance
 *
 * Each app launch gets its own AppContext, providing:
 * - Access to the app's manifest
 * - The intent that launched the app
 * - APIs to set results and launch child apps
 *
 * - APIs to set results and launch child apps
 */
class AppContext {
public:

	AppContext(const AppManifest* manifest, const Intent& intent, LaunchId launchId)
		: m_manifest(manifest), m_intent(intent), m_launchId(launchId) {}

	// === Manifest access ===

	const AppManifest* getManifest() const { return m_manifest; }
	const std::string& getAppId() const;

	// === Intent access ===

	const Intent& getIntent() const { return m_intent; }
	const Bundle& getExtras() const { return m_intent.extras; }
	const std::string& getData() const { return m_intent.data; }

	// === Launch tracking ===

	LaunchId getLaunchId() const { return m_launchId; }

	// === Result setting ===

	/** Set the result to deliver back to the parent app when this app finishes */
	void setResult(ResultCode code, const Bundle& data = {}) {
		m_resultCode = code;
		m_resultData = data;
		m_hasResult = true;
	}

	bool hasResult() const { return m_hasResult; }
	ResultCode getResultCode() const { return m_resultCode; }
	const Bundle& getResultData() const { return m_resultData; }

	// === Result callback (set by AppManager, used internally) ===

	void setResultCallback(ResultCallback cb) { m_resultCallback = std::move(cb); }
	const ResultCallback& getResultCallback() const { return m_resultCallback; }

private:

	const AppManifest* m_manifest;
	Intent m_intent;
	LaunchId m_launchId;

	bool m_hasResult = false;
	ResultCode m_resultCode = ResultCode::Cancelled;
	Bundle m_resultData;
	ResultCallback m_resultCallback;
};

} // namespace flx::apps
