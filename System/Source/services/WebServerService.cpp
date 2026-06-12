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

	uint8_t* rx_buffer = (uint8_t*)malloc(512);
	uint8_t* tx_buffer = (uint8_t*)malloc(512);
	if (!rx_buffer || !tx_buffer) {
		Log::error(TAG, "Failed to allocate DNS buffers");
		free(rx_buffer);
		free(tx_buffer);
		close(s_dns_sock);
		s_dns_sock = -1;
		s_dns_task_handle = nullptr;
		vTaskDelete(nullptr);
		return;
	}

	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	while (true) {
		int len = recvfrom(s_dns_sock, rx_buffer, 512, 0, (struct sockaddr*)&client_addr, &client_addr_len);
		if (len < 12) {
			if (len < 0) break;
			continue;
		}

		uint16_t flags = (rx_buffer[2] << 8) | rx_buffer[3];
		uint16_t questions = (rx_buffer[4] << 8) | rx_buffer[5];
		if ((flags & 0x8000) == 0 && questions == 1) {
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

	free(rx_buffer);
	free(tx_buffer);
	close(s_dns_sock);
	s_dns_sock = -1;
	s_dns_task_handle = nullptr;
	vTaskDelete(nullptr);
}

static void start_dns_server() {
	if (s_dns_task_handle == nullptr) {
		xTaskCreate(dns_captive_portal_task, "dns_captive", 4096, nullptr, 4, &s_dns_task_handle);
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
		Log::info(TAG, "webserverEnabled observable changed to %ld", (long)val);
		if (val) {
			startServer();
		} else {
			stopServer();
		}
	});

	webserverPort.subscribe([this](int32_t val) {
		Log::info(TAG, "webserverPort observable changed to %ld", (long)val);
		if (isServerRunning()) {
			stopServer();
			startServer();
		}
	});

	Log::info(TAG, "onStart: Initial webserverEnabled value: %ld", (long)webserverEnabled.get());
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

	Log::info(TAG, "registerEventHandlers: Subscribing to Hotspot events");

	// Subscribe to Hotspot events to manage DNS server
	m_hotspot_enabled_sub_id = ConnectivityManager::getInstance().getHotspotEnabledObservable().subscribe([this](int32_t enabled) {
		bool running = isServerRunning();
		Log::info(TAG, "Hotspot enabled callback: enabled=%ld, isServerRunning=%d", (long)enabled, running);
		if (running) {
			if (enabled) {
				start_dns_server();
			} else {
				stop_dns_server();
			}
		}
	});

	m_events_registered = true;
}

void WebServerService::unregisterEventHandlers() {
	if (!m_events_registered) return;
	ConnectivityManager::getInstance().getHotspotEnabledObservable().unsubscribe(m_hotspot_enabled_sub_id);
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
	const char* filepath = "/data/index.html";
	FILE* f = fopen(filepath, "r");
	if (!f) {
		Log::error(TAG, "Dashboard file not found: %s", filepath);
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Dashboard HTML file not found in storage");
		return ESP_FAIL;
	}

	httpd_resp_set_type(req, "text/html");

	char buf[1024];
	size_t read_bytes = 0;
	while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
		if (httpd_resp_send_chunk(req, buf, read_bytes) != ESP_OK) {
			fclose(f);
			httpd_resp_send_chunk(req, nullptr, 0);
			return ESP_FAIL;
		}
	}
	fclose(f);
	httpd_resp_send_chunk(req, nullptr, 0);
	return ESP_OK;
}

} // namespace flx::system::services
