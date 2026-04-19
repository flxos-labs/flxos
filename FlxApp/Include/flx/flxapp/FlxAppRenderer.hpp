#pragma once

#include <flx/core/Value.hpp>

#include <cstdint>
#include <lvgl.h>
#include <memory>
#include <string>
#include <vector>

namespace flx::flxapp {

class FlxAppActionRunner;
class FlxAppState;

class FlxAppRenderer {
public:

	FlxAppRenderer(FlxAppState& state, FlxAppActionRunner& actionRunner);

	void render(void* parent, const flx::core::FlxValueView& ui);
	void refreshBindings();

private:

	struct TextBinding {
		_lv_obj_t* label = nullptr;
		std::string textTemplate {};
	};

	struct ImageBinding {
		_lv_obj_t* image = nullptr;
		std::string srcTemplate {};
	};

	struct TextInputBinding {
		_lv_obj_t* textarea = nullptr;
		std::string key {};
	};

	struct ToggleBinding {
		_lv_obj_t* obj = nullptr;
		std::string key {};
	};

	struct SliderBinding {
		_lv_obj_t* slider = nullptr;
		std::string key {};
		int32_t minValue = 0;
		int32_t maxValue = 100;
	};

	struct DropdownBinding {
		_lv_obj_t* dropdown = nullptr;
		std::string key {};
	};

	struct BarBinding {
		_lv_obj_t* bar = nullptr;
		std::string key {};
		int32_t minValue = 0;
		int32_t maxValue = 100;
	};

	struct ListButtonTextBinding {
		_lv_obj_t* list = nullptr;
		_lv_obj_t* button = nullptr;
		std::string textTemplate {};
	};

	struct EventBinding {
		enum class Kind {
			ButtonClick,
			TextInputChanged,
			SwitchChanged,
			SliderChanged,
			CheckboxChanged,
			DropdownChanged,
			ListItemClicked,
		};

		FlxAppRenderer* renderer = nullptr;
		FlxAppActionRunner* actionRunner = nullptr;
		FlxAppState* state = nullptr;
		std::string key {};
		Kind kind = Kind::ButtonClick;
		flx::core::FlxValueView actions {};
	};

	FlxAppState& m_state;
	FlxAppActionRunner& m_actionRunner;
	std::vector<TextBinding> m_textBindings {};
	std::vector<ImageBinding> m_imageBindings {};
	std::vector<TextInputBinding> m_textInputBindings {};
	std::vector<ToggleBinding> m_switchBindings {};
	std::vector<SliderBinding> m_sliderBindings {};
	std::vector<ToggleBinding> m_checkboxBindings {};
	std::vector<DropdownBinding> m_dropdownBindings {};
	std::vector<BarBinding> m_barBindings {};
	std::vector<ListButtonTextBinding> m_listButtonTextBindings {};
	std::vector<std::unique_ptr<EventBinding>> m_eventBindings {};
	bool m_isRefreshing = false;

	lv_obj_t* renderNode(lv_obj_t* parent, const flx::core::FlxValueView& node);
	void configureLayout(lv_obj_t* obj, const std::string& type) const;
	void bindText(lv_obj_t* label, const std::string& textTemplate);
	void bindActions(lv_obj_t* obj,
		lv_event_code_t eventCode,
		const flx::core::FlxValueView& actions,
		EventBinding::Kind kind,
		const std::string& key = {});
	void handleBoundEvent(const EventBinding& binding, lv_event_t* event);

	std::string resolveText(const std::string& textTemplate) const;
	int32_t clampToRange(int32_t value, int32_t minValue, int32_t maxValue) const;

	static void applyCheckedState(lv_obj_t* obj, bool checked);
	static void onBoundEvent(lv_event_t* event);
};

} // namespace flx::flxapp
