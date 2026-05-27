#pragma once

#include <flx/flxapp/FlxAppState.hpp>
#include <cstdint>
#include <string>
#include <string_view>

namespace flx::flxapp {

class FlxApp;

/**
 * @brief Lightweight custom scripting engine for FlxApp.
 *
 * FlxScript is a minimal, zero-dependency domain-specific language (DSL)
 * that executes inline inside FlxApp action lists via the "script" action type.
 *
 * Supported syntax:
 *   - Assignment:   varName = expression
 *   - Conditionals: if expr then ... else ... endif
 *   - OS commands:  log("msg"), notify("title", "msg"), navigate("appId"),
 *                   event_publish("event"), close(), increment(key, n), toggle(key)
 *   - Comments:     lines starting with '#'
 *
 * Design principles:
 *   - Zero heap allocation per execution: tokens are string_view slices.
 *   - No AST or bytecode: single-pass line-by-line evaluation.
 *   - State reads/writes go directly through FlxAppState (no separate symbol table).
 *   - Hard statement limit (kMaxStatements) to prevent runaway scripts.
 */
class FlxScript {
public:

    FlxScript(FlxAppState& state, FlxApp& app);

    /**
     * @brief Execute a multi-line script block.
     * @param script The script text (may be a string_view into YAML document memory).
     * @return true if execution completed normally, false on fatal error.
     */
    bool execute(std::string_view script);

private:

    // -------------------------------------------------------------------------
    // Tagged variant type — mirrors FlxAppState::Value, lives on the stack.
    // -------------------------------------------------------------------------
    struct Val {
        enum class Type : uint8_t { Int, Str, Bool } type = Type::Int;

        int32_t     intVal  = 0;
        std::string strVal  {};
        bool        boolVal = false;

        bool        isTrue()    const;
        std::string asString()  const;
        int32_t     asInt()     const;
    };

    // -------------------------------------------------------------------------
    // Block execution (used for if/else branches)
    // -------------------------------------------------------------------------
    struct Block {
        std::string_view thenLines {};  // raw text of the then-block
        std::string_view elseLines {};  // raw text of the else-block (may be empty)
    };

    // -------------------------------------------------------------------------
    // Line-level execution
    // -------------------------------------------------------------------------
    bool executeLine(std::string_view line);
    bool executeBlock(std::string_view block);
    bool executeAssignment(std::string_view lhs, std::string_view rhs);
    bool executeCall(std::string_view funcName, std::string_view argsRaw);

    // Parse "if <cond> then ... [else ...] endif" starting at the first "then"
    // line.  Returns the tail of the script after "endif".
    bool executeIf(std::string_view condExpr, std::string_view remainder);

    // -------------------------------------------------------------------------
    // Recursive-descent expression evaluator (2-level precedence)
    // -------------------------------------------------------------------------
    Val evalExpr(std::string_view expr);
    Val evalComparison(std::string_view& cursor);
    Val evalAddSub(std::string_view& cursor);
    Val evalAtom(std::string_view& cursor);

    // -------------------------------------------------------------------------
    // Argument parsing helpers
    // -------------------------------------------------------------------------
    std::string_view parseStringLiteral(std::string_view& cursor);
    std::string      evaluateArg(std::string_view arg);
    std::string_view parseArgs(std::string_view argsRaw,
                               std::string args[],
                               int maxArgs,
                               int& outCount);

    // -------------------------------------------------------------------------
    // Identifier / value resolution
    // -------------------------------------------------------------------------
    Val              resolveIdentifier(std::string_view name);
    static std::string_view trimView(std::string_view sv);
    static bool      startsWith(std::string_view sv, std::string_view prefix);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------
    FlxAppState& m_state;
    FlxApp&      m_app;

    uint16_t m_statementsExecuted = 0;
    bool     m_shouldStop         = false;  // set by close()

    static constexpr uint16_t kMaxStatements = 200;
};

} // namespace flx::flxapp
