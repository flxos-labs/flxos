#include <flx/flxapp/FlxApp.hpp>

#include <cJSON.h>
#include <flx/core/Logger.hpp>
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

void FlxApp::JsonDeleter::operator()(cJSON* json) const {
    if (json != nullptr) {
        cJSON_Delete(json);
    }
}

bool FlxApp::onStart() {
    return loadDocument();
}

void FlxApp::onStop() {
    m_renderer.reset();
    m_actionRunner.reset();
    m_state.reset();
    m_uiRoot = nullptr;
    m_document.reset();
}

void FlxApp::createUI(void* parent) {
    if (m_renderer == nullptr || m_uiRoot == nullptr) {
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

bool FlxApp::loadDocument() {
    const std::string raw = readFile(m_sourcePath);
    if (raw.empty()) {
        Log::error(TAG, "Failed to read app source: %s", m_sourcePath.c_str());
        return false;
    }

    m_document.reset(cJSON_Parse(raw.c_str()));
    if (!m_document) {
        Log::error(TAG, "Failed to parse app source as JSON: %s", m_sourcePath.c_str());
        return false;
    }

    const cJSON* variables = cJSON_GetObjectItemCaseSensitive(m_document.get(), "variables");
    m_uiRoot = cJSON_GetObjectItemCaseSensitive(m_document.get(), "ui");
    if (m_uiRoot == nullptr) {
        Log::error(TAG, "FlxApp is missing 'ui': %s", m_sourcePath.c_str());
        return false;
    }

    m_state = std::make_unique<FlxAppState>();
    m_actionRunner = std::make_unique<FlxAppActionRunner>(*this, *m_state);
    m_renderer = std::make_unique<FlxAppRenderer>(*m_state, *m_actionRunner);

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
    m_state->loadFromJson(variables);

    return true;
}

} // namespace flx::flxapp
