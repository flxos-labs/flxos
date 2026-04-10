#pragma once

#include <flx/core/Value.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace flx::flxapp {

class FlxAppState {
public:

    using ChangeCallback = std::function<void()>;

    void loadFromValue(const flx::core::FlxValueView& variables);
    std::string resolve(const std::string& text) const;

    bool has(const std::string& key) const;
    std::string getString(const std::string& key) const;
    bool getBool(const std::string& key) const;
    int32_t getInt(const std::string& key) const;

    void setString(const std::string& key, const std::string& value);
    void setBool(const std::string& key, bool value);
    void setInt(const std::string& key, int32_t value);
    void setFromValue(const std::string& key, const flx::core::FlxValueView& value);
    void increment(const std::string& key, int32_t delta = 1);
    void toggle(const std::string& key);

    void setChangeCallback(ChangeCallback callback);

private:

    struct Value {
        enum class Type {
            String,
            Integer,
            Boolean,
        };

        Type type = Type::String;
        std::string stringValue {};
        int32_t intValue = 0;
        bool boolValue = false;
    };

    std::unordered_map<std::string, Value> m_values {};
    ChangeCallback m_changeCallback {};

    void notifyChanged();
    static std::string trim(const std::string& value);
};

} // namespace flx::flxapp
