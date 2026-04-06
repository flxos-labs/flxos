#include <flx/flxapp/FlxAppRenderer.hpp>

#include <cJSON.h>
#include <flx/flxapp/FlxAppActionRunner.hpp>
#include <flx/flxapp/FlxAppState.hpp>
#include <lvgl.h>

namespace flx::flxapp {

namespace {

std::string jsonString(const cJSON* object, const char* key, const char* fallback = "") {
    if (!cJSON_IsObject(object)) {
        return fallback;
    }

    const cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
        return item->valuestring;
    }

    return fallback;
}

} // namespace

FlxAppRenderer::FlxAppRenderer(FlxAppState& state, FlxAppActionRunner& actionRunner)
    : m_state(state), m_actionRunner(actionRunner) {}

void FlxAppRenderer::render(void* parent, const cJSON* ui) {
    m_textBindings.clear();
    m_buttonBindings.clear();

    auto* root = static_cast<lv_obj_t*>(parent);
    if (root == nullptr || ui == nullptr) {
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
    for (const auto& binding: m_textBindings) {
        if (binding.label != nullptr) {
            lv_label_set_text(binding.label, m_state.resolve(binding.textTemplate).c_str());
        }
    }
}

lv_obj_t* FlxAppRenderer::renderNode(lv_obj_t* parent, const cJSON* node) {
    if (!cJSON_IsObject(node)) {
        return nullptr;
    }

    const std::string type = jsonString(node, "type", "column");
    lv_obj_t* obj = nullptr;

    if (type == "column" || type == "row") {
        obj = lv_obj_create(parent);
        lv_obj_set_width(obj, lv_pct(100));
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        configureLayout(obj, type);

        const cJSON* children = cJSON_GetObjectItemCaseSensitive(node, "children");
        if (cJSON_IsArray(children)) {
            const cJSON* child = nullptr;
            cJSON_ArrayForEach(child, children) {
                renderNode(obj, child);
            }
        }
        return obj;
    }

    if (type == "label") {
        obj = lv_label_create(parent);
        bindText(obj, jsonString(node, "text"));
        return obj;
    }

    if (type == "button") {
        obj = lv_button_create(parent);
        lv_obj_set_width(obj, lv_pct(100));

        lv_obj_t* label = lv_label_create(obj);
        bindText(label, jsonString(node, "text", "Button"));
        lv_obj_center(label);

        const cJSON* onClick = cJSON_GetObjectItemCaseSensitive(node, "on_click");
        if (onClick != nullptr) {
            auto binding = std::make_unique<ButtonBinding>();
            binding->actionRunner = &m_actionRunner;
            binding->actions = onClick;
            lv_obj_add_event_cb(obj, onButtonClicked, LV_EVENT_CLICKED, binding.get());
            m_buttonBindings.push_back(std::move(binding));
        }
        return obj;
    }

    if (type == "spacer") {
        obj = lv_obj_create(parent);
        lv_obj_remove_style_all(obj);
        lv_obj_set_width(obj, lv_pct(100));
        lv_obj_set_height(obj, lv_dpx(12));
        return obj;
    }

    obj = lv_label_create(parent);
    lv_label_set_text(obj, type.c_str());
    return obj;
}

void FlxAppRenderer::configureLayout(lv_obj_t* obj, const std::string& type) const {
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_style_pad_all(obj, 8, 0);
    lv_obj_set_style_pad_row(obj, 8, 0);
    lv_obj_set_style_pad_column(obj, 8, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(obj, type == "row" ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
}

void FlxAppRenderer::bindText(lv_obj_t* label, const std::string& textTemplate) {
    m_textBindings.push_back(TextBinding {label, textTemplate});
}

void FlxAppRenderer::onButtonClicked(lv_event_t* event) {
    auto* binding = static_cast<ButtonBinding*>(lv_event_get_user_data(event));
    if (binding == nullptr) {
        return;
    }

    if (binding->actionRunner != nullptr) {
        binding->actionRunner->run(binding->actions);
    }
}

} // namespace flx::flxapp
