#pragma once

#include "core/apps/AppManager.hpp"
#include "core/apps/AppManifest.hpp"
#include "lvgl.h"
#include <string>

namespace System::Apps {

class ImageViewerApp : public App {
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
	std::string m_lvglPath;  // Must persist — LVGL stores the pointer

	std::string getFileName(const std::string& path);
};

} // namespace System::Apps
