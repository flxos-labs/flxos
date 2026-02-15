#pragma once

#include "lvgl.h"
#include <flx/ui/app/AppManager.hpp>
#include <flx/ui/app/AppManifest.hpp>
#include <string>

namespace System::Apps {

using flx::app::AppManifest;

class ImageViewerApp : public flx::app::App {
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
