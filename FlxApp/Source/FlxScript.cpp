#include <flx/flxapp/FlxScript.hpp>

#include <flx/apps/AppManager.hpp>
#include <flx/apps/Intent.hpp>
#include <flx/core/Bundle.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/flxapp/FlxApp.hpp>
#include <flx/flxapp/FlxAppState.hpp>
#include <flx/flxapp/NumberUtils.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace flx::flxapp {

static const char* TAG = "FlxScript";

// =============================================================================
// Val helpers
// =============================================================================

bool FlxScript::Val::isTrue() const {
    switch (type) {
        case Type::Bool: return boolVal;
        case Type::Int:  return intVal != 0;
        case Type::Str:  return !strVal.empty() && strVal != "false" && strVal != "0";
    }
    return false;
}

int32_t FlxScript::Val::asInt() const {
    switch (type) {
        case Type::Int:  return intVal;
        case Type::Bool: return boolVal ? 1 : 0;
        case Type::Str:  return static_cast<int32_t>(std::atoi(strVal.c_str()));
    }
    return 0;
}

std::string FlxScript::Val::asString() const {
    switch (type) {
        case Type::Str:  return strVal;
        case Type::Int:  return std::to_string(intVal);
        case Type::Bool: return boolVal ? "true" : "false";
    }
    return {};
}

// =============================================================================
// Constructor
// =============================================================================

FlxScript::FlxScript(FlxAppState& state, FlxApp& app)
    : m_state(state), m_app(app) {}

// =============================================================================
// Static helpers
// =============================================================================

std::string_view FlxScript::trimView(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

bool FlxScript::startsWith(std::string_view sv, std::string_view prefix) {
    return sv.size() >= prefix.size() &&
           sv.compare(0, prefix.size(), prefix) == 0;
}

// =============================================================================
// Identifier resolution — reads from FlxAppState
// =============================================================================

FlxScript::Val FlxScript::resolveIdentifier(std::string_view name) {
    const std::string key(name);

    // true / false literals
    if (name == "true")  return { Val::Type::Bool, 0, {}, true };
    if (name == "false") return { Val::Type::Bool, 0, {}, false };

    if (!m_state.has(key)) {
        // Unknown identifier: treat as integer 0
        return { Val::Type::Int, 0, {}, false };
    }

    // Determine the stored type heuristically via getString — FlxAppState
    // provides typed accessors, so we pick the right one.
    const int32_t iv = m_state.getInt(key);
    const bool    bv = m_state.getBool(key);
    const std::string sv = m_state.getString(key);

    // If it parses cleanly as a number and getString()==to_string(iv), it's Int.
    const std::string asNum = std::to_string(iv);
    if (sv == asNum && sv != "true" && sv != "false") {
        return { Val::Type::Int, iv, {}, false };
    }
    if (sv == "true" || sv == "false") {
        return { Val::Type::Bool, 0, {}, bv };
    }
    return { Val::Type::Str, 0, sv, false };
}

// =============================================================================
// Argument parsing
// =============================================================================

// Trim outer quotes and return the inner text as a view.
std::string_view FlxScript::parseStringLiteral(std::string_view& cursor) {
    cursor = trimView(cursor);
    if (!cursor.empty() && cursor.front() == '"') {
        cursor.remove_prefix(1);
        auto end = cursor.find('"');
        if (end == std::string_view::npos) end = cursor.size();
        auto literal = cursor.substr(0, end);
        cursor.remove_prefix(end + (end < cursor.size() ? 1 : 0));
        return literal;
    }
    return {};
}

std::string FlxScript::evaluateArg(std::string_view arg) {
    arg = trimView(arg);
    if (arg.empty()) return {};

    // Evaluate as expression (handles string literals, identifiers, numbers, and arithmetic)
    return evalExpr(arg).asString();
}

// Split comma-separated args (respecting quotes), store in args[], return remainder.
// Returns the args view. args[] must have at least maxArgs entries.
std::string_view FlxScript::parseArgs(std::string_view argsRaw,
                                      std::string     args[],
                                      int             maxArgs,
                                      int&            outCount) {
    outCount = 0;
    argsRaw = trimView(argsRaw);

    while (!argsRaw.empty() && outCount < maxArgs) {
        // Find the next comma (not inside quotes)
        size_t i       = 0;
        bool   inQuote = false;
        for (; i < argsRaw.size(); ++i) {
            if (argsRaw[i] == '"') inQuote = !inQuote;
            if (!inQuote && argsRaw[i] == ',') break;
        }

        args[outCount++] = evaluateArg(argsRaw.substr(0, i));
        argsRaw = (i < argsRaw.size()) ? trimView(argsRaw.substr(i + 1)) : std::string_view{};
    }

    return argsRaw;
}

// =============================================================================
// Recursive-descent expression evaluator
// =============================================================================

// Entry point: evaluate a full expression string.
FlxScript::Val FlxScript::evalExpr(std::string_view expr) {
    expr = trimView(expr);
    return evalComparison(expr);
}

// Level 1 — comparison operators: ==  !=  <  <=  >  >=
FlxScript::Val FlxScript::evalComparison(std::string_view& cursor) {
    Val left = evalAddSub(cursor);
    cursor = trimView(cursor);

    struct Op { const char* sym; };
    // Order matters: check 2-char ops before 1-char ops.
    static constexpr const char* ops[] = { "==", "!=", "<=", ">=", "<", ">", nullptr };

    for (int i = 0; ops[i] != nullptr; ++i) {
        const std::string_view op(ops[i]);
        if (startsWith(cursor, op)) {
            cursor.remove_prefix(op.size());
            cursor = trimView(cursor);
            Val right = evalAddSub(cursor);

            bool result = false;
            if (op == "==") result = left.asInt() == right.asInt();
            else if (op == "!=") result = left.asInt() != right.asInt();
            else if (op == "<")  result = left.asInt() <  right.asInt();
            else if (op == "<=") result = left.asInt() <= right.asInt();
            else if (op == ">")  result = left.asInt() >  right.asInt();
            else if (op == ">=") result = left.asInt() >= right.asInt();

            return { Val::Type::Bool, 0, {}, result };
        }
    }

    return left;
}

// Level 2 — addition / subtraction / string concatenation.
FlxScript::Val FlxScript::evalAddSub(std::string_view& cursor) {
    Val left = evalAtom(cursor);
    cursor = trimView(cursor);

    while (!cursor.empty()) {
        char op = cursor.front();
        if (op != '+' && op != '-') break;

        cursor.remove_prefix(1);
        cursor = trimView(cursor);
        Val right = evalAtom(cursor);
        cursor = trimView(cursor);

        if (op == '+') {
            // If either side is a string, concatenate.
            if (left.type == Val::Type::Str || right.type == Val::Type::Str) {
                left = { Val::Type::Str, 0, left.asString() + right.asString(), false };
            } else {
                left = { Val::Type::Int, left.asInt() + right.asInt(), {}, false };
            }
        } else {
            left = { Val::Type::Int, left.asInt() - right.asInt(), {}, false };
        }
    }

    return left;
}

// Level 3 — atoms: literals, identifiers, parenthesised sub-expressions.
FlxScript::Val FlxScript::evalAtom(std::string_view& cursor) {
    cursor = trimView(cursor);
    if (cursor.empty()) return { Val::Type::Int, 0, {}, false };

    // String literal
    if (cursor.front() == '"') {
        cursor.remove_prefix(1);
        auto end = cursor.find('"');
        if (end == std::string_view::npos) end = cursor.size();
        std::string s(cursor.substr(0, end));
        cursor.remove_prefix(end + (end < cursor.size() ? 1 : 0));
        return { Val::Type::Str, 0, std::move(s), false };
    }

    // Parenthesised expression
    if (cursor.front() == '(') {
        cursor.remove_prefix(1);
        Val v = evalComparison(cursor);
        cursor = trimView(cursor);
        if (!cursor.empty() && cursor.front() == ')') cursor.remove_prefix(1);
        return v;
    }

    // Numeric literal (optional leading '-')
    bool negative = false;
    if (cursor.front() == '-' && cursor.size() > 1 && std::isdigit(static_cast<unsigned char>(cursor[1]))) {
        negative = true;
        cursor.remove_prefix(1);
    }
    if (std::isdigit(static_cast<unsigned char>(cursor.front()))) {
        size_t i = 0;
        while (i < cursor.size() && std::isdigit(static_cast<unsigned char>(cursor[i]))) ++i;
        int32_t n = static_cast<int32_t>(std::atoi(std::string(cursor.substr(0, i)).c_str()));
        cursor.remove_prefix(i);
        return { Val::Type::Int, negative ? -n : n, {}, false };
    }

    // Identifier
    size_t i = 0;
    while (i < cursor.size() && (std::isalnum(static_cast<unsigned char>(cursor[i])) || cursor[i] == '_')) {
        ++i;
    }
    if (i > 0) {
        auto name = cursor.substr(0, i);
        cursor.remove_prefix(i);
        return resolveIdentifier(name);
    }

    // Unknown character — skip it and return 0
    cursor.remove_prefix(1);
    return { Val::Type::Int, 0, {}, false };
}

// =============================================================================
// Assignment
// =============================================================================

bool FlxScript::executeAssignment(std::string_view lhs, std::string_view rhs) {
    lhs = trimView(lhs);
    rhs = trimView(rhs);

    if (lhs.empty()) {
        Log::warn(TAG, "FlxScript: empty assignment target");
        return false;
    }

    const std::string key(lhs);
    Val v = evalExpr(rhs);

    switch (v.type) {
        case Val::Type::Int:  m_state.setInt(key, v.intVal);       break;
        case Val::Type::Bool: m_state.setBool(key, v.boolVal);     break;
        case Val::Type::Str:  m_state.setString(key, v.strVal);    break;
    }
    return true;
}

// =============================================================================
// Built-in function calls
// =============================================================================

bool FlxScript::executeCall(std::string_view funcName, std::string_view argsRaw) {
    static constexpr int kMaxArgs = 6;
    std::string args[kMaxArgs];
    int         argc = 0;
    parseArgs(argsRaw, args, kMaxArgs, argc);

    if (funcName == "log") {
        Log::info(TAG, "%s", argc > 0 ? args[0].c_str() : "");

    } else if (funcName == "notify") {
        flx::core::Bundle data;
        data.putString("title",   argc > 0 ? args[0] : "");
        data.putString("message", argc > 1 ? args[1] : "");
        data.putString("appName", m_app.getAppName());
        data.putString("icon",    argc > 2 ? args[2] : "");
        if (argc > 3) {
            data.putBool("dismissable", args[3] == "false" || args[3] == "0" ? false : true);
        }
        if (argc > 4) {
            data.putString("id", args[4]);
        }
        flx::core::EventBus::getInstance().publish("system.notify", data);

    } else if (funcName == "remove_notify") {
        if (argc > 0) {
            flx::core::Bundle data;
            data.putString("id", args[0]);
            flx::core::EventBus::getInstance().publish("system.remove_notify", data);
        }

    } else if (funcName == "navigate") {
        if (argc > 0 && !args[0].empty()) {
            flx::apps::AppManager::getInstance().startApp(
                flx::apps::Intent::forApp(args[0]));
        }

    } else if (funcName == "event_publish") {
        if (argc > 0 && !args[0].empty()) {
            flx::core::Bundle data;
            if (argc > 1) data.putString("payload", args[1]);
            data.putString("appId", m_app.getPackageName());
            flx::core::EventBus::getInstance().publish(args[0], data);
        }

    } else if (funcName == "close") {
        flx::apps::AppManager::getInstance().stopApp(m_app.getPackageName());
        m_shouldStop = true;  // halt further execution this invocation

    } else if (funcName == "increment") {
        // increment(key) or increment(key, amount)
        if (argc > 0 && !args[0].empty()) {
            int32_t amount = (argc > 1) ? static_cast<int32_t>(std::atoi(args[1].c_str())) : 1;
            m_state.increment(args[0], amount);
        }

    } else if (funcName == "decrement") {
        if (argc > 0 && !args[0].empty()) {
            int32_t amount = (argc > 1) ? static_cast<int32_t>(std::atoi(args[1].c_str())) : 1;
            m_state.increment(args[0], -(amount < 0 ? -amount : amount));
        }

    } else if (funcName == "toggle") {
        if (argc > 0 && !args[0].empty()) {
            m_state.toggle(args[0]);
        }

    } else if (funcName == "set_string") {
        if (argc >= 2) m_state.setString(args[0], args[1]);

    } else {
        Log::warn(TAG, "FlxScript: unknown function '%.*s'",
                  static_cast<int>(funcName.size()), funcName.data());
    }

    return true;
}

// =============================================================================
// If / else / endif handling
// =============================================================================

// Called when we've parsed "if <condExpr> then" on the current line.
// `remainder` is everything after the "then" line (the rest of the script text).
// Scans remainder for matching else/endif, executes appropriate branch.
bool FlxScript::executeIf(std::string_view condExpr, std::string_view remainder) {
    // Evaluate the condition
    Val cond = evalExpr(condExpr);

    // Split remainder into then-block / else-block / tail.
    // We look for "else" and "endif" at the start of trimmed lines, tracking nesting depth.
    size_t elsPos  = std::string_view::npos;
    size_t endPos  = std::string_view::npos;

    size_t pos = 0;
    int depth = 0;
    while (pos < remainder.size()) {
        // Find next newline
        auto nl = remainder.find('\n', pos);
        auto lineEnd = (nl == std::string_view::npos) ? remainder.size() : nl;
        std::string_view line = trimView(remainder.substr(pos, lineEnd - pos));

        if (startsWith(line, "if ")) {
            ++depth;
        } else if (line == "else" || startsWith(line, "else ")) {
            if (depth == 0) {
                if (elsPos == std::string_view::npos) elsPos = pos;
            }
        } else if (line == "endif") {
            if (depth == 0) {
                endPos = pos;
                break;
            } else {
                --depth;
            }
        }

        pos = (nl == std::string_view::npos) ? remainder.size() : nl + 1;
    }

    if (endPos == std::string_view::npos) {
        Log::warn(TAG, "FlxScript: 'if' missing 'endif'");
        return false;
    }

    std::string_view thenBlock;
    std::string_view elseBlock;

    if (elsPos != std::string_view::npos) {
        thenBlock = remainder.substr(0, elsPos);
        // Skip "else" line
        auto nl = remainder.find('\n', elsPos);
        size_t elseBodyStart = (nl == std::string_view::npos) ? endPos : nl + 1;
        elseBlock = remainder.substr(elseBodyStart, endPos - elseBodyStart);
    } else {
        thenBlock = remainder.substr(0, endPos);
        elseBlock = {};
    }

    if (cond.isTrue()) {
        return executeBlock(thenBlock);
    } else if (!elseBlock.empty()) {
        return executeBlock(elseBlock);
    }

    return true;
}

// =============================================================================
// Single line execution
// =============================================================================

bool FlxScript::executeLine(std::string_view line) {
    line = trimView(line);

    // Empty line or comment
    if (line.empty() || line.front() == '#') return true;

    // Guard statement budget
    if (m_statementsExecuted >= kMaxStatements) {
        Log::warn(TAG, "FlxScript: statement limit (%u) reached — halting", kMaxStatements);
        return false;
    }
    ++m_statementsExecuted;

    // 'if <cond> then' — handled by caller (executeBlock) as a multi-line construct.
    // Should not reach here as a plain line.  If it does, warn and skip.
    if (startsWith(line, "if ")) {
        Log::warn(TAG, "FlxScript: bare 'if' reached single-line executor — check parser");
        return true;
    }

    // Function call: identifier(...)
    auto parenPos = line.find('(');
    if (parenPos != std::string_view::npos) {
        auto funcName = trimView(line.substr(0, parenPos));
        auto closePos = line.rfind(')');
        std::string_view argsRaw = (closePos != std::string_view::npos && closePos > parenPos)
            ? line.substr(parenPos + 1, closePos - parenPos - 1)
            : line.substr(parenPos + 1);
        return executeCall(funcName, argsRaw);
    }

    // Assignment: lhs = rhs
    auto eqPos = line.find('=');
    if (eqPos != std::string_view::npos) {
        // Make sure it's not == (comparison in a bare statement — weird, but skip safely)
        if (eqPos + 1 < line.size() && line[eqPos + 1] == '=') {
            Log::warn(TAG, "FlxScript: bare comparison statement ignored");
            return true;
        }
        auto lhs = line.substr(0, eqPos);
        auto rhs = line.substr(eqPos + 1);
        return executeAssignment(lhs, rhs);
    }

    Log::warn(TAG, "FlxScript: unrecognised statement: '%.*s'",
              static_cast<int>(line.size()), line.data());
    return true;
}

// =============================================================================
// Block execution — the core multi-line loop, handles if/else/endif.
// =============================================================================

bool FlxScript::executeBlock(std::string_view block) {
    size_t pos = 0;

    while (pos < block.size() && !m_shouldStop) {
        auto nl      = block.find('\n', pos);
        auto lineEnd = (nl == std::string_view::npos) ? block.size() : nl;
        std::string_view line = trimView(block.substr(pos, lineEnd - pos));

        pos = (nl == std::string_view::npos) ? block.size() : nl + 1;

        if (line.empty() || line.front() == '#') continue;
        // Skip 'else' and 'endif' — they are consumed by executeIf.
        if (line == "else" || startsWith(line, "else ") || line == "endif") continue;

        // Multi-line construct: if <cond> then
        if (startsWith(line, "if ")) {
            // Expect the line to end with "then" (or contain it after the condition)
            auto thenPos = line.rfind(" then");
            std::string_view condExpr;
            if (thenPos != std::string_view::npos) {
                condExpr = trimView(line.substr(3, thenPos - 3));  // between "if " and " then"
            } else {
                // "then" on the same line wasn't found — treat condition as everything after "if "
                condExpr = trimView(line.substr(3));
            }

            // Remainder = everything from 'pos' onwards in this block
            std::string_view remainder = block.substr(pos);
            if (!executeIf(condExpr, remainder)) return false;

            // Fast-forward past the matching endif
            // We need to skip to after the endif in the outer block.
            int depth = 1;
            while (pos < block.size() && depth > 0) {
                auto nl2     = block.find('\n', pos);
                auto le2     = (nl2 == std::string_view::npos) ? block.size() : nl2;
                std::string_view l2 = trimView(block.substr(pos, le2 - pos));
                pos = (nl2 == std::string_view::npos) ? block.size() : nl2 + 1;

                if (startsWith(l2, "if ")) ++depth;
                else if (l2 == "endif")   --depth;
            }
            continue;
        }

        if (!executeLine(line)) return false;
    }

    return true;
}

// =============================================================================
// Public entry point
// =============================================================================

bool FlxScript::execute(std::string_view script) {
    m_statementsExecuted = 0;
    m_shouldStop         = false;

    if (script.empty()) return true;

    return executeBlock(script);
}

} // namespace flx::flxapp
