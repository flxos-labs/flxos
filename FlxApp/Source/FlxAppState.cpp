#include <flx/flxapp/FlxAppState.hpp>

#include <cJSON.h>

#include <cctype>
#include <utility>

namespace flx::flxapp {

void FlxAppState::loadFromJson(const cJSON* variables) {
    m_values.clear();
    if (!cJSON_IsObject(variables)) {
        notifyChanged();
        return;
    }

    const cJSON* item = nullptr;
    cJSON_ArrayForEach(item, variables) {
        if (item->string == nullptr) {
            continue;
        }

        Value value {};
        if (cJSON_IsString(item) && item->valuestring != nullptr) {
            value.type = Value::Type::String;
            value.stringValue = item->valuestring;
        } else if (cJSON_IsBool(item)) {
            value.type = Value::Type::Boolean;
            value.boolValue = cJSON_IsTrue(item);
        } else if (cJSON_IsNumber(item)) {
            value.type = Value::Type::Integer;
            value.intValue = item->valueint;
        } else {
            continue;
        }

        m_values[item->string] = std::move(value);
    }

    notifyChanged();
}

std::string FlxAppState::resolve(const std::string& text) const {
    std::string output;
    output.reserve(text.size());

    std::size_t cursor = 0;
    while (cursor < text.size()) {
        std::size_t open = text.find("{{", cursor);
        if (open == std::string::npos) {
            output.append(text.substr(cursor));
            break;
        }

        output.append(text.substr(cursor, open - cursor));
        std::size_t close = text.find("}}", open + 2);
        if (close == std::string::npos) {
            output.append(text.substr(open));
            break;
        }

        const std::string key = trim(text.substr(open + 2, close - open - 2));
        output.append(getString(key));
        cursor = close + 2;
    }

    return output;
}

bool FlxAppState::has(const std::string& key) const {
    return m_values.find(key) != m_values.end();
}

std::string FlxAppState::getString(const std::string& key) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) {
        return {};
    }

    switch (it->second.type) {
        case Value::Type::String:
            return it->second.stringValue;
        case Value::Type::Integer:
            return std::to_string(it->second.intValue);
        case Value::Type::Boolean:
            return it->second.boolValue ? "true" : "false";
    }

    return {};
}

bool FlxAppState::getBool(const std::string& key) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) {
        return false;
    }

    if (it->second.type == Value::Type::Boolean) {
        return it->second.boolValue;
    }

    if (it->second.type == Value::Type::Integer) {
        return it->second.intValue != 0;
    }

    return it->second.stringValue == "true";
}

int32_t FlxAppState::getInt(const std::string& key) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) {
        return 0;
    }

    if (it->second.type == Value::Type::Integer) {
        return it->second.intValue;
    }

    if (it->second.type == Value::Type::Boolean) {
        return it->second.boolValue ? 1 : 0;
    }

    return std::atoi(it->second.stringValue.c_str());
}

void FlxAppState::setString(const std::string& key, const std::string& value) {
    Value item;
    item.type = Value::Type::String;
    item.stringValue = value;
    m_values[key] = item;
    notifyChanged();
}

void FlxAppState::setBool(const std::string& key, bool value) {
    Value item;
    item.type = Value::Type::Boolean;
    item.boolValue = value;
    m_values[key] = item;
    notifyChanged();
}

void FlxAppState::setInt(const std::string& key, int32_t value) {
    Value item;
    item.type = Value::Type::Integer;
    item.intValue = value;
    m_values[key] = item;
    notifyChanged();
}

void FlxAppState::setFromJson(const std::string& key, const cJSON* value) {
    if (cJSON_IsBool(value)) {
        Value item;
        item.type = Value::Type::Boolean;
        item.boolValue = cJSON_IsTrue(value);
        m_values[key] = item;
    } else if (cJSON_IsNumber(value)) {
        Value item;
        item.type = Value::Type::Integer;
        item.intValue = value->valueint;
        m_values[key] = item;
    } else if (cJSON_IsString(value) && value->valuestring != nullptr) {
        Value item;
        item.type = Value::Type::String;
        item.stringValue = value->valuestring;
        m_values[key] = item;
    } else {
        Value item;
        item.type = Value::Type::String;
        item.stringValue.clear();
        m_values[key] = item;
    }

    notifyChanged();
}

void FlxAppState::increment(const std::string& key, int32_t delta) {
    setInt(key, getInt(key) + delta);
}

void FlxAppState::toggle(const std::string& key) {
    setBool(key, !getBool(key));
}

void FlxAppState::setChangeCallback(ChangeCallback callback) {
    m_changeCallback = std::move(callback);
}

void FlxAppState::notifyChanged() {
    if (m_changeCallback) {
        m_changeCallback();
    }
}

std::string FlxAppState::trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

} // namespace flx::flxapp
