#pragma once

#include <flx/core/Value.hpp>

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

    struct ButtonBinding {
        FlxAppActionRunner* actionRunner = nullptr;
        flx::core::FlxValueView actions {};
    };

    FlxAppState& m_state;
    FlxAppActionRunner& m_actionRunner;
    std::vector<TextBinding> m_textBindings {};
    std::vector<std::unique_ptr<ButtonBinding>> m_buttonBindings {};

    lv_obj_t* renderNode(lv_obj_t* parent, const flx::core::FlxValueView& node);
    void configureLayout(lv_obj_t* obj, const std::string& type) const;
    void bindText(lv_obj_t* label, const std::string& textTemplate);
    static void onButtonClicked(lv_event_t* event);
};

} // namespace flx::flxapp
