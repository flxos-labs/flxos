#pragma once

#include <esp_http_server.h>
#include <flx/core/Observable.hpp>
#include <flx/services/IService.hpp>
#include <memory>
#include <string>

namespace flx::system::services {

class WebServerService : public flx::services::IService {
public:

	static WebServerService& getInstance();

	// ──── IService manifest ────
	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;
	flx::services::HealthStatus onHealthCheck() override;

	// ──── Settings Observables ────
	static flx::Observable<int32_t> webserverEnabled;
	static flx::Observable<int32_t> webserverPort;

	// ──── Server State ────
	bool isServerRunning() const { return m_server != nullptr; }

private:

	WebServerService();
	~WebServerService() = default;

	WebServerService(const WebServerService&) = delete;
	WebServerService& operator=(const WebServerService&) = delete;

	esp_err_t startServer();
	void stopServer();

	// ──── Event Registration ────
	void registerEventHandlers();
	void unregisterEventHandlers();

	// ──── Basic HTTP Authentication ────
	static bool authenticateRequest(httpd_req_t* req);

	// ──── REST API Route Handlers ────
	static esp_err_t getSystemHandler(httpd_req_t* req);
	static esp_err_t getWifiHandler(httpd_req_t* req);
	static esp_err_t postWifiScanHandler(httpd_req_t* req);
	static esp_err_t postWifiConnectHandler(httpd_req_t* req);
	static esp_err_t postWifiDisconnectHandler(httpd_req_t* req);
	static esp_err_t getWifiSavedHandler(httpd_req_t* req);
	static esp_err_t deleteWifiSavedHandler(httpd_req_t* req);

	static esp_err_t getHotspotHandler(httpd_req_t* req);
	static esp_err_t postHotspotStartHandler(httpd_req_t* req);
	static esp_err_t postHotspotStopHandler(httpd_req_t* req);

	static esp_err_t getAppsHandler(httpd_req_t* req);
	static esp_err_t postRebootHandler(httpd_req_t* req);

	// ──── File System Handlers ────
	static esp_err_t getFsListHandler(httpd_req_t* req);
	static esp_err_t getFsDownloadHandler(httpd_req_t* req);
	static esp_err_t postFsUploadHandler(httpd_req_t* req);
	static esp_err_t postFsDeleteHandler(httpd_req_t* req);

	// ──── Static Dashboard Handler ────
	static esp_err_t getDashboardHandler(httpd_req_t* req);

	// ──── Helper Methods ────
	static std::string getQueryParameter(httpd_req_t* req, const std::string& paramName);
	static std::string decodeBase64(const std::string& input);

	httpd_handle_t m_server = nullptr;
	bool m_events_registered = false;
	size_t m_hotspot_enabled_sub_id = 0;
};

} // namespace flx::system::services
