#include <flx/apps/AppManifest.hpp>
#include <flx/apps/AppRegistry.hpp>

#include "calendar/CalendarApp.hpp"
#include "files/FilesApp.hpp"
#include "image_viewer/ImageViewerApp.hpp"
#include "settings/SettingsApp.hpp"
#include "system_info/SystemInfoApp.hpp"
#include "text_editor/TextEditorApp.hpp"
#include "tools/ToolsApp.hpp"
#include "wallpaper_engine/WallpaperEngineApp.hpp"

namespace System::Apps {

void registerBuiltInApps() {
	auto& appRegistry = flx::apps::AppRegistry::getInstance();

	appRegistry.addApp(SettingsApp::manifest);
	appRegistry.addApp(CalendarApp::manifest);
	appRegistry.addApp(FilesApp::manifest);
	appRegistry.addApp(ImageViewerApp::manifest);
	appRegistry.addApp(SystemInfoApp::manifest);
	appRegistry.addApp(TextEditorApp::manifest);
	appRegistry.addApp(ToolsApp::manifest);
	appRegistry.addApp(WallpaperEngineApp::manifest);
}

} // namespace System::Apps
