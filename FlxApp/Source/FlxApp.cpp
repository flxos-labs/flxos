#include <flx/flxapp/FlxApp.hpp>

#include <flx/core/Logger.hpp>
#include <flx/core/Value.hpp>
#include <flx/flxapp/FlxAppActionRunner.hpp>
#include <flx/flxapp/FlxAppManifest.hpp>
#include <flx/flxapp/FlxAppRenderer.hpp>
#include <flx/flxapp/FlxAppState.hpp>

#include <fstream>
#include <sstream>
#include <utility>

namespace flx::flxapp {

namespace {

constexpr const char* TAG = "FlxApp";

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

} // namespace

FlxApp::FlxApp(std::string sourcePath)
    : m_sourcePath(std::move(sourcePath)) {
    auto manifest = FlxAppManifest::loadFromFile(m_sourcePath);
    if (manifest) {
        m_manifest = *manifest;
    } else {
        m_manifest.appId = m_sourcePath;
        m_manifest.appName = "FlxApp";
        m_manifest.appVersionName = "0.1.0";
    }
}

FlxApp::~FlxApp() = default;

bool FlxApp::onStart() {
    return loadDocument();
}

void FlxApp::onStop() {
    m_renderer.reset();
    m_actionRunner.reset();
    m_state.reset();
    m_uiRoot = {};
    m_document.reset();
}

void FlxApp::createUI(void* parent) {
    if (m_renderer == nullptr || !m_uiRoot.valid()) {
        return;
    }
    m_renderer->render(parent, m_uiRoot);
}

std::string FlxApp::getPackageName() const {
    return m_manifest.appId;
}

std::string FlxApp::getAppName() const {
    return m_manifest.appName;
}

const void* FlxApp::getIcon() const {
    return m_manifest.appIcon;
}

std::string FlxApp::getVersion() const {
    return m_manifest.appVersionName;
}

void FlxApp::applyHttpGetResult(
	const std::string& responseKey,
	const std::string& statusKey,
	const std::string& errorKey,
	int32_t statusCode,
	bool success,
	const std::string& body,
	const std::string& errorMessage) {
	if (!m_state) {
		return;
	}

	if (!statusKey.empty()) {
		m_state->setInt(statusKey, statusCode);
	}

	if (success) {
		if (!responseKey.empty()) {
			m_state->setString(responseKey, body);
		}
		return;
	}

	if (!errorKey.empty()) {
		m_state->setString(errorKey, errorMessage);
	}
}

bool FlxApp::loadDocument() {
    const std::string raw = readFile(m_sourcePath);
    if (raw.empty()) {
        Log::error(TAG, "Failed to read app source: %s", m_sourcePath.c_str());
        return false;
    }

    auto document = flx::core::FlxValueDocument::parseAuto(std::move(raw));
    if (!document) {
        Log::error(TAG, "Failed to parse app source: %s", m_sourcePath.c_str());
        return false;
    }

    m_document = std::move(document);

    const flx::core::FlxValueView root = m_document->root();
    const flx::core::FlxValueView variables = root.child("variables");
    m_uiRoot = root.child("ui");
    if (!m_uiRoot.valid()) {
        Log::error(TAG, "FlxApp is missing 'ui' section: %s", m_sourcePath.c_str());
        return false;
    }

    m_state        = std::make_unique<FlxAppState>();
    m_actionRunner = std::make_unique<FlxAppActionRunner>(*this, *m_state);
    m_renderer     = std::make_unique<FlxAppRenderer>(*m_state, *m_actionRunner);

    m_state->setChangeCallback([this]() {
        if (m_renderer) {
            m_renderer->refreshBindings();
        }
    });
    m_actionRunner->setRefreshCallback([this]() {
        if (m_renderer) {
            m_renderer->refreshBindings();
        }
    });
    m_state->loadFromValue(variables);

    return true;
}

} // namespace flx::flxapp
