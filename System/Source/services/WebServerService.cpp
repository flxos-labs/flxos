#include <algorithm>
#include <dirent.h>
#include <esp_chip_info.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppRegistry.hpp>
#include <flx/connectivity/ConnectivityManager.hpp>
#include <flx/core/Logger.hpp>
#include <flx/core/PathUtils.hpp>
#include <flx/core/Value.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/system/services/WebServerService.hpp>
#include <iomanip>
#include <lwip/sockets.h>
#include <mbedtls/base64.h>
#include <mdns.h>
#include <mutex>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

static constexpr const char* TAG = "WebServerService";

namespace flx::system::services {

using namespace flx::connectivity;

// ──── Static Member Initialization ────
flx::Observable<int32_t> WebServerService::webserverEnabled {0};
flx::Observable<int32_t> WebServerService::webserverPort {80};

// Static DNS Captive Portal task handle and socket
static TaskHandle_t s_dns_task_handle = nullptr;
static int s_dns_sock = -1;

const flx::services::ServiceManifest WebServerService::serviceManifest = {
	.serviceId = "com.flxos.webserver",
	.serviceName = "Web Server",
	.version = "1.0.0",
	.dependencies = {"com.flxos.settings"},
	.priority = 110,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::WiFi,
	.description = "Management Web Dashboard and REST API",
};

WebServerService& WebServerService::getInstance() {
	static WebServerService instance;
	return instance;
}

WebServerService::WebServerService() = default;

// ──── DNS Captive Portal Server Task ────
static void dns_captive_portal_task(void* pvParameters) {
	s_dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (s_dns_sock < 0) {
		Log::error(TAG, "Failed to create DNS socket");
		s_dns_task_handle = nullptr;
		vTaskDelete(nullptr);
		return;
	}

	struct sockaddr_in server_addr = {};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(53);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(s_dns_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		Log::error(TAG, "Failed to bind DNS socket to port 53");
		close(s_dns_sock);
		s_dns_sock = -1;
		s_dns_task_handle = nullptr;
		vTaskDelete(nullptr);
		return;
	}

	Log::info(TAG, "DNS Captive Portal redirect server listening on port 53...");

	uint8_t rx_buffer[512];
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	while (true) {
		int len = recvfrom(s_dns_sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr*)&client_addr, &client_addr_len);
		if (len < 12) {
			if (len < 0) break;
			continue;
		}

		uint16_t flags = (rx_buffer[2] << 8) | rx_buffer[3];
		uint16_t questions = (rx_buffer[4] << 8) | rx_buffer[5];
		if ((flags & 0x8000) == 0 && questions == 1) {
			uint8_t tx_buffer[512];
			memcpy(tx_buffer, rx_buffer, len);

			// Response flags: Response, Authoritative, Recursion Desired
			tx_buffer[2] = 0x84;
			tx_buffer[3] = 0x00;

			// Answers count = 1
			tx_buffer[6] = 0x00;
			tx_buffer[7] = 0x01;

			int offset = len;
			// Answer RR points back to query name
			tx_buffer[offset++] = 0xc0;
			tx_buffer[offset++] = 0x0c;

			// Type A (IPv4)
			tx_buffer[offset++] = 0x00;
			tx_buffer[offset++] = 0x01;

			// Class IN
			tx_buffer[offset++] = 0x00;
			tx_buffer[offset++] = 0x01;

			// TTL (10 seconds)
			tx_buffer[offset++] = 0x00;
			tx_buffer[offset++] = 0x00;
			tx_buffer[offset++] = 0x00;
			tx_buffer[offset++] = 0x0a;

			// Data Length = 4
			tx_buffer[offset++] = 0x00;
			tx_buffer[offset++] = 0x04;

			// Direct to SoftAP Gateway: 192.168.4.1
			tx_buffer[offset++] = 192;
			tx_buffer[offset++] = 168;
			tx_buffer[offset++] = 4;
			tx_buffer[offset++] = 1;

			sendto(s_dns_sock, tx_buffer, offset, 0, (struct sockaddr*)&client_addr, client_addr_len);
		}
	}

	close(s_dns_sock);
	s_dns_sock = -1;
	s_dns_task_handle = nullptr;
	vTaskDelete(nullptr);
}

static void start_dns_server() {
	if (s_dns_task_handle == nullptr) {
		xTaskCreate(dns_captive_portal_task, "dns_captive", 3072, nullptr, 4, &s_dns_task_handle);
	}
}

static void stop_dns_server() {
	if (s_dns_sock >= 0) {
		close(s_dns_sock);
		s_dns_sock = -1;
	}
	if (s_dns_task_handle != nullptr) {
		// Socket closure breaks recvfrom block, task self-terminates.
		s_dns_task_handle = nullptr;
	}
}

// ──── Captive Portal HTTP 404 Redirect Handler ────
static esp_err_t captive_portal_redirect_handler(httpd_req_t* req, httpd_err_code_t error) {
	char host[64] = {};
	if (httpd_req_get_hdr_value_str(req, "Host", host, sizeof(host)) == ESP_OK) {
		std::string host_str(host);
		// If requesting something other than the portal domain/IP, perform 302 Found redirect
		if (host_str.find("192.168.4.1") == std::string::npos && host_str.find("flxos.local") == std::string::npos) {
			Log::info(TAG, "Redirecting client query to portal home (host: %s)", host);
			httpd_resp_set_status(req, "302 Found");
			httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
			httpd_resp_send(req, nullptr, 0);
			return ESP_OK;
		}
	}

	httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Resource not found");
	return ESP_FAIL;
}

// ──── IService Lifecycle ────
bool WebServerService::onStart() {
	Log::info(TAG, "Initializing WebServer configuration...");

	// Register settings with settings manager
	flx::system::SettingsManager::getInstance().registerSetting("webserver.enabled", webserverEnabled);
	flx::system::SettingsManager::getInstance().registerSetting("webserver.port", webserverPort);

	// Dynamically handle switches/toggles
	webserverEnabled.subscribe([this](int32_t val) {
		if (val) {
			startServer();
		} else {
			stopServer();
		}
	});

	webserverPort.subscribe([this](int32_t val) {
		if (isServerRunning()) {
			stopServer();
			startServer();
		}
	});

	if (webserverEnabled.get()) {
		startServer();
	}

	registerEventHandlers();
	return true;
}

void WebServerService::onStop() {
	stopServer();
	unregisterEventHandlers();
}

flx::services::HealthStatus WebServerService::onHealthCheck() {
	if (webserverEnabled.get() && !isServerRunning()) {
		return flx::services::HealthStatus::Unhealthy;
	}
	return flx::services::HealthStatus::Healthy;
}

void WebServerService::registerEventHandlers() {
	if (m_events_registered) return;

	// Keep captive portal DNS active whenever hotspot starts
	// (or whenever WiFi state indicates no network is saved/connected)
	m_events_registered = true;
}

void WebServerService::unregisterEventHandlers() {
	m_events_registered = false;
}

// ──── Start/Stop HTTP server ────
esp_err_t WebServerService::startServer() {
	if (m_server) {
		return ESP_OK;
	}

	int port = webserverPort.get();
	Log::info(TAG, "Spinning up HTTP Daemon on port %d...", port);

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = port;
	config.ctrl_port = port + 1;
	config.max_uri_handlers = 24;
	config.stack_size = 8192;

	esp_err_t err = httpd_start(&m_server, &config);
	if (err != ESP_OK) {
		Log::error(TAG, "Daemon boot failed: %s", esp_err_to_name(err));
		return err;
	}

	// Register route-mapping helper
	auto reg = [this](const char* uri, httpd_method_t method, esp_err_t (*h)(httpd_req_t*)) {
		httpd_uri_t route = {};
		route.uri = uri;
		route.method = method;
		route.handler = h;
		route.user_ctx = nullptr;
		httpd_register_uri_handler(m_server, &route);
	};

	// Routes
	reg("/api/system", HTTP_GET, getSystemHandler);
	reg("/api/wifi", HTTP_GET, getWifiHandler);
	reg("/api/wifi/scan", HTTP_POST, postWifiScanHandler);
	reg("/api/wifi/connect", HTTP_POST, postWifiConnectHandler);
	reg("/api/wifi/disconnect", HTTP_POST, postWifiDisconnectHandler);
	reg("/api/wifi/saved", HTTP_GET, getWifiSavedHandler);
	reg("/api/wifi/saved", HTTP_DELETE, deleteWifiSavedHandler);
	reg("/api/hotspot", HTTP_GET, getHotspotHandler);
	reg("/api/hotspot/start", HTTP_POST, postHotspotStartHandler);
	reg("/api/hotspot/stop", HTTP_POST, postHotspotStopHandler);
	reg("/api/apps", HTTP_GET, getAppsHandler);
	reg("/admin/reboot", HTTP_POST, postRebootHandler);
	reg("/fs/list", HTTP_GET, getFsListHandler);
	reg("/fs/download", HTTP_GET, getFsDownloadHandler);
	reg("/fs/upload", HTTP_POST, postFsUploadHandler);
	reg("/fs/delete", HTTP_POST, postFsDeleteHandler);
	reg("/", HTTP_GET, getDashboardHandler);
	reg("/index.html", HTTP_GET, getDashboardHandler);

	// Register redirect captive portal handler for 404 page requests
	httpd_register_err_handler(m_server, HTTPD_404_NOT_FOUND, captive_portal_redirect_handler);

	// Advertise mDNS
	mdns_init();
	mdns_hostname_set("flxos");
	mdns_instance_name_set("FlxOS Hub");
	mdns_service_add(nullptr, "_http", "_tcp", port, nullptr, 0);

	// If hotspot active, spin up captive portal DNS server
	if (flx::connectivity::ConnectivityManager::getInstance().isHotspotEnabled()) {
		start_dns_server();
	}

	return ESP_OK;
}

void WebServerService::stopServer() {
	if (m_server) {
		Log::info(TAG, "Tearing down HTTP Daemon...");
		httpd_stop(m_server);
		m_server = nullptr;

		mdns_service_remove("_http", "_tcp");
		mdns_free();

		stop_dns_server();
	}
}

// ──── Timing-safe comparison to prevent side-channel leaks ────
static bool timing_safe_compare(const std::string& a, const std::string& b) {
	if (a.length() != b.length()) return false;
	volatile uint8_t diff = 0;
	for (size_t i = 0; i < a.length(); ++i) {
		diff |= (a[i] ^ b[i]);
	}
	return diff == 0;
}

// ──── Basic HTTP Authentication ────
bool WebServerService::authenticateRequest(httpd_req_t* req) {
	char auth_hdr[128] = {};
	if (httpd_req_get_hdr_value_str(req, "Authorization", auth_hdr, sizeof(auth_hdr)) == ESP_OK) {
		std::string auth_str(auth_hdr);
		if (auth_str.find("Basic ") == 0) {
			std::string b64 = auth_str.substr(6);
			std::string creds = decodeBase64(b64);
			size_t colon = creds.find(':');
			if (colon != std::string::npos) {
				std::string username = creds.substr(0, colon);
				std::string password = creds.substr(colon + 1);

				// Fallback to defaults or settings
				std::string expected_user = "admin";
				std::string expected_pass = "admin";

				if (timing_safe_compare(username, expected_user) && timing_safe_compare(password, expected_pass)) {
					return true;
				}
			}
		}
	}

	httpd_resp_set_status(req, "401 Unauthorized");
	httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"FlxOS System Dashboard\"");
	httpd_resp_send(req, "Authentication Required", 23);
	return false;
}

std::string WebServerService::decodeBase64(const std::string& input) {
	size_t out_len = 0;
	std::vector<uint8_t> out_buf(input.length() + 1);
	if (mbedtls_base64_decode(out_buf.data(), out_buf.size(), &out_len, reinterpret_cast<const unsigned char*>(input.data()), input.length()) == 0) {
		return std::string(reinterpret_cast<char*>(out_buf.data()), out_len);
	}
	return "";
}

// ──── Helper to pull query parameters from request ────
std::string WebServerService::getQueryParameter(httpd_req_t* req, const std::string& paramName) {
	size_t query_len = httpd_req_get_url_query_len(req) + 1;
	if (query_len <= 1) return "";

	std::vector<char> query(query_len);
	if (httpd_req_get_url_query_str(req, query.data(), query_len) == ESP_OK) {
		char val[128] = {};
		if (httpd_query_key_value(query.data(), paramName.c_str(), val, sizeof(val)) == ESP_OK) {
			// URL Decode space and special chars
			std::string decoded;
			for (size_t i = 0; val[i] != '\0'; ++i) {
				if (val[i] == '+') {
					decoded += ' ';
				} else if (val[i] == '%' && val[i + 1] != '\0' && val[i + 2] != '\0') {
					char hex[3] = {val[i + 1], val[i + 2], '\0'};
					decoded += static_cast<char>(strtol(hex, nullptr, 16));
					i += 2;
				} else {
					decoded += val[i];
				}
			}
			return decoded;
		}
	}
	return "";
}

// JSON escaping
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

// ──── REST API Route Handlers ────
esp_err_t WebServerService::getSystemHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	uint32_t free_heap = esp_get_free_heap_size();
	uint32_t min_heap = esp_get_minimum_free_heap_size();
	int64_t uptime = esp_timer_get_time() / 1000000;

	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);

	std::string model = "ESP32";
	if (chip_info.model == CHIP_ESP32S3) model = "ESP32-S3";
	else if (chip_info.model == CHIP_ESP32C3)
		model = "ESP32-C3";
	else if (chip_info.model == CHIP_ESP32C6)
		model = "ESP32-C6";

	std::stringstream ss;
	ss << "{"
	   << "\"heap\":" << free_heap << ","
	   << "\"min_heap\":" << min_heap << ","
	   << "\"uptime\":" << uptime << ","
	   << "\"chip_model\":\"" << model << "\","
	   << "\"chip_cores\":" << static_cast<int>(chip_info.cores) << ","
	   << "\"chip_revision\":" << static_cast<int>(chip_info.revision)
	   << "}";

	std::string response = ss.str();
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, response.c_str(), response.length());
	return ESP_OK;
}

esp_err_t WebServerService::getWifiHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	bool connected = ConnectivityManager::getInstance().isWiFiConnected();
	std::string ssid = ConnectivityManager::getInstance().getWiFiSsidObservable().get();
	std::string ip = ConnectivityManager::getInstance().getWiFiIpObservable().get();

	int rssi = -100;
	int channel = 0;
	wifi_ap_record_t ap_info;
	if (connected && esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
		rssi = ap_info.rssi;
		channel = ap_info.primary;
	}

	std::stringstream ss;
	ss << "{"
	   << "\"connected\":" << (connected ? "true" : "false") << ","
	   << "\"ssid\":\"" << escapeJson(ssid) << "\","
	   << "\"ip\":\"" << ip << "\","
	   << "\"rssi\":" << rssi << ","
	   << "\"channel\":" << channel
	   << "}";

	std::string response = ss.str();
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, response.c_str(), response.length());
	return ESP_OK;
}

esp_err_t WebServerService::postWifiScanHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	SemaphoreHandle_t scan_sem = xSemaphoreCreateBinary();
	std::vector<wifi_ap_record_t> scan_results;

	esp_err_t err = ConnectivityManager::getInstance().scanWiFi([&](const std::vector<wifi_ap_record_t>& results) {
		scan_results = results;
		xSemaphoreGive(scan_sem);
	});

	if (err == ESP_OK) {
		if (xSemaphoreTake(scan_sem, pdMS_TO_TICKS(10000)) != pdTRUE) {
			err = ESP_FAIL;
		}
	}
	vSemaphoreDelete(scan_sem);

	if (err != ESP_OK) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "WiFi Scan failed");
		return ESP_FAIL;
	}

	std::stringstream ss;
	ss << "[";
	for (size_t i = 0; i < scan_results.size(); ++i) {
		if (i > 0) ss << ",";
		ss << "{"
		   << "\"ssid\":\"" << escapeJson(reinterpret_cast<char*>(scan_results[i].ssid)) << "\","
		   << "\"rssi\":" << static_cast<int>(scan_results[i].rssi) << ","
		   << "\"channel\":" << static_cast<int>(scan_results[i].primary) << ","
		   << "\"auth\":" << static_cast<int>(scan_results[i].authmode)
		   << "}";
	}
	ss << "]";

	std::string response = ss.str();
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, response.c_str(), response.length());
	return ESP_OK;
}

esp_err_t WebServerService::postWifiConnectHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	char buf[256] = {};
	int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
	if (received <= 0) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Body required");
		return ESP_FAIL;
	}

	auto doc = flx::core::FlxValueDocument::parseJson(buf);
	if (!doc || !doc->root().isMap()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
		return ESP_FAIL;
	}

	std::string ssid = doc->root().child("ssid").asString();
	std::string password = doc->root().child("password").asString();

	if (ssid.empty()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
		return ESP_FAIL;
	}

	esp_err_t err = ConnectivityManager::getInstance().connectWiFi(ssid.c_str(), password.c_str(), true);
	if (err == ESP_OK) {
		httpd_resp_send(req, "{\"status\":\"connecting\"}", 23);
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Connect failed");
	}
	return ESP_OK;
}

esp_err_t WebServerService::postWifiDisconnectHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	esp_err_t err = ConnectivityManager::getInstance().disconnectWiFi();
	if (err == ESP_OK) {
		httpd_resp_send(req, "{\"status\":\"ok\"}", 15);
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Disconnect failed");
	}
	return ESP_OK;
}

esp_err_t WebServerService::getWifiSavedHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	auto saved = ConnectivityManager::getInstance().getSavedNetworks();

	std::stringstream ss;
	ss << "[";
	for (size_t i = 0; i < saved.size(); ++i) {
		if (i > 0) ss << ",";
		ss << "{"
		   << "\"ssid\":\"" << escapeJson(saved[i].ssid) << "\","
		   << "\"priority\":" << saved[i].priority << ","
		   << "\"autoConnect\":" << (saved[i].autoConnect ? "true" : "false")
		   << "}";
	}
	ss << "]";

	std::string response = ss.str();
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, response.c_str(), response.length());
	return ESP_OK;
}

esp_err_t WebServerService::deleteWifiSavedHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	std::string ssid = getQueryParameter(req, "ssid");
	if (ssid.empty()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
		return ESP_FAIL;
	}

	esp_err_t err = ConnectivityManager::getInstance().removeWiFiNetwork(ssid);
	if (err == ESP_OK) {
		httpd_resp_send(req, "{\"status\":\"ok\"}", 15);
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to delete");
	}
	return ESP_OK;
}

esp_err_t WebServerService::getHotspotHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	bool enabled = ConnectivityManager::getInstance().isHotspotEnabled();
	std::string ssid = ConnectivityManager::getInstance().getHotspotSsidObservable().get();
	std::string password = ConnectivityManager::getInstance().getHotspotPasswordObservable().get();
	int channel = ConnectivityManager::getInstance().getHotspotChannelObservable().get();
	int max_conn = ConnectivityManager::getInstance().getHotspotMaxConnObservable().get();
	bool hidden = ConnectivityManager::getInstance().getHotspotHiddenObservable().get();
	bool nat = ConnectivityManager::getInstance().isHotspotNatEnabled();
	size_t clients = ConnectivityManager::getInstance().getHotspotClientsList().size();

	std::stringstream ss;
	ss << "{"
	   << "\"enabled\":" << (enabled ? "true" : "false") << ","
	   << "\"ssid\":\"" << escapeJson(ssid) << "\","
	   << "\"password\":\"" << escapeJson(password) << "\","
	   << "\"channel\":" << channel << ","
	   << "\"max_connections\":" << max_conn << ","
	   << "\"hidden\":" << (hidden ? "true" : "false") << ","
	   << "\"nat_enabled\":" << (nat ? "true" : "false") << ","
	   << "\"clients\":" << clients
	   << "}";

	std::string response = ss.str();
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, response.c_str(), response.length());
	return ESP_OK;
}

esp_err_t WebServerService::postHotspotStartHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	char buf[256] = {};
	int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
	if (received <= 0) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Body required");
		return ESP_FAIL;
	}

	auto doc = flx::core::FlxValueDocument::parseJson(buf);
	if (!doc || !doc->root().isMap()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
		return ESP_FAIL;
	}

	std::string ssid = doc->root().child("ssid").asString();
	std::string password = doc->root().child("password").asString();
	int channel = static_cast<int>(doc->root().child("channel").asInt64(1));
	int max_conn = static_cast<int>(doc->root().child("max_connections").asInt64(4));
	bool hidden = doc->root().child("hidden").asBool(false);
	bool nat = doc->root().child("nat_enabled").asBool(true);

	if (ssid.empty()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
		return ESP_FAIL;
	}

	ConnectivityManager::setHotspotNatEnabled(nat);
	esp_err_t err = ConnectivityManager::getInstance().startHotspot(
		ssid.c_str(),
		password.c_str(),
		channel,
		max_conn,
		hidden);

	if (err == ESP_OK) {
		start_dns_server();
		httpd_resp_send(req, "{\"status\":\"ok\"}", 15);
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to start Hotspot");
	}
	return ESP_OK;
}

esp_err_t WebServerService::postHotspotStopHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	esp_err_t err = ConnectivityManager::getInstance().stopHotspot();
	if (err == ESP_OK) {
		stop_dns_server();
		httpd_resp_send(req, "{\"status\":\"ok\"}", 15);
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to stop Hotspot");
	}
	return ESP_OK;
}

esp_err_t WebServerService::getAppsHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	auto apps = flx::apps::AppRegistry::getInstance().getAll();

	std::stringstream ss;
	ss << "[";
	for (size_t i = 0; i < apps.size(); ++i) {
		if (i > 0) ss << ",";
		ss << "{"
		   << "\"id\":\"" << escapeJson(apps[i].appId) << "\","
		   << "\"name\":\"" << escapeJson(apps[i].appName) << "\""
		   << "}";
	}
	ss << "]";

	std::string response = ss.str();
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, response.c_str(), response.length());
	return ESP_OK;
}

esp_err_t WebServerService::postRebootHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	httpd_resp_send(req, "{\"status\":\"rebooting\"}", 22);
	vTaskDelay(pdMS_TO_TICKS(500));
	esp_restart();
	return ESP_OK;
}

// ──── File System Handlers ────
esp_err_t WebServerService::getFsListHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	std::string path = getQueryParameter(req, "path");
	if (path.empty()) {
		path = "/data";
	}

	DIR* d = opendir(path.c_str());
	if (!d) {
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Directory not found");
		return ESP_FAIL;
	}

	std::stringstream json;
	json << "[";
	struct dirent* entry;
	bool first = true;
	while ((entry = readdir(d)) != nullptr) {
		std::string name = entry->d_name;
		if (name == "." || name == "..") {
			continue;
		}
		if (!first) json << ",";
		first = false;

		std::string fullpath = path + "/" + name;
		struct stat st = {};
		stat(fullpath.c_str(), &st);

		bool is_dir = S_ISDIR(st.st_mode);
		json << "{"
			 << "\"name\":\"" << escapeJson(name) << "\","
			 << "\"is_dir\":" << (is_dir ? "true" : "false") << ","
			 << "\"size\":" << (is_dir ? 0 : st.st_size)
			 << "}";
	}
	closedir(d);
	json << "]";

	std::string res = json.str();
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, res.c_str(), res.length());
	return ESP_OK;
}

esp_err_t WebServerService::getFsDownloadHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	std::string path = getQueryParameter(req, "path");
	if (path.empty()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Path required");
		return ESP_FAIL;
	}

	FILE* f = fopen(path.c_str(), "r");
	if (!f) {
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
		return ESP_FAIL;
	}

	httpd_resp_set_type(req, "application/octet-stream");
	size_t last_slash = path.find_last_of('/');
	std::string filename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
	std::string disposition = "attachment; filename=\"" + filename + "\"";
	httpd_resp_set_hdr(req, "Content-Disposition", disposition.c_str());

	char buf[1024];
	size_t read_bytes = 0;
	while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
		if (httpd_resp_send_chunk(req, buf, read_bytes) != ESP_OK) {
			fclose(f);
			httpd_resp_sendstr_chunk(req, nullptr);
			return ESP_FAIL;
		}
	}
	fclose(f);
	httpd_resp_send_chunk(req, nullptr, 0);
	return ESP_OK;
}

esp_err_t WebServerService::postFsUploadHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	std::string path = getQueryParameter(req, "path");
	if (path.empty()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Path required");
		return ESP_FAIL;
	}

	FILE* f = fopen(path.c_str(), "w");
	if (!f) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file");
		return ESP_FAIL;
	}

	char buf[1024];
	int received = 0;
	int remaining = req->content_len;

	while (remaining > 0) {
		int to_recv = std::min(remaining, static_cast<int>(sizeof(buf)));
		received = httpd_req_recv(req, buf, to_recv);
		if (received <= 0) {
			if (received == HTTPD_SOCK_ERR_TIMEOUT) continue;
			fclose(f);
			::remove(path.c_str());
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload failure");
			return ESP_FAIL;
		}
		fwrite(buf, 1, received, f);
		remaining -= received;
	}

	fclose(f);
	httpd_resp_send(req, "{\"status\":\"ok\"}", 15);
	return ESP_OK;
}

esp_err_t WebServerService::postFsDeleteHandler(httpd_req_t* req) {
	if (!authenticateRequest(req)) return ESP_OK;

	std::string path = getQueryParameter(req, "path");
	if (path.empty()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Path required");
		return ESP_FAIL;
	}

	if (::remove(path.c_str()) == 0) {
		httpd_resp_send(req, "{\"status\":\"ok\"}", 15);
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to delete");
	}
	return ESP_OK;
}

// ──── Static Dashboard HTML/CSS/JS (Glow Dark Mode / Glassmorphism) ────
esp_err_t WebServerService::getDashboardHandler(httpd_req_t* req) {
	const char* html = R"html(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FlxOS System Dashboard</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-dark: #090714;
            --panel-bg: rgba(20, 16, 38, 0.45);
            --border-glass: rgba(255, 255, 255, 0.08);
            --accent-cyan: #06b6d4;
            --accent-indigo: #6366f1;
            --accent-purple: #a855f7;
            --text-main: #f3f4f6;
            --text-dim: #9ca3af;
            --gradient: linear-gradient(135deg, var(--accent-cyan), var(--accent-indigo), var(--accent-purple));
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            user-select: none;
        }

        body {
            font-family: 'Outfit', sans-serif;
            background: var(--bg-dark);
            color: var(--text-main);
            min-height: 100vh;
            display: flex;
            overflow-x: hidden;
            background-image: radial-gradient(circle at 10% 20%, rgba(99, 102, 241, 0.15) 0%, transparent 40%),
                              radial-gradient(circle at 90% 80%, rgba(168, 85, 247, 0.15) 0%, transparent 40%);
        }

        /* Sidebar Navigation */
        .sidebar {
            width: 260px;
            background: rgba(10, 8, 20, 0.7);
            border-right: 1px solid var(--border-glass);
            backdrop-filter: blur(20px);
            display: flex;
            flex-direction: column;
            padding: 2rem 1.5rem;
            position: fixed;
            height: 100vh;
            z-index: 10;
        }

        .logo {
            font-weight: 800;
            font-size: 1.8rem;
            letter-spacing: 1px;
            background: var(--gradient);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 3rem;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .nav-list {
            list-style: none;
            display: flex;
            flex-direction: column;
            gap: 1rem;
        }

        .nav-item {
            padding: 1rem 1.25rem;
            border-radius: 12px;
            cursor: pointer;
            color: var(--text-dim);
            font-weight: 600;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            display: flex;
            align-items: center;
            gap: 1rem;
            border: 1px solid transparent;
        }

        .nav-item:hover, .nav-item.active {
            color: var(--text-main);
            background: rgba(255, 255, 255, 0.05);
            border-color: var(--border-glass);
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.2);
        }

        .nav-item.active {
            background: linear-gradient(135deg, rgba(6, 182, 212, 0.15), rgba(99, 102, 241, 0.15));
            border-color: rgba(99, 102, 241, 0.3);
            position: relative;
        }

        .nav-item.active::before {
            content: '';
            position: absolute;
            left: 0;
            top: 25%;
            height: 50%;
            width: 4px;
            background: var(--accent-cyan);
            border-radius: 0 4px 4px 0;
        }

        /* Main Content Container */
        .content-area {
            margin-left: 260px;
            flex-grow: 1;
            padding: 3rem;
            max-width: 1200px;
        }

        header {
            margin-bottom: 3rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        h1 {
            font-size: 2.2rem;
            font-weight: 800;
        }

        .status-badge {
            background: rgba(16, 185, 129, 0.15);
            border: 1px solid rgba(16, 185, 129, 0.3);
            color: #10b981;
            padding: 0.5rem 1rem;
            border-radius: 20px;
            font-weight: 600;
            font-size: 0.9rem;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        /* Dashboard Grid */
        .tab-content {
            display: none;
            animation: fadeIn 0.4s ease-out;
        }

        .tab-content.active {
            display: block;
        }

        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(15px); }
            to { opacity: 1; transform: translateY(0); }
        }

        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
            gap: 2rem;
            margin-bottom: 2rem;
        }

        /* Cards & Glassmorphism Panels */
        .card {
            background: var(--panel-bg);
            border: 1px solid var(--border-glass);
            border-radius: 20px;
            padding: 2rem;
            backdrop-filter: blur(15px);
            transition: transform 0.3s ease;
        }

        .card:hover {
            transform: translateY(-5px);
        }

        .card-title {
            font-size: 1.1rem;
            color: var(--text-dim);
            font-weight: 600;
            margin-bottom: 1.5rem;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .metric {
            font-size: 2.5rem;
            font-weight: 800;
            background: var(--gradient);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .metric-sub {
            font-size: 0.9rem;
            color: var(--text-dim);
            margin-top: 0.5rem;
        }

        /* Forms, Buttons, Inputs */
        input[type="text"], input[type="password"], select {
            width: 100%;
            padding: 1rem;
            background: rgba(255, 255, 255, 0.04);
            border: 1px solid var(--border-glass);
            border-radius: 12px;
            color: var(--text-main);
            outline: none;
            margin-bottom: 1.25rem;
            font-family: inherit;
            transition: all 0.3s;
        }

        input:focus {
            border-color: var(--accent-cyan);
            background: rgba(255, 255, 255, 0.08);
            box-shadow: 0 0 15px rgba(6, 182, 212, 0.25);
        }

        .btn {
            display: inline-block;
            width: 100%;
            padding: 1rem;
            background: var(--gradient);
            color: var(--bg-dark);
            font-weight: 700;
            border-radius: 12px;
            border: none;
            cursor: pointer;
            transition: all 0.3s;
            text-align: center;
        }

        .btn:hover {
            box-shadow: 0 0 25px rgba(99, 102, 241, 0.45);
            transform: scale(1.02);
        }

        /* Table Design */
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 1rem;
        }

        th, td {
            padding: 1rem;
            text-align: left;
            border-bottom: 1px solid var(--border-glass);
        }

        th {
            color: var(--text-dim);
            font-weight: 600;
        }

        /* File Explorer */
        .file-list {
            max-height: 400px;
            overflow-y: auto;
        }

        .file-row {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 0.85rem 1rem;
            border-bottom: 1px solid var(--border-glass);
            transition: background 0.2s;
            border-radius: 8px;
        }

        .file-row:hover {
            background: rgba(255, 255, 255, 0.03);
        }

        .file-info {
            display: flex;
            align-items: center;
            gap: 1rem;
        }

        .file-icon {
            font-size: 1.2rem;
        }

        .file-actions {
            display: flex;
            gap: 1rem;
        }

        .action-link {
            color: var(--accent-cyan);
            text-decoration: none;
            cursor: pointer;
            font-weight: 600;
        }

        .action-link.delete {
            color: #ef4444;
        }
    </style>
</head>
<body>
    <div class="sidebar">
        <div class="logo">⚡ FLXOS HUB</div>
        <ul class="nav-list">
            <li class="nav-item active" onclick="switchTab('dashboard')">Dashboard</li>
            <li class="nav-item" onclick="switchTab('wifi')">Wi-Fi Control</li>
            <li class="nav-item" onclick="switchTab('hotspot')">AP Hotspot</li>
            <li class="nav-item" onclick="switchTab('files')">Files</li>
        </ul>
    </div>

    <div class="content-area">
        <header>
            <h1 id="page-title">Dashboard</h1>
            <div class="status-badge">● Device Connected</div>
        </header>

        <!-- DASHBOARD TAB -->
        <div id="tab-dashboard" class="tab-content active">
            <div class="grid">
                <div class="card">
                    <div class="card-title">Free Heap RAM</div>
                    <div class="metric" id="stat-heap">-- KB</div>
                    <div class="metric-sub" id="stat-min-heap">Min historical limit: -- KB</div>
                </div>
                <div class="card">
                    <div class="card-title">System Uptime</div>
                    <div class="metric" id="stat-uptime">--s</div>
                    <div class="metric-sub" id="stat-chip">Hardware target: --</div>
                </div>
            </div>
            <div class="card">
                <div class="card-title">Active Applications</div>
                <div id="app-list">Loading registered user interfaces...</div>
            </div>
        </div>

        <!-- WI-FI CONTROL TAB -->
        <div id="tab-wifi" class="tab-content">
            <div class="grid">
                <div class="card">
                    <div class="card-title">Connect to Wi-Fi</div>
                    <input type="text" id="wifi-ssid" placeholder="Network SSID">
                    <input type="password" id="wifi-pass" placeholder="Passphrase">
                    <button class="btn" onclick="connectWifi()">Connect Station</button>
                </div>
                <div class="card">
                    <div class="card-title">Saved Networks</div>
                    <table id="saved-wifi-table">
                        <thead>
                            <tr>
                                <th>SSID</th>
                                <th>Priority</th>
                                <th>Actions</th>
                            </tr>
                        </thead>
                        <tbody>
                            <!-- Filled dynamically -->
                        </tbody>
                    </table>
                </div>
            </div>
            <div class="card">
                <div class="card-title" style="display:flex; justify-content:space-between; align-items:center;">
                    <span>Wi-Fi Network Scan</span>
                    <button class="btn" style="width:auto; padding:0.5rem 1.5rem;" onclick="scanWifi()">Scan</button>
                </div>
                <table id="scan-wifi-table">
                    <thead>
                        <tr>
                            <th>SSID</th>
                            <th>RSSI (Signal)</th>
                            <th>Channel</th>
                            <th>Security</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr><td colspan="4">Click Scan to search for active access points.</td></tr>
                    </tbody>
                </table>
            </div>
        </div>

        <!-- HOTSPOT TAB -->
        <div id="tab-hotspot" class="tab-content">
            <div class="grid">
                <div class="card">
                    <div class="card-title">AP Hotspot Settings</div>
                    <input type="text" id="hs-ssid" placeholder="Hotspot SSID">
                    <input type="password" id="hs-pass" placeholder="Passphrase">
                    <select id="hs-chan">
                        <option value="1">Channel 1 (2.412 GHz)</option>
                        <option value="6">Channel 6 (2.437 GHz)</option>
                        <option value="11">Channel 11 (2.462 GHz)</option>
                    </select>
                    <button class="btn" onclick="saveHotspot()">Apply and Start AP</button>
                    <button class="btn" style="background:#ef4444; margin-top:0.75rem; color:#fff;" onclick="stopHotspot()">Shutdown AP</button>
                </div>
                <div class="card">
                    <div class="card-title">Hotspot Status</div>
                    <div class="metric" id="hs-status-text">Inactive</div>
                    <div class="metric-sub" id="hs-clients-count">Connected clients: 0</div>
                </div>
            </div>
        </div>

        <!-- FILES TAB -->
        <div id="tab-files" class="tab-content">
            <div class="card">
                <div class="card-title" style="display:flex; justify-content:space-between; align-items:center;">
                    <span>File Explorer</span>
                    <input type="file" id="upload-file" style="display:none;" onchange="uploadFile()">
                    <button class="btn" style="width:auto; padding:0.5rem 1.5rem;" onclick="document.getElementById('upload-file').click()">Upload File</button>
                </div>
                <div class="file-list" id="file-explorer-list">
                    <!-- Loaded dynamically -->
                </div>
            </div>
        </div>
    </div>

    <script>
        let currentPath = "/data";

        function switchTab(tabId) {
            document.querySelectorAll('.nav-item').forEach(el => el.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));

            const activeItem = Array.from(document.querySelectorAll('.nav-item')).find(el => el.innerText.toLowerCase().includes(tabId));
            if (activeItem) activeItem.classList.add('active');

            const activeTab = document.getElementById('tab-' + tabId);
            if (activeTab) activeTab.classList.add('active');

            document.getElementById('page-title').innerText = activeItem.innerText;

            if (tabId === 'dashboard') loadStats();
            if (tabId === 'wifi') { loadSavedWifi(); }
            if (tabId === 'hotspot') { loadHotspot(); }
            if (tabId === 'files') { loadFiles(); }
        }

        // ──── Stats loading ────
        function loadStats() {
            fetch('/api/system')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('stat-heap').innerText = Math.round(data.heap / 1024) + " KB";
                    document.getElementById('stat-min-heap').innerText = "Min historical limit: " + Math.round(data.min_heap / 1024) + " KB";
                    document.getElementById('stat-uptime').innerText = data.uptime + "s";
                    document.getElementById('stat-chip').innerText = data.chip_model + " Cores: " + data.chip_cores;
                });

            fetch('/api/apps')
                .then(r => r.json())
                .then(data => {
                    const list = document.getElementById('app-list');
                    list.innerHTML = "";
                    data.forEach(app => {
                        const card = document.createElement('div');
                        card.style.padding = "0.75rem 1rem";
                        card.style.background = "rgba(255,255,255,0.03)";
                        card.style.borderRadius = "8px";
                        card.style.marginBottom = "0.5rem";
                        card.innerText = "🚀 " + app.name + " (" + app.id + ")";
                        list.appendChild(card);
                    });
                });
        }

        // ──── WiFi STA ────
        function loadSavedWifi() {
            fetch('/api/wifi/saved')
                .then(r => r.json())
                .then(data => {
                    const tbody = document.querySelector('#saved-wifi-table tbody');
                    tbody.innerHTML = "";
                    data.forEach(net => {
                        const tr = document.createElement('tr');
                        tr.innerHTML = `<td>${net.ssid}</td><td>${net.priority}</td><td><span class="action-link delete" onclick="deleteSaved('${net.ssid}')">Forget</span></td>`;
                        tbody.appendChild(tr);
                    });
                });
        }

        // ──── Forgotten Net ────
        function deleteSaved(ssid) {
            fetch(`/api/wifi/saved?ssid=${encodeURIComponent(ssid)}`, { method: 'DELETE' })
                .then(() => loadSavedWifi());
        }

        function scanWifi() {
            const tbody = document.querySelector('#scan-wifi-table tbody');
            tbody.innerHTML = '<tr><td colspan="4">Scanning background airwaves...</td></tr>';
            fetch('/api/wifi/scan', { method: 'POST' })
                .then(r => r.json())
                .then(data => {
                    tbody.innerHTML = "";
                    data.forEach(net => {
                        const tr = document.createElement('tr');
                        tr.style.cursor = "pointer";
                        tr.onclick = () => { document.getElementById('wifi-ssid').value = net.ssid; };
                        tr.innerHTML = `<td>${net.ssid}</td><td>${net.rssi} dBm</td><td>${net.channel}</td><td>${net.auth > 0 ? "WPA2/Secure" : "Open"}</td>`;
                        tbody.appendChild(tr);
                    });
                });
        }

        function connectWifi() {
            const ssid = document.getElementById('wifi-ssid').value;
            const password = document.getElementById('wifi-pass').value;
            fetch('/api/wifi/connect', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, password })
            }).then(() => alert("Connecting target wifi station..."));
        }

        // ──── Hotspot ────
        function loadHotspot() {
            fetch('/api/hotspot')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('hs-ssid').value = data.ssid;
                    document.getElementById('hs-pass').value = data.password;
                    document.getElementById('hs-chan').value = data.channel;
                    document.getElementById('hs-status-text').innerText = data.enabled ? "Active" : "Inactive";
                    document.getElementById('hs-clients-count').innerText = "Connected clients: " + data.clients;
                });
        }

        function saveHotspot() {
            const ssid = document.getElementById('hs-ssid').value;
            const password = document.getElementById('hs-pass').value;
            const channel = parseInt(document.getElementById('hs-chan').value);
            fetch('/api/hotspot/start', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, password, channel, max_connections: 4, hidden: false, nat_enabled: true })
            }).then(() => loadHotspot());
        }

        function stopHotspot() {
            fetch('/api/hotspot/stop', { method: 'POST' })
                .then(() => loadHotspot());
        }

        // ──── Files ────
        function loadFiles() {
            fetch(`/fs/list?path=${encodeURIComponent(currentPath)}`)
                .then(r => r.json())
                .then(data => {
                    const list = document.getElementById('file-explorer-list');
                    list.innerHTML = "";
                    
                    // Add Back Directory if not root
                    if (currentPath !== "/data" && currentPath !== "/sdcard") {
                        const row = document.createElement('div');
                        row.className = "file-row";
                        row.innerHTML = `<div class="file-info"><span class="file-icon">📁</span><span>..</span></div><div class="file-actions"><span class="action-link" onclick="goUp()">Back</span></div>`;
                        list.appendChild(row);
                    }

                    data.forEach(item => {
                        const row = document.createElement('div');
                        row.className = "file-row";
                        const icon = item.is_dir ? "📁" : "📄";
                        const size = item.is_dir ? "" : `(${Math.round(item.size/1024)} KB)`;
                        const path = currentPath + "/" + item.name;
                        
                        let actions = "";
                        if (item.is_dir) {
                            actions = `<span class="action-link" onclick="enterDir('${item.name}')">Enter</span>`;
                        } else {
                            actions = `<a class="action-link" href="/fs/download?path=${encodeURIComponent(path)}">Download</a>
                                       <span class="action-link delete" onclick="deleteFile('${path}')">Delete</span>`;
                        }

                        row.innerHTML = `<div class="file-info"><span class="file-icon">${icon}</span><span>${item.name} ${size}</span></div><div class="file-actions">${actions}</div>`;
                        list.appendChild(row);
                    });
                });
        }

        function enterDir(name) {
            currentPath = currentPath + "/" + name;
            loadFiles();
        }

        function goUp() {
            const idx = currentPath.lastIndexOf('/');
            if (idx > 0) {
                currentPath = currentPath.substring(0, idx);
                loadFiles();
            }
        }

        function deleteFile(path) {
            fetch(`/fs/delete?path=${encodeURIComponent(path)}`, { method: 'POST' })
                .then(() => loadFiles());
        }

        function uploadFile() {
            const fileInput = document.getElementById('upload-file');
            if (fileInput.files.length === 0) return;
            const file = fileInput.files[0];
            const path = currentPath + "/" + file.name;

            fetch(`/fs/upload?path=${encodeURIComponent(path)}`, {
                method: 'POST',
                body: file
            }).then(() => {
                fileInput.value = "";
                loadFiles();
            });
        }

        // Loop refresh dashboard stats
        setInterval(() => {
            if (document.getElementById('tab-dashboard').classList.contains('active')) {
                loadStats();
            }
        }, 3000);

        // Initial Stats Load
        loadStats();
    </script>
</body>
</html>)html";

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, html, strlen(html));
	return ESP_OK;
}

} // namespace flx::system::services
