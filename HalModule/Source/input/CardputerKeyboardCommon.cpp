#include <flx/hal/input/CardputerKeyboardCommon.hpp>

namespace flx::hal::input {

// HID key codes mapped in standard keymap.h
#define KEY_BACKSPACE 0x2a
#define KEY_ENTER 0x28
#define KEY_SPACE 0x2c
#define KEY_TAB 0x2b
#define KEY_SEMICOLON 0x33
#define KEY_DOT 0x37
#define KEY_COMMA 0x36
#define KEY_SLASH 0x38
#define KEY_GRAVE 0x35
#define KEY_APOSTROPHE 0x34
#define KEY_MINUS 0x2d
#define KEY_EQUAL 0x2e
#define KEY_LEFTBRACE 0x2f
#define KEY_RIGHTBRACE 0x30
#define KEY_BACKSLASH 0x31

#define KEY_1 0x1e
#define KEY_2 0x1f
#define KEY_3 0x20
#define KEY_4 0x21
#define KEY_5 0x22
#define KEY_6 0x23
#define KEY_7 0x24
#define KEY_8 0x25
#define KEY_9 0x26
#define KEY_0 0x27

#define KEY_Q 0x14
#define KEY_W 0x1a
#define KEY_E 0x08
#define KEY_R 0x15
#define KEY_T 0x17
#define KEY_Y 0x1c
#define KEY_U 0x18
#define KEY_I 0x0c
#define KEY_O 0x12
#define KEY_P 0x13

#define KEY_A 0x04
#define KEY_S 0x16
#define KEY_D 0x07
#define KEY_F 0x09
#define KEY_G 0x0a
#define KEY_H 0x0b
#define KEY_J 0x0d
#define KEY_K 0x0e
#define KEY_L 0x0f

#define KEY_Z 0x1d
#define KEY_X 0x1b
#define KEY_C 0x06
#define KEY_V 0x19
#define KEY_B 0x05
#define KEY_N 0x11
#define KEY_M 0x10

#define KEY_LEFTCTRL 0xe0
#define KEY_LEFTALT 0xe2
#define KEY_KPASTERISK 0x55
#define KEY_KPLEFTPAREN 0xb6
#define KEY_KPRIGHTPAREN 0xb7
#define KEY_KPMINUS 0x56
#define KEY_KPPLUS 0x57
#define KEY_KPSLASH 0x54

const KeyValue_t CardputerKeyValueMap[4][14] = {
	{{"`", KEY_GRAVE, "~", KEY_GRAVE},
		{"1", KEY_1, "!", KEY_1},
		{"2", KEY_2, "@", KEY_2},
		{"3", KEY_3, "#", KEY_3},
		{"4", KEY_4, "$", KEY_4},
		{"5", KEY_5, "%", KEY_5},
		{"6", KEY_6, "^", KEY_6},
		{"7", KEY_7, "&", KEY_7},
		{"8", KEY_8, "*", KEY_KPASTERISK},
		{"9", KEY_9, "(", KEY_KPLEFTPAREN},
		{"0", KEY_0, ")", KEY_KPRIGHTPAREN},
		{"-", KEY_MINUS, "_", KEY_KPMINUS},
		{"=", KEY_EQUAL, "+", KEY_KPPLUS},
		{"del", KEY_BACKSPACE, "del", KEY_BACKSPACE}},
	{{"tab", KEY_TAB, "tab", KEY_TAB},
		{"q", KEY_Q, "Q", KEY_Q},
		{"w", KEY_W, "W", KEY_W},
		{"e", KEY_E, "E", KEY_E},
		{"r", KEY_R, "R", KEY_R},
		{"t", KEY_T, "T", KEY_T},
		{"y", KEY_Y, "Y", KEY_Y},
		{"u", KEY_U, "U", KEY_U},
		{"i", KEY_I, "I", KEY_I},
		{"o", KEY_O, "O", KEY_O},
		{"p", KEY_P, "P", KEY_P},
		{"[", KEY_LEFTBRACE, "{", KEY_LEFTBRACE},
		{"]", KEY_RIGHTBRACE, "}", KEY_RIGHTBRACE},
		{"\\", KEY_BACKSLASH, "|", KEY_BACKSLASH}},
	{{"fn", 0, "fn", 0},
		{"shift", 0, "shift", 0},
		{"a", KEY_A, "A", KEY_A},
		{"s", KEY_S, "S", KEY_S},
		{"d", KEY_D, "D", KEY_D},
		{"f", KEY_F, "F", KEY_F},
		{"g", KEY_G, "G", KEY_G},
		{"h", KEY_H, "H", KEY_H},
		{"j", KEY_J, "J", KEY_J},
		{"k", KEY_K, "K", KEY_K},
		{"l", KEY_L, "L", KEY_L},
		{";", KEY_SEMICOLON, ":", KEY_SEMICOLON},
		{"'", KEY_APOSTROPHE, "\"", KEY_APOSTROPHE},
		{"enter", KEY_ENTER, "enter", KEY_ENTER}},
	{{"ctrl", KEY_LEFTCTRL, "ctrl", KEY_LEFTCTRL},
		{"opt", 0, "opt", 0},
		{"alt", KEY_LEFTALT, "alt", KEY_LEFTALT},
		{"z", KEY_Z, "Z", KEY_Z},
		{"x", KEY_X, "X", KEY_X},
		{"c", KEY_C, "C", KEY_C},
		{"v", KEY_V, "V", KEY_V},
		{"b", KEY_B, "B", KEY_B},
		{"n", KEY_N, "N", KEY_N},
		{"m", KEY_M, "M", KEY_M},
		{",", KEY_COMMA, "<", KEY_COMMA},
		{".", KEY_DOT, ">", KEY_DOT},
		{"/", KEY_KPSLASH, "?", KEY_KPSLASH},
		{"space", KEY_SPACE, "space", KEY_SPACE}}};

} // namespace flx::hal::input
