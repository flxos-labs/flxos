#pragma once

namespace LayoutConstants {

// Panel & Window Sizes (Percentages)
static constexpr int PANEL_WIDTH_PCT = 80;
static constexpr int PANEL_HEIGHT_PCT = 60;
static constexpr int LIST_HEIGHT_PCT = 85;
static constexpr int MODAL_WIDTH_PCT = 80;
static constexpr int KEYBOARD_HEIGHT_PCT = 40;
static constexpr int BAR_WIDTH_PCT = 90;
static constexpr int INPUT_WIDTH_PCT = 90;
static constexpr int FILE_DIALOG_WIDTH_PCT = 90;
static constexpr int FILE_DIALOG_HEIGHT_PCT = 80;

// Dock
static constexpr int DOCK_BTN_WIDTH_PCT = 13;
static constexpr int DOCK_BTN_HEIGHT_PCT = 85;
static constexpr int DOCK_BTN_HEIGHT_SMALL_PCT = 80;

// Fixed Pixel Sizes (Logic Units - use with lv_dpx)
static constexpr int SIZE_TOUCH_TARGET = 30;
static constexpr int SIZE_DROPDOWN_WIDTH_SMALL = 80;
static constexpr int SIZE_DROPDOWN_WIDTH_LARGE = 125;
static constexpr int SIZE_DROPDOWN_HEIGHT = 35;
static constexpr int SIZE_DROPDOWN_BTN_WIDTH = 40;

// Rotations (0.1 degree units)
static constexpr int ROTATION_90_DEG = 900;

// Specific Margins/Paddings (Logic Units)
static constexpr int MARGIN_SECTION = 15;
static constexpr int PAD_CONTAINER = 10;

} // namespace LayoutConstants
