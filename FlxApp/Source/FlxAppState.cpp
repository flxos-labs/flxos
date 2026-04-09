#include <flx/flxapp/FlxAppState.hpp>

#include <cstdlib>
#include <cctype>
#include <limits>
#include <utility>

namespace flx::flxapp {

namespace {

int32_t toInt32Checked(int64_t value) {
    if (value > std::numeric_limits<int32_t>::max()) {
        return std::numeric_limits<int32_t>::max();
    }
    if (value < std::numeric_limits<int32_t>::min()) {
        return std::numeric_limits<int32_t>::min();
    }
    return static_cast<int32_t>(value);
}

} // namespace

void FlxAppState::loadFromValue(const flx::core::FlxValueView& variables) {
    m_values.clear();
    if (!variables.valid() || !variables.isMap()) {
        notifyChanged();
        return;
    }

	variables.forEachNamedChild([this](std::string_view key, const flx::core::FlxValueView& item) {
		if (!item.valid() || !item.hasValue() || item.isNull()) {
			return;
		}

		Value value {};
		if (item.isBoolScalar()) {
			value.type = Value::Type::Boolean;
			value.boolValue = item.asBool();
        } else if (item.isIntScalar()) {
            value.type = Value::Type::Integer;
            value.intValue = toInt32Checked(item.asInt64());
		} else {
			value.type = Value::Type::String;
			value.stringValue = item.asString();
		}

		m_values[std::string(key)] = std::move(value);
	});

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

void FlxAppState::setFromValue(const std::string& key, const flx::core::FlxValueView& value) {
    Value item;
    if (value.valid() && value.hasValue() && !value.isNull() && value.isBoolScalar()) {
        item.type = Value::Type::Boolean;
        item.boolValue = value.asBool();
    } else if (value.valid() && value.hasValue() && !value.isNull() && value.isIntScalar()) {
        item.type = Value::Type::Integer;
        item.intValue = toInt32Checked(value.asInt64());
    } else {
        item.type = Value::Type::String;
        item.stringValue = value.asString();
    }

    m_values[key] = std::move(item);

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
