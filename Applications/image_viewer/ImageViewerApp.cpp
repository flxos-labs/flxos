#include "ImageViewerApp.hpp"
#include "font/lv_symbol_def.h"
#include "misc/cache/instance/lv_image_cache.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include <flx/apps/AppManager.hpp>
#include <flx/core/Logger.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>
#include <string_view>

static constexpr std::string_view TAG = "ImageViewer";

namespace System::Apps {

using namespace flx::apps;
using namespace flx::ui::common;

const flx::apps::AppManifest ImageViewerApp::manifest = {
	.appId = "com.flxos.imageviewer",
	.appName = "Image Viewer",
	.appIcon = LV_SYMBOL_IMAGE,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::Hidden,
	.location = AppLocation::internal(),
	.description = "View image files",
	.sortPriority = 100,
	.capabilities = AppCapability::Storage,
	.supportedMimeTypes = {"image/bmp", "image/*"},
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<ImageViewerApp>(); }
};

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

	m_container = create_page_container(parentObj);

	// Get the file path from the intent data
	if (m_context) {
		m_filePath = m_context->getData();
	}

	std::string title = m_filePath.empty() ? "Image Viewer" : getFileName(m_filePath);

	// Header with back button and filename
	lv_obj_t* backBtn = nullptr;
	lv_obj_t* headerBar = create_header(m_container, title.c_str(), &backBtn);

	// Prevent whole header from scrolling and make only filename scroll
	lv_obj_remove_flag(headerBar, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_t* titleLabel = lv_obj_get_child(headerBar, 1);
	if (titleLabel) {
		lv_obj_set_width(titleLabel, 0);
		lv_obj_set_flex_grow(titleLabel, 1);
		lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
	}

	// Back button closes the app
	lv_obj_add_event_cb(
		backBtn,
		[](lv_event_t* e) {
			auto* app = static_cast<ImageViewerApp*>(lv_event_get_user_data(e));
			flx::apps::AppManager::getInstance().stopApp(app->getPackageName());
		},
		LV_EVENT_CLICKED, this
	);

	// Image content area — no scrolling, image fits inside
	lv_obj_t* content = lv_obj_create(m_container);
	lv_obj_set_size(content, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_set_style_pad_all(content, 0, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_scroll_dir(content, LV_DIR_NONE);
	lv_obj_set_style_clip_corner(content, true, 0);

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
	lv_obj_set_size(m_image, lv_pct(100), lv_pct(100));
	lv_image_set_inner_align(m_image, LV_IMAGE_ALIGN_CONTAIN);
	lv_obj_set_style_image_recolor_opa(m_image, LV_OPA_TRANSP, 0);

	// Check if image loaded successfully
	lv_image_header_t header;
	if (lv_image_decoder_get_info(m_lvglPath.c_str(), &header) != LV_RESULT_OK) {
		Log::error(TAG, "Failed to decode image: %s", m_lvglPath.c_str());
		// lv_image_create handles displaying a placeholder or nothing,
		// but we add an error label for clarity.
		if (m_image) {
			lv_obj_del(m_image);
			m_image = nullptr;
		}

		m_errorLabel = lv_label_create(content);
		lv_label_set_text(m_errorLabel, "Failed to load image");
		lv_obj_set_style_text_color(m_errorLabel, lv_color_hex(0xFF5252), 0);
	}
}

void ImageViewerApp::onNewIntent(const flx::apps::Intent& intent) {
	if (intent.data.empty()) return;

	// Drop cache for the old image before switching
	if (!m_lvglPath.empty() && lv_image_cache_is_enabled()) {
		lv_image_cache_drop(m_lvglPath.c_str());
	}

	m_filePath = intent.data;
	m_lvglPath = "A:" + m_filePath;

	// Update header title
	if (m_container) {
		lv_obj_t* header = lv_obj_get_child(m_container, 0); // Header container
		if (header) {
			lv_obj_t* titleLabel = lv_obj_get_child(header, 1); // Title label
			if (titleLabel) lv_label_set_text(titleLabel, getFileName(m_filePath).c_str());
		}
	}

	// Safely clear old UI elements
	if (m_image) {
		lv_obj_del(m_image);
		m_image = nullptr;
	}
	if (m_errorLabel) {
		lv_obj_del(m_errorLabel);
		m_errorLabel = nullptr;
	}

	// Recreate image in the content container securely
	lv_obj_t* content = lv_obj_get_child(m_container, 1);
	if (!content) return;

	m_image = lv_image_create(content);
	lv_image_set_src(m_image, m_lvglPath.c_str());
	lv_obj_set_size(m_image, lv_pct(100), lv_pct(100));
	lv_image_set_inner_align(m_image, LV_IMAGE_ALIGN_CONTAIN);
	lv_obj_set_style_image_recolor_opa(m_image, LV_OPA_TRANSP, 0);

	lv_image_header_t header_info;
	if (lv_image_decoder_get_info(m_lvglPath.c_str(), &header_info) != LV_RESULT_OK) {
		Log::error(TAG, "Failed to load image on new intent: %s", m_lvglPath.c_str());
		if (m_image) {
			lv_obj_del(m_image);
			m_image = nullptr;
		}

		m_errorLabel = lv_label_create(content);
		lv_label_set_text(m_errorLabel, "Failed to load image");
		lv_obj_set_style_text_color(m_errorLabel, lv_color_hex(0xFF5252), 0);
	}
}

void ImageViewerApp::onStop() {
	// Drop cached image data on close
	if (!m_lvglPath.empty() && lv_image_cache_is_enabled()) {
		lv_image_cache_drop(m_lvglPath.c_str());
	}

	m_container = nullptr;
	m_image = nullptr;
	m_errorLabel = nullptr;
	m_filePath.clear();
	m_lvglPath.clear();
}

} // namespace System::Apps
