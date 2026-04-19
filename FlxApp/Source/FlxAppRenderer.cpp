#include <flx/flxapp/FlxAppRenderer.hpp>

#include <flx/flxapp/FlxAppActionRunner.hpp>
#include <flx/flxapp/FlxAppState.hpp>
#include <flx/flxapp/NumberUtils.hpp>
#include <lvgl.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <limits>

namespace flx::flxapp {

namespace {

std::string valueString(const flx::core::FlxValueView& object, const char* key, const char* fallback = "") {
	if (key == nullptr) {
		return fallback;
	}
	return object.child(key).asString(fallback);
}

int32_t parseInt(const std::string& text, int32_t fallback) {
	if (text.empty()) {
		return fallback;
	}

	errno = 0;
	char* end = nullptr;
	const long parsed = std::strtol(text.c_str(), &end, 10);
	if (end == text.c_str()) {
		return fallback;
	}

	while (end != nullptr && *end != '\0') {
		if (std::isspace(static_cast<unsigned char>(*end)) == 0) {
			return fallback;
		}
		++end;
	}

	if (errno == ERANGE) {
		if (parsed < 0) {
			return std::numeric_limits<int32_t>::min();
		}
		return std::numeric_limits<int32_t>::max();
	}

	return flx::flxapp::detail::clampInt64ToInt32(static_cast<int64_t>(parsed));
}

int32_t valueInt(const flx::core::FlxValueView& object, const char* key, int32_t fallback) {
	if (key == nullptr) {
		return fallback;
	}

	const flx::core::FlxValueView value = object.child(key);
	if (!value.valid() || !value.hasValue() || value.isNull()) {
		return fallback;
	}

	if (value.isIntScalar()) {
		return flx::flxapp::detail::clampInt64ToInt32(value.asInt64());
	}

	return parseInt(value.asString(), fallback);
}

bool equalsIgnoreCase(const std::string& left, const std::string& right) {
	if (left.size() != right.size()) {
		return false;
	}

	for (size_t index = 0; index < left.size(); ++index) {
		if (std::tolower(static_cast<unsigned char>(left[index])) !=
			std::tolower(static_cast<unsigned char>(right[index]))) {
			return false;
		}
	}

	return true;
}

bool valueBool(const flx::core::FlxValueView& object, const char* key, bool fallback) {
	if (key == nullptr) {
		return fallback;
	}

	const flx::core::FlxValueView value = object.child(key);
	if (!value.valid() || !value.hasValue() || value.isNull()) {
		return fallback;
	}

	if (value.isBoolScalar()) {
		return value.asBool(fallback);
	}

	if (value.isIntScalar()) {
		return value.asInt64() != 0;
	}

	const std::string text = value.asString();
	if (equalsIgnoreCase(text, "true") ||
		equalsIgnoreCase(text, "yes") ||
		equalsIgnoreCase(text, "on") ||
		text == "1") {
		return true;
	}

	if (equalsIgnoreCase(text, "false") ||
		equalsIgnoreCase(text, "no") ||
		equalsIgnoreCase(text, "off") ||
		text == "0") {
		return false;
	}

	return fallback;
}

std::string buildDropdownOptions(const FlxAppState& state, const flx::core::FlxValueView& optionsNode) {
	if (!optionsNode.valid()) {
		return {};
	}

	if (!optionsNode.isSeq()) {
		return state.resolve(optionsNode.asString());
	}

	std::string options;
	bool first = true;
	optionsNode.forEachChild([&state, &options, &first](const flx::core::FlxValueView& option) {
		std::string text;
		if (option.isMap()) {
			text = option.child("label").asString();
			if (text.empty()) {
				text = option.child("text").asString();
			}
		} else {
			text = option.asString();
		}

		if (!first) {
			options.push_back('\n');
		}
		first = false;
		options += state.resolve(text);
	});

	return options;
}

} // namespace

FlxAppRenderer::FlxAppRenderer(FlxAppState& state, FlxAppActionRunner& actionRunner)
	: m_state(state), m_actionRunner(actionRunner) {}

void FlxAppRenderer::render(void* parent, const flx::core::FlxValueView& ui) {
	m_textBindings.clear();
	m_imageBindings.clear();
	m_textInputBindings.clear();
	m_switchBindings.clear();
	m_sliderBindings.clear();
	m_checkboxBindings.clear();
	m_dropdownBindings.clear();
	m_barBindings.clear();
	m_listButtonTextBindings.clear();
	m_eventBindings.clear();

	auto* root = static_cast<lv_obj_t*>(parent);
	if (root == nullptr || !ui.valid()) {
		return;
	}

	lv_obj_set_layout(root, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_width(root, lv_pct(100));
	lv_obj_set_height(root, lv_pct(100));

	renderNode(root, ui);
	refreshBindings();
}

void FlxAppRenderer::refreshBindings() {
	m_isRefreshing = true;

	for (const auto& binding: m_textBindings) {
		if (binding.label != nullptr) {
			lv_label_set_text(binding.label, resolveText(binding.textTemplate).c_str());
		}
	}

	for (const auto& binding: m_imageBindings) {
		if (binding.image != nullptr) {
			const std::string source = resolveText(binding.srcTemplate);
			if (!source.empty()) {
				lv_image_set_src(binding.image, source.c_str());
			}
		}
	}

	for (const auto& binding: m_textInputBindings) {
		if (binding.textarea == nullptr || binding.key.empty()) {
			continue;
		}

		const std::string currentText = m_state.getString(binding.key);
		const char* existing = lv_textarea_get_text(binding.textarea);
		if (existing == nullptr || currentText != existing) {
			lv_textarea_set_text(binding.textarea, currentText.c_str());
		}
	}

	for (const auto& binding: m_switchBindings) {
		if (binding.obj != nullptr && !binding.key.empty()) {
			applyCheckedState(binding.obj, m_state.getBool(binding.key));
		}
	}

	for (const auto& binding: m_sliderBindings) {
		if (binding.slider == nullptr || binding.key.empty()) {
			continue;
		}

		const int32_t value = clampToRange(m_state.getInt(binding.key), binding.minValue, binding.maxValue);
		if (lv_slider_get_value(binding.slider) != value) {
			lv_slider_set_value(binding.slider, value, LV_ANIM_OFF);
		}
	}

	for (const auto& binding: m_checkboxBindings) {
		if (binding.obj != nullptr && !binding.key.empty()) {
			applyCheckedState(binding.obj, m_state.getBool(binding.key));
		}
	}

	for (const auto& binding: m_dropdownBindings) {
		if (binding.dropdown == nullptr || binding.key.empty()) {
			continue;
		}

		const uint32_t optionCount = lv_dropdown_get_option_count(binding.dropdown);
		if (optionCount == 0) {
			continue;
		}

		const int32_t selected =
			clampToRange(m_state.getInt(binding.key), 0, static_cast<int32_t>(optionCount - 1));
		if (lv_dropdown_get_selected(binding.dropdown) != static_cast<uint32_t>(selected)) {
			lv_dropdown_set_selected(binding.dropdown, static_cast<uint32_t>(selected));
		}
	}

	for (const auto& binding: m_barBindings) {
		if (binding.bar == nullptr || binding.key.empty()) {
			continue;
		}

		const int32_t value = clampToRange(m_state.getInt(binding.key), binding.minValue, binding.maxValue);
		lv_bar_set_value(binding.bar, value, LV_ANIM_OFF);
	}

	for (const auto& binding: m_listButtonTextBindings) {
		if (binding.list == nullptr || binding.button == nullptr) {
			continue;
		}

		const std::string text = resolveText(binding.textTemplate);
		lv_list_set_button_text(binding.list, binding.button, text.c_str());
	}

	m_isRefreshing = false;
}

lv_obj_t* FlxAppRenderer::renderNode(lv_obj_t* parent, const flx::core::FlxValueView& node) {
	if (!node.valid() || !node.isMap()) {
		return nullptr;
	}

	const std::string type = valueString(node, "type", "column");
	lv_obj_t* obj = nullptr;

	if (type == "column" || type == "row") {
		obj = lv_obj_create(parent);
		lv_obj_set_width(obj, lv_pct(100));
		lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
		configureLayout(obj, type);

		const flx::core::FlxValueView children = node.child("children");
		if (children.valid()) {
			children.forEachChild([this, obj](const flx::core::FlxValueView& child) {
				renderNode(obj, child);
			});
		}
		return obj;
	}

	if (type == "label") {
		obj = lv_label_create(parent);
		bindText(obj, valueString(node, "text"));
		return obj;
	}

	if (type == "button") {
		obj = lv_button_create(parent);
		lv_obj_set_width(obj, lv_pct(100));

		lv_obj_t* label = lv_label_create(obj);
		bindText(label, valueString(node, "text", "Button"));
		lv_obj_center(label);

		bindActions(obj, LV_EVENT_CLICKED, node.child("on_click"), EventBinding::Kind::ButtonClick);
		return obj;
	}

	if (type == "spacer") {
		obj = lv_obj_create(parent);
		lv_obj_remove_style_all(obj);
		lv_obj_set_width(obj, lv_pct(100));
		lv_obj_set_height(obj, lv_dpx(valueInt(node, "height", 12)));
		return obj;
	}

	if (type == "text_input" || type == "textarea") {
		obj = lv_textarea_create(parent);
		lv_obj_set_width(obj, lv_pct(100));
		lv_textarea_set_one_line(obj, valueBool(node, "one_line", false));
		lv_textarea_set_password_mode(obj, valueBool(node, "password", false));

		const std::string placeholder = valueString(node, "placeholder");
		if (!placeholder.empty()) {
			lv_textarea_set_placeholder_text(obj, resolveText(placeholder).c_str());
		}

		const std::string key = valueString(node, "key");
		const std::string initial = key.empty() ? resolveText(valueString(node, "value")) : m_state.getString(key);
		lv_textarea_set_text(obj, initial.c_str());

		if (!key.empty()) {
			m_textInputBindings.push_back(TextInputBinding {obj, key});
		}

		bindActions(obj,
			LV_EVENT_VALUE_CHANGED,
			node.child("on_change"),
			EventBinding::Kind::TextInputChanged,
			key);
		return obj;
	}

	if (type == "switch") {
		obj = lv_switch_create(parent);

		const std::string key = valueString(node, "key");
		const bool initial = key.empty() ? valueBool(node, "value", false) : m_state.getBool(key);
		applyCheckedState(obj, initial);

		if (!key.empty()) {
			m_switchBindings.push_back(ToggleBinding {obj, key});
		}

		bindActions(obj,
			LV_EVENT_VALUE_CHANGED,
			node.child("on_change"),
			EventBinding::Kind::SwitchChanged,
			key);
		return obj;
	}

	if (type == "slider") {
		obj = lv_slider_create(parent);
		lv_obj_set_width(obj, lv_pct(100));

		int32_t minValue = valueInt(node, "min", 0);
		int32_t maxValue = valueInt(node, "max", 100);
		if (maxValue < minValue) {
			std::swap(minValue, maxValue);
		}

		lv_slider_set_range(obj, minValue, maxValue);

		const std::string key = valueString(node, "key");
		int32_t initialValue = key.empty() ? valueInt(node, "value", minValue) : m_state.getInt(key);
		initialValue = clampToRange(initialValue, minValue, maxValue);
		lv_slider_set_value(obj, initialValue, LV_ANIM_OFF);

		if (!key.empty()) {
			m_sliderBindings.push_back(SliderBinding {obj, key, minValue, maxValue});
		}

		bindActions(obj,
			LV_EVENT_VALUE_CHANGED,
			node.child("on_change"),
			EventBinding::Kind::SliderChanged,
			key);
		return obj;
	}

	if (type == "checkbox") {
		obj = lv_checkbox_create(parent);

		const std::string textTemplate = valueString(node, "text");
		if (!textTemplate.empty()) {
			lv_checkbox_set_text(obj, resolveText(textTemplate).c_str());
		}

		const std::string key = valueString(node, "key");
		const bool initial = key.empty() ? valueBool(node, "value", false) : m_state.getBool(key);
		applyCheckedState(obj, initial);

		if (!key.empty()) {
			m_checkboxBindings.push_back(ToggleBinding {obj, key});
		}

		bindActions(obj,
			LV_EVENT_VALUE_CHANGED,
			node.child("on_change"),
			EventBinding::Kind::CheckboxChanged,
			key);
		return obj;
	}

	if (type == "dropdown") {
		obj = lv_dropdown_create(parent);
		lv_obj_set_width(obj, lv_pct(100));

		std::string options = buildDropdownOptions(m_state, node.child("options"));
		if (options.empty()) {
			options = "Option";
		}
		lv_dropdown_set_options(obj, options.c_str());

		const std::string key = valueString(node, "key");
		int32_t selected = key.empty() ? valueInt(node, "selected", 0) : m_state.getInt(key);
		const uint32_t optionCount = lv_dropdown_get_option_count(obj);
		if (optionCount > 0) {
			selected = clampToRange(selected, 0, static_cast<int32_t>(optionCount - 1));
			lv_dropdown_set_selected(obj, static_cast<uint32_t>(selected));
		}

		if (!key.empty()) {
			m_dropdownBindings.push_back(DropdownBinding {obj, key});
		}

		bindActions(obj,
			LV_EVENT_VALUE_CHANGED,
			node.child("on_change"),
			EventBinding::Kind::DropdownChanged,
			key);
		return obj;
	}

	if (type == "image") {
		obj = lv_image_create(parent);

		std::string srcTemplate = valueString(node, "src");
		if (srcTemplate.empty()) {
			srcTemplate = valueString(node, "path");
		}
		if (!srcTemplate.empty()) {
			lv_image_set_src(obj, resolveText(srcTemplate).c_str());
			m_imageBindings.push_back(ImageBinding {obj, srcTemplate});
		}

		const int32_t width = valueInt(node, "width", 0);
		if (width > 0) {
			lv_obj_set_width(obj, lv_dpx(width));
		}

		const int32_t height = valueInt(node, "height", 0);
		if (height > 0) {
			lv_obj_set_height(obj, lv_dpx(height));
		}

		const std::string mode = valueString(node, "mode", "contain");
		if (mode == "cover") {
			lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_COVER);
		} else {
			lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_CONTAIN);
		}
		return obj;
	}

	if (type == "list") {
		obj = lv_list_create(parent);
		lv_obj_set_width(obj, lv_pct(100));
		lv_obj_set_height(obj, lv_dpx(valueInt(node, "height", 180)));

		const flx::core::FlxValueView items = node.child("items");
		if (items.valid() && items.isSeq()) {
			items.forEachChild([this, obj](const flx::core::FlxValueView& item) {
				if (!item.valid()) {
					return;
				}

				if (!item.isMap()) {
					const std::string textTemplate = item.asString();
					lv_obj_t* textObj = lv_list_add_text(obj, resolveText(textTemplate).c_str());
					m_textBindings.push_back(TextBinding {textObj, textTemplate});
					return;
				}

				const bool isHeader = valueBool(item, "header", false) ||
					valueString(item, "kind") == "text" ||
					valueString(item, "type") == "text";
				const std::string textTemplate = valueString(item, "text");
				if (isHeader) {
					lv_obj_t* textObj = lv_list_add_text(obj, resolveText(textTemplate).c_str());
					m_textBindings.push_back(TextBinding {textObj, textTemplate});
					return;
				}

				const std::string icon = valueString(item, "icon");
				lv_obj_t* button = lv_list_add_button(
					obj,
					icon.empty() ? nullptr : icon.c_str(),
					resolveText(textTemplate).c_str());
				m_listButtonTextBindings.push_back(ListButtonTextBinding {obj, button, textTemplate});
				bindActions(button,
					LV_EVENT_CLICKED,
					item.child("on_click"),
					EventBinding::Kind::ListItemClicked);
			});
		}
		return obj;
	}

	if (type == "bar") {
		obj = lv_bar_create(parent);
		lv_obj_set_width(obj, lv_pct(100));

		int32_t minValue = valueInt(node, "min", 0);
		int32_t maxValue = valueInt(node, "max", 100);
		if (maxValue < minValue) {
			std::swap(minValue, maxValue);
		}
		lv_bar_set_range(obj, minValue, maxValue);

		const std::string key = valueString(node, "key");
		int32_t initialValue = key.empty() ? valueInt(node, "value", minValue) : m_state.getInt(key);
		initialValue = clampToRange(initialValue, minValue, maxValue);
		lv_bar_set_value(obj, initialValue, LV_ANIM_OFF);

		if (!key.empty()) {
			m_barBindings.push_back(BarBinding {obj, key, minValue, maxValue});
		}
		return obj;
	}

	obj = lv_label_create(parent);
	lv_label_set_text(obj, type.c_str());
	return obj;
}

void FlxAppRenderer::configureLayout(lv_obj_t* obj, const std::string& type) const {
	lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
	lv_obj_set_style_pad_all(obj, lv_dpx(8), 0);
	lv_obj_set_style_pad_row(obj, lv_dpx(8), 0);
	lv_obj_set_style_pad_column(obj, lv_dpx(8), 0);
	lv_obj_set_style_border_width(obj, 0, 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
	lv_obj_set_flex_flow(obj, type == "row" ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
}

void FlxAppRenderer::bindText(lv_obj_t* label, const std::string& textTemplate) {
	m_textBindings.push_back(TextBinding {label, textTemplate});
}

void FlxAppRenderer::bindActions(lv_obj_t* obj,
	lv_event_code_t eventCode,
	const flx::core::FlxValueView& actions,
	EventBinding::Kind kind,
	const std::string& key) {
	if (obj == nullptr || (!actions.valid() && key.empty())) {
		return;
	}

	auto binding = std::make_unique<EventBinding>();
	binding->renderer = this;
	binding->actionRunner = &m_actionRunner;
	binding->state = &m_state;
	binding->actions = actions;
	binding->key = key;
	binding->kind = kind;

	lv_obj_add_event_cb(obj, onBoundEvent, eventCode, binding.get());
	m_eventBindings.push_back(std::move(binding));
}

void FlxAppRenderer::handleBoundEvent(const EventBinding& binding, lv_event_t* event) {
	if (m_isRefreshing) {
		return;
	}

	auto* target = lv_event_get_target_obj(event);
	if (target == nullptr) {
		return;
	}

	if (binding.state != nullptr && !binding.key.empty()) {
		switch (binding.kind) {
			case EventBinding::Kind::TextInputChanged:
				binding.state->setString(binding.key, lv_textarea_get_text(target));
				break;
			case EventBinding::Kind::SwitchChanged:
			case EventBinding::Kind::CheckboxChanged:
				binding.state->setBool(binding.key, lv_obj_has_state(target, LV_STATE_CHECKED));
				break;
			case EventBinding::Kind::SliderChanged:
				binding.state->setInt(binding.key, lv_slider_get_value(target));
				break;
			case EventBinding::Kind::DropdownChanged: {
				const uint32_t selected = lv_dropdown_get_selected(target);
				if (selected > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
					binding.state->setInt(binding.key, std::numeric_limits<int32_t>::max());
				} else {
					binding.state->setInt(binding.key, static_cast<int32_t>(selected));
				}
				break;
			}
			case EventBinding::Kind::ButtonClick:
			case EventBinding::Kind::ListItemClicked:
				break;
		}
	}

	if (binding.actionRunner != nullptr && binding.actions.valid()) {
		binding.actionRunner->run(binding.actions);
	}
}

std::string FlxAppRenderer::resolveText(const std::string& textTemplate) const {
	return m_state.resolve(textTemplate);
}

int32_t FlxAppRenderer::clampToRange(int32_t value, int32_t minValue, int32_t maxValue) const {
	if (maxValue < minValue) {
		std::swap(minValue, maxValue);
	}

	return std::clamp(value, minValue, maxValue);
}

void FlxAppRenderer::applyCheckedState(lv_obj_t* obj, bool checked) {
	if (obj == nullptr) {
		return;
	}

	if (checked) {
		lv_obj_add_state(obj, LV_STATE_CHECKED);
	} else {
		lv_obj_remove_state(obj, LV_STATE_CHECKED);
	}
}

void FlxAppRenderer::onBoundEvent(lv_event_t* event) {
	auto* binding = static_cast<EventBinding*>(lv_event_get_user_data(event));
	if (binding == nullptr) {
		return;
	}

	if (binding->renderer != nullptr) {
		binding->renderer->handleBoundEvent(*binding, event);
	}
}

} // namespace flx::flxapp
