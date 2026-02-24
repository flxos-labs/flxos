#pragma once

#include "lvgl.h"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <string>

namespace System::Apps {

class ImageViewerApp : public flx::apps::App {
public:

	std::string getPackageName() const override;
	std::string getAppName() const override;
	const void* getIcon() const override;

	static const flx::apps::AppManifest manifest;

	void createUI(void* parent) override;
	void onStop() override;
	void onNewIntent(const flx::apps::Intent& intent) override;

private:

	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_image = nullptr;
	lv_obj_t* m_errorLabel = nullptr;
	std::string m_filePath;
	std::string m_lvglPath; // Must persist — LVGL stores the pointer

	std::string getFileName(const std::string& path);
};

} // namespace System::Apps
