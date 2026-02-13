#include "ImageViewerApp.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include "core/common/Logger.hpp"
#include "core/ui/theming/ui_constants/UiConstants.hpp"
#include "font/lv_symbol_def.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include <string_view>

static constexpr std::string_view TAG = "ImageViewer";

namespace System::Apps {

std::string ImageViewerApp::getPackageName() const { return manifest.appId; }
std::string ImageViewerApp::getAppName() const { return manifest.appName; }
const void* ImageViewerApp::getIcon() const { return manifest.appIcon; }

std::string ImageViewerApp::getFileName(const std::string& path) {
	auto pos = path.rfind('/');
	if (pos != std::string::npos && pos + 1 < path.size()) {
		return path.substr(pos + 1);
	}
	return path;
}

void ImageViewerApp::createUI(void* parent) {
	auto* parentObj = static_cast<lv_obj_t*>(parent);

	m_container = Settings::create_page_container(parentObj);

	// Get the file path from the intent data
	if (m_context) {
		m_filePath = m_context->getData();
	}

	std::string title = m_filePath.empty() ? "Image Viewer" : getFileName(m_filePath);

	// Header with back button and filename
	lv_obj_t* backBtn = nullptr;
	Settings::create_header(m_container, title.c_str(), &backBtn);

	// Back button closes the app
	lv_obj_add_event_cb(
		backBtn,
		[](lv_event_t* e) {
			auto* app = static_cast<ImageViewerApp*>(lv_event_get_user_data(e));
			AppManager::getInstance().stopApp(app->getPackageName());
		},
		LV_EVENT_CLICKED, this
	);

	// Image content area — scrollable, centered
	lv_obj_t* content = lv_obj_create(m_container);
	lv_obj_set_size(content, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_set_style_pad_all(content, 0, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_scroll_dir(content, LV_DIR_ALL);

	if (m_filePath.empty()) {
		m_errorLabel = lv_label_create(content);
		lv_label_set_text(m_errorLabel, "No file specified");
		lv_obj_set_style_text_color(m_errorLabel, lv_color_hex(0xFF5252), 0);
		return;
	}

	// Convert VFS path to LVGL path: "/data/foo.bmp" → "A:/data/foo.bmp"
	m_lvglPath = "A:" + m_filePath;
	Log::info(TAG, "Opening image: %s", m_lvglPath.c_str());

	m_image = lv_image_create(content);
	lv_image_set_src(m_image, m_lvglPath.c_str());
	lv_obj_set_style_image_recolor_opa(m_image, LV_OPA_TRANSP, 0);

	// Check if image loaded successfully
	lv_image_header_t header;
	if (lv_image_decoder_get_info(m_lvglPath.c_str(), &header) != LV_RESULT_OK) {
		Log::error(TAG, "Failed to decode image: %s", m_lvglPath.c_str());
		lv_obj_del(m_image);
		m_image = nullptr;

		m_errorLabel = lv_label_create(content);
		lv_label_set_text(m_errorLabel, "Failed to load image");
		lv_obj_set_style_text_color(m_errorLabel, lv_color_hex(0xFF5252), 0);
	}
}

void ImageViewerApp::onStop() {
	m_container = nullptr;
	m_image = nullptr;
	m_errorLabel = nullptr;
	m_filePath.clear();
	m_lvglPath.clear();
}

} // namespace System::Apps
