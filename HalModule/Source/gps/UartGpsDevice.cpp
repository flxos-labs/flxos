#include <cstdlib>
#include <cstring>
#include <flx/core/Logger.hpp>
#include <flx/hal/gps/UartGpsDevice.hpp>

namespace flx::hal::gps {

static constexpr std::string_view TAG = "UartGpsDevice";

UartGpsDevice::UartGpsDevice(std::shared_ptr<flx::hal::uart::IUartBus> uartBus)
	: m_uart(std::move(uartBus)) {
	this->setState(State::Uninitialized);
}

UartGpsDevice::~UartGpsDevice() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool UartGpsDevice::start() {
	if (!m_uart) return false;

	this->setState(State::Starting);

	if (!m_uart->open(9600)) { // Default fallback, but Probe usually sets this up
		flx::Log::error(TAG, "Failed to open UART bus");
		this->setState(State::Error);
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_isRunning = true;
		m_state = GpsState::Searching;
	}

	if (xTaskCreate(rxTaskRunner, "gps_rx_task", 4096, this, 5, &m_rxTaskHandle) != pdPASS) {
		flx::Log::error(TAG, "Failed to create GPS RX task");
		m_isRunning = false;
		if (m_uart) m_uart->close();
		this->setState(State::Error);
		return false;
	}

	this->setState(State::Ready);
	flx::Log::info(TAG, "UART GPS started");
	return true;
}

bool UartGpsDevice::stop() {
	m_isRunning = false;

	if (m_rxTaskHandle) {
		// Wait briefly for task to exit its loop
		vTaskDelay(pdMS_TO_TICKS(50));
		vTaskDelete(m_rxTaskHandle);
		m_rxTaskHandle = nullptr;
	}

	if (m_uart) {
		m_uart->close();
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_state = GpsState::Off;
	}
	this->setState(State::Stopped);
	return true;
}

IGpsDevice::GpsState UartGpsDevice::getGpsState() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_state;
}

GpsPosition UartGpsDevice::getLastPosition() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_lastPosition;
}

int UartGpsDevice::subscribePosition(PositionCallback cb) {
	std::lock_guard<std::mutex> lock(m_mutex);
	int id = m_nextObserverId++;
	m_observers.emplace_back(id, cb);
	return id;
}

void UartGpsDevice::unsubscribePosition(int id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
		if (it->first == id) {
			m_observers.erase(it);
			break;
		}
	}
}

std::string_view UartGpsDevice::getGpsModel() const {
	return m_model;
}

void UartGpsDevice::setModel(std::string_view model) {
	m_model = std::string(model);
}

void UartGpsDevice::requestColdStart() {
	flx::Log::info(TAG, "Requesting cold start (sending proprietary NMEA command)...");
	// This varies by module. For MTK (ATGM336H):
	// $PMTK103*30

	if (m_uart) {
		const char* cmd = "$PMTK103*30\r\n";
		m_uart->write(reinterpret_cast<const uint8_t*>(cmd), strlen(cmd), 100);
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	m_state = GpsState::Searching;
}

void UartGpsDevice::rxTaskRunner(void* arg) {
	auto* device = static_cast<UartGpsDevice*>(arg);
	while (device->m_isRunning) {
		device->processIncomingData();
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

void UartGpsDevice::processIncomingData() {
	uint8_t buf[128];
	size_t len = m_uart->read(buf, sizeof(buf) - 1, 10);
	if (len > 0) {
		buf[len] = '\0';
		// Basic line splitting
		// In a production system, use a robust NMEA parsing library like minmea
		for (size_t i = 0; i < len; ++i) {
			char c = static_cast<char>(buf[i]);
			if (c == '\n') {
				if (!m_lineBuffer.empty() && m_lineBuffer.back() == '\r') {
					m_lineBuffer.pop_back();
				}
				parseNmeaSentence(m_lineBuffer);
				m_lineBuffer.clear();
			} else {
				m_lineBuffer += c;
			}
		}
	}
}

void UartGpsDevice::parseNmeaSentence(const std::string& sentence) {
	if (sentence.rfind("$GNGGA", 0) == 0 || sentence.rfind("$GPGGA", 0) == 0) {
		// Mock parse: $GNGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
		// Normally would parse comma separated tokens
		bool notify = false;
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			// Very dummy check for fix quality
			// Token 6 is fix quality (0=invalid, 1=GPS fix, 2=DGPS fix)
			size_t commas = 0;
			for (size_t i = 0; i < sentence.size(); ++i) {
				if (sentence[i] == ',') commas++;
				if (commas == 6) {
					char fixQ = (i + 1 < sentence.size()) ? sentence[i + 1] : '0';
					m_lastPosition.valid = (fixQ == '1' || fixQ == '2');
					m_state = m_lastPosition.valid ? GpsState::FixAcquired : GpsState::Searching;
					break;
				}
			}
			notify = m_lastPosition.valid;
		}

		if (notify) {
			notifyObservers();
		}
	}
}

void UartGpsDevice::notifyObservers() {
	std::vector<PositionCallback> callbacks;
	GpsPosition pos;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (const auto& observer: m_observers) {
			callbacks.push_back(observer.second);
		}
		pos = m_lastPosition;
	}

	for (const auto& cb: callbacks) {
		if (cb) cb(pos);
	}
}

} // namespace flx::hal::gps
