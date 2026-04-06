#pragma once

#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <memory>
#include <string>

struct cJSON;

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

    struct JsonDeleter {
        void operator()(cJSON* json) const;
    };

    std::string m_sourcePath {};
    flx::apps::AppManifest m_manifest {};
    std::unique_ptr<cJSON, JsonDeleter> m_document {};
    const cJSON* m_uiRoot = nullptr;
    std::unique_ptr<FlxAppState> m_state;
    std::unique_ptr<FlxAppActionRunner> m_actionRunner;
    std::unique_ptr<FlxAppRenderer> m_renderer;

    bool loadDocument();
};

} // namespace flx::flxapp
