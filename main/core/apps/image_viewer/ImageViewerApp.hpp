#pragma once

#include "lvgl.h"
#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppManifest.hpp>
#include <string>

// Use flx::apps namespace elements
using flx::apps::App;
using flx::apps::AppManifest;

namespace System::Apps {

class ImageViewerApp : public flx::apps::App {
public:

	std::string getPackageName() const override;
	std::string getAppName() const override;
	const void* getIcon() const override;

	static const AppManifest manifest;

	void createUI(void* parent) override;
	void onStop() override;

private:

	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_image = nullptr;
	lv_obj_t* m_errorLabel = nullptr;
	std::string m_filePath;
	std::string m_lvglPath; // Must persist — LVGL stores the pointer

	std::string getFileName(const std::string& path);
};

} // namespace System::Apps
