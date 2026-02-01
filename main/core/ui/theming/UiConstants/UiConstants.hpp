#pragma once

namespace UiConstants {
// Padding and Gaps
static constexpr int PAD_TINY = 2;
static constexpr int PAD_SMALL = 4;
static constexpr int PAD_MEDIUM = 5;
static constexpr int PAD_DEFAULT = 8;
static constexpr int PAD_LARGE = 10;

// Radii
static constexpr int RADIUS_SMALL = 6;
static constexpr int RADIUS_DEFAULT = 8;
static constexpr int RADIUS_LARGE = 10;

// Sizes
static constexpr int SIZE_HEADER = 30;
static constexpr int SIZE_TAB_BAR = 40;
static constexpr int SIZE_BAR_HEIGHT = 20;
static constexpr int SIZE_SWIPE_ZONE = 30;
static constexpr int SIZE_ICON_OVERLAP = 12;

// Glass Blur Defaults
static constexpr int GLASS_BLUR_SMALL = 4;
static constexpr int GLASS_BLUR_DEFAULT = 10;
static constexpr int GLASS_BLUR_LARGE = 15;

// Borders and Offsets
static constexpr int BORDER_THIN = 1;
static constexpr int BORDER_FOCUS = 2;
static constexpr int OFFSET_TINY = 2;

// Opacity
static constexpr int OPA_TRANSP = 0;
static constexpr int OPA_ITEM_BG = 51; // LV_OPA_20
static constexpr int OPA_GLASS_BG = 153; // LV_OPA_60
static constexpr int OPA_30 = 76; // LV_OPA_30
static constexpr int OPA_40 = 102; // LV_OPA_40
static constexpr int OPA_TEXT_DIM = 127; // LV_OPA_50
static constexpr int OPA_70 = 178; // LV_OPA_70
static constexpr int OPA_HIGH = 204; // LV_OPA_80
static constexpr int OPA_COVER = 255; // LV_OPA_COVER
} // namespace UiConstants
