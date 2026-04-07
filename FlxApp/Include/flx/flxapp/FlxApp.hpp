#pragma once

#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/core/Value.hpp>

#include <optional>
#include <memory>
#include <string>

namespace flx::flxapp {

class FlxAppActionRunner;
class FlxAppRenderer;
class FlxAppState;

class FlxApp : public flx::apps::App {
public:

    explicit FlxApp(std::string sourcePath);
    ~FlxApp() override;

    bool onStart() override;
    void onStop() override;
    void createUI(void* parent) override;

    std::string getPackageName() const override;
    std::string getAppName() const override;
    const void* getIcon() const override;
    std::string getVersion() const override;

    const std::string& getSourcePath() const { return m_sourcePath; }

private:

    std::string m_sourcePath {};
    flx::apps::AppManifest m_manifest {};
    std::optional<flx::core::FlxValueDocument> m_document {};
    flx::core::FlxValueView m_uiRoot {};
    std::unique_ptr<FlxAppState> m_state;
    std::unique_ptr<FlxAppActionRunner> m_actionRunner;
    std::unique_ptr<FlxAppRenderer> m_renderer;

    bool loadDocument();
};

} // namespace flx::flxapp
