#include <flx/core/Logger.hpp>
#include <flx/hal/gpio/EspGpioController.hpp>

namespace flx::hal::gpio {

static constexpr std::string_view TAG = "EspGpio";

EspGpioController::EspGpioController() {
	this->setState(State::Uninitialized);
}

EspGpioController::~EspGpioController() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool EspGpioController::start() {
	this->setState(State::Starting);

	// Initialize the debounce queue (holds Pin numbers)
	m_debounceQueue = xQueueCreate(20, sizeof(Pin));
	if (!m_debounceQueue) {
		flx::Log::error(TAG, "Failed to create debounce queue");
		this->setState(State::Error);
		return false;
	}

	m_taskRunning = true;
	if (xTaskCreate(debounceTaskRunner, "gpio_debounce", 3072, this, 10, &m_debounceTaskHandle) != pdPASS) {
		flx::Log::error(TAG, "Failed to create debounce task");
		vQueueDelete(m_debounceQueue);
		m_debounceQueue = nullptr;
		this->setState(State::Error);
		return false;
	}

	this->setState(State::Ready);
	return true;
}

bool EspGpioController::stop() {
	if (m_ownsIsrService) {
		gpio_uninstall_isr_service();
		m_ownsIsrService = false;
		m_isrServiceInstalled = false;
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto& pair: m_pinConfigs) {
			gpio_isr_handler_remove(static_cast<gpio_num_t>(pair.first));
			delete pair.second;
		}
		m_pinConfigs.clear();
	}

	m_taskRunning = false;
	if (m_debounceQueue) {
		// Send a dummy to wake up the task and exit
		Pin dummy = PIN_NC;
		xQueueSend(m_debounceQueue, &dummy, 0);
	}

	if (m_debounceTaskHandle) {
		// Wait briefly for task to exit
		vTaskDelay(pdMS_TO_TICKS(50));
		eTaskState state = eTaskGetState(m_debounceTaskHandle);
		if (state != eDeleted) {
			vTaskDelete(m_debounceTaskHandle);
		}
		m_debounceTaskHandle = nullptr;
	}

	if (m_debounceQueue) {
		vQueueDelete(m_debounceQueue);
		m_debounceQueue = nullptr;
	}

	this->setState(State::Stopped);
	return true;
}

bool EspGpioController::configure(Pin pin, GpioMode mode, GpioPullMode pull) {
	if (pin == PIN_NC) return false;

	gpio_config_t conf = {};
	conf.pin_bit_mask = (1ULL << pin);
	conf.intr_type = GPIO_INTR_DISABLE;

	switch (mode) {
		case GpioMode::Disable:
			conf.mode = GPIO_MODE_DISABLE;
			break;
		case GpioMode::Input:
			conf.mode = GPIO_MODE_INPUT;
			break;
		case GpioMode::Output:
			conf.mode = GPIO_MODE_OUTPUT;
			break;
		case GpioMode::OutputOpenDrain:
			conf.mode = GPIO_MODE_OUTPUT_OD;
			break;
		case GpioMode::InputOutput:
			conf.mode = GPIO_MODE_INPUT_OUTPUT;
			break;
		case GpioMode::InputOutputOpenDrain:
			conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
			break;
	}

	switch (pull) {
		case GpioPullMode::None:
			conf.pull_up_en = GPIO_PULLUP_DISABLE;
			conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
			break;
		case GpioPullMode::Up:
			conf.pull_up_en = GPIO_PULLUP_ENABLE;
			conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
			break;
		case GpioPullMode::Down:
			conf.pull_up_en = GPIO_PULLUP_DISABLE;
			conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
			break;
		case GpioPullMode::Both:
			conf.pull_up_en = GPIO_PULLUP_ENABLE;
			conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
			break;
	}

	esp_err_t err = gpio_config(&conf);
	if (err != ESP_OK) {
		flx::Log::error(TAG, "gpio_config failed for pin %d: %s", pin, esp_err_to_name(err));
		return false;
	}
	return true;
}

bool EspGpioController::setLevel(Pin pin, bool level) {
	if (pin == PIN_NC) return false;
	return gpio_set_level(static_cast<gpio_num_t>(pin), level ? 1 : 0) == ESP_OK;
}

bool EspGpioController::getLevel(Pin pin) const {
	if (pin == PIN_NC) return false;
	return gpio_get_level(static_cast<gpio_num_t>(pin)) == 1;
}

int EspGpioController::getPinCount() const {
	return GPIO_NUM_MAX;
}

bool EspGpioController::attachInterrupt(Pin pin, GpioInterruptEdge edge, GpioCallback callback) {
	if (pin == PIN_NC || !callback) return false;

	std::lock_guard<std::mutex> lock(m_mutex);

	gpio_int_type_t intr_type;
	switch (edge) {
		case GpioInterruptEdge::Rising:
			intr_type = GPIO_INTR_POSEDGE;
			break;
		case GpioInterruptEdge::Falling:
			intr_type = GPIO_INTR_NEGEDGE;
			break;
		case GpioInterruptEdge::Both:
			intr_type = GPIO_INTR_ANYEDGE;
			break;
		default:
			intr_type = GPIO_INTR_DISABLE;
			break;
	}

	gpio_set_intr_type(static_cast<gpio_num_t>(pin), intr_type);

	if (!m_isrServiceInstalled) {
		esp_err_t err = gpio_install_isr_service(0);
		if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
			flx::Log::error(TAG, "Failed to install ISR service: %s", esp_err_to_name(err));
			return false;
		}
		m_ownsIsrService = (err == ESP_OK);
		m_isrServiceInstalled = true;
	}

	auto it = m_pinConfigs.find(pin);
	PinConfig* config = nullptr;
	if (it == m_pinConfigs.end()) {
		config = new PinConfig {this, pin, callback, 0, 0, false};
		m_pinConfigs[pin] = config;
	} else {
		gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
		config = it->second;
		config->callback = callback;
		config->isDebounced = false;
	}

	esp_err_t err = gpio_isr_handler_add(static_cast<gpio_num_t>(pin), gpioIsrHandler, config);
	return err == ESP_OK;
}

bool EspGpioController::detachInterrupt(Pin pin) {
	if (pin == PIN_NC) return false;

	std::lock_guard<std::mutex> lock(m_mutex);
	gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));

	auto it = m_pinConfigs.find(pin);
	if (it != m_pinConfigs.end()) {
		delete it->second;
		m_pinConfigs.erase(it);
	}
	return true;
}

bool EspGpioController::configureDebounced(Pin pin, uint32_t debounceMs, GpioCallback callback) {
	if (pin == PIN_NC || !callback) return false;

	std::lock_guard<std::mutex> lock(m_mutex);

	// For debounce, trigger on any edge and handle logic in task
	gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_ANYEDGE);

	if (!m_isrServiceInstalled) {
		esp_err_t err = gpio_install_isr_service(0);
		if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
			flx::Log::error(TAG, "Failed to install ISR service: %s", esp_err_to_name(err));
			return false;
		}
		m_ownsIsrService = (err == ESP_OK);
		m_isrServiceInstalled = true;
	}

	auto it = m_pinConfigs.find(pin);
	PinConfig* config = nullptr;
	if (it == m_pinConfigs.end()) {
		config = new PinConfig {this, pin, callback, debounceMs, 0, true};
		m_pinConfigs[pin] = config;
	} else {
		gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
		config = it->second;
		config->callback = callback;
		config->debounceMs = debounceMs;
		config->isDebounced = true;
	}

	esp_err_t err = gpio_isr_handler_add(static_cast<gpio_num_t>(pin), gpioIsrHandler, config);
	return err == ESP_OK;
}

void IRAM_ATTR EspGpioController::gpioIsrHandler(void* arg) {
	PinConfig* config = static_cast<PinConfig*>(arg);
	if (!config || !config->controller || !config->controller->m_debounceQueue) return;

	if (config->isDebounced) {
		TickType_t now = xTaskGetTickCountFromISR();
		if ((now - config->lastIsrTick) * portTICK_PERIOD_MS >= config->debounceMs) {
			config->lastIsrTick = now;
			BaseType_t higherPriorityTaskWoken = pdFALSE;
			xQueueSendFromISR(config->controller->m_debounceQueue, &config->pin, &higherPriorityTaskWoken);
			if (higherPriorityTaskWoken) {
				portYIELD_FROM_ISR();
			}
		}
	} else {
		BaseType_t higherPriorityTaskWoken = pdFALSE;
		xQueueSendFromISR(config->controller->m_debounceQueue, &config->pin, &higherPriorityTaskWoken);
		if (higherPriorityTaskWoken) {
			portYIELD_FROM_ISR();
		}
	}
}

void EspGpioController::debounceTaskRunner(void* arg) {
	EspGpioController* controller = static_cast<EspGpioController*>(arg);
	Pin pin;

	while (controller->m_taskRunning) {
		if (xQueueReceive(controller->m_debounceQueue, &pin, portMAX_DELAY) == pdTRUE) {
			if (pin == PIN_NC) break; // Exit signal

			PinConfig* config = nullptr;
			GpioCallback cb = nullptr;
			{
				std::lock_guard<std::mutex> lock(controller->m_mutex);
				auto it = controller->m_pinConfigs.find(pin);
				if (it != controller->m_pinConfigs.end()) {
					config = it->second;
					if (config) cb = config->callback;
				}
			}

			if (cb) {
				// Re-read level after taking off queue to confirm state
				bool level = gpio_get_level(static_cast<gpio_num_t>(pin)) == 1;
				cb(pin, level);
			}
		}
	}
	vTaskDelete(nullptr);
}

} // namespace flx::hal::gpio
