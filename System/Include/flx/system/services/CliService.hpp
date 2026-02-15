#pragma once

#include "esp_err.h"
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <string>

namespace flx::system {

/**
 * @brief CLI Service for headless mode operation
 *
 * Provides an interactive command-line interface via serial console
 * using ESP-IDF's esp_console component. Designed for future GUI integration
 * where commands could be sent via other interfaces (e.g., serial terminal widget).
 */
class CliService : public flx::services::IService {
public:

	static CliService& getInstance();

	// ──── IService manifest ────
	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

	/**
	 * @brief Get current working directory
	 */
	const std::string& getCurrentDirectory() const { return m_currentDirectory; }

	/**
	 * @brief Set current working directory
	 */
	void setCurrentDirectory(const std::string& path) { m_currentDirectory = path; }

private:

	CliService() = default;
	~CliService() = default;
	CliService(const CliService&) = delete;
	CliService& operator=(const CliService&) = delete;

	static void registerCommands();

	std::string m_currentDirectory = "/";
};

} // namespace System
