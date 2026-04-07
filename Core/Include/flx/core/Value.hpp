#pragma once

#include <csetjmp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <rapidyaml.hpp>

namespace flx::core {

class FlxValueView {
public:

	FlxValueView() = default;
	explicit FlxValueView(const c4::yml::ConstNodeRef& node)
	    : m_node(node) {
	}

	bool valid() const { return !m_node.invalid(); }
	bool readable() const { return m_node.readable(); }
	bool hasChildren() const { return readable() && m_node.has_children(); }
	bool isMap() const { return readable() && m_node.is_map(); }
	bool isSeq() const { return readable() && m_node.is_seq(); }
	bool hasValue() const { return readable() && m_node.has_val(); }
	bool isNull() const { return readable() && m_node.val_is_null(); }
	bool isQuoted() const { return readable() && m_node.is_val_quoted(); }
	bool isBoolScalar() const {
		if (!hasValue() || isNull() || isQuoted()) {
			return false;
		}
		const c4::csubstr value = m_node.val();
		return value == "true" || value == "True" || value == "TRUE" ||
		       value == "false" || value == "False" || value == "FALSE";
	}
	bool isIntScalar() const {
		if (!hasValue() || isNull() || isQuoted()) {
			return false;
		}
		const c4::csubstr value = m_node.val();
		int64_t parsed = 0;
		return c4::from_chars(value, &parsed);
	}

	FlxValueView child(std::string_view key) const {
		if (!readable()) {
			return {};
		}
		return FlxValueView(m_node.find_child(c4::to_csubstr(key)));
	}

	bool hasChild(std::string_view key) const {
		return readable() && m_node.has_child(c4::to_csubstr(key));
	}

	std::string asString(const std::string& fallback = {}) const {
		if (!hasValue() || isNull()) {
			return fallback;
		}
		const c4::csubstr value = m_node.val();
		return std::string(value.str, value.len);
	}

	int64_t asInt64(int64_t fallback = 0) const {
		if (!isIntScalar()) {
			return fallback;
		}
		const c4::csubstr value = m_node.val();
		int64_t parsed = fallback;
		if (c4::from_chars(value, &parsed)) {
			return parsed;
		}
		return fallback;
	}

	bool asBool(bool fallback = false) const {
		if (!isBoolScalar()) {
			return fallback;
		}

		const c4::csubstr value = m_node.val();
		if (value == "true" || value == "True" || value == "TRUE" || value == "1") {
			return true;
		}
		if (value == "false" || value == "False" || value == "FALSE" || value == "0") {
			return false;
		}
		return fallback;
	}

	std::string toJsonString() const {
		if (!readable()) {
			return {};
		}

		std::string output(128, '\0');
		while (true) {
			c4::substr buffer(output.data(), output.size());
			c4::substr emitted = c4::yml::emit_json(m_node, buffer, false);
			if (emitted.str != nullptr) {
				output.resize(emitted.len);
				return output;
			}
			if (emitted.len == 0) {
				return {};
			}
			output.resize(emitted.len);
		}
	}

	template<typename Fn>
	void forEachChild(Fn&& fn) const {
		if (!readable()) {
			return;
		}
		for (const auto& child: m_node.children()) {
			fn(FlxValueView(child));
		}
	}

	template<typename Fn>
	void forEachNamedChild(Fn&& fn) const {
		if (!readable()) {
			return;
		}
		for (const auto& child: m_node.children()) {
			if (!child.has_key()) {
				continue;
			}
			fn(std::string_view(child.key().str, child.key().len), FlxValueView(child));
		}
	}

	const c4::yml::ConstNodeRef& node() const { return m_node; }

private:

	c4::yml::ConstNodeRef m_node {};
};

class FlxValueDocument {
public:

	enum class Format {
		Json,
		Yaml,
	};

	static std::optional<FlxValueDocument> parseJson(std::string source) {
		return parse(std::move(source), Format::Json);
	}

	static std::optional<FlxValueDocument> parseYaml(std::string source) {
		return parse(std::move(source), Format::Yaml);
	}

	static std::optional<FlxValueDocument> parseAuto(std::string source) {
		if (source.empty()) {
			return std::nullopt;
		}

		if (looksLikeJson(source)) {
			if (auto document = parse(source, Format::Json)) {
				return document;
			}
			return parse(std::move(source), Format::Yaml);
		}

		if (auto document = parse(source, Format::Yaml)) {
			return document;
		}
		return parse(std::move(source), Format::Json);
	}

	FlxValueView root() const {
		if (m_tree.empty()) {
			return {};
		}
		return FlxValueView(m_tree.crootref());
	}

	const std::string& source() const { return m_source; }
	const c4::yml::Tree& tree() const { return m_tree; }

private:

	struct ParseGuard {
		jmp_buf jump {};
	};

	static void onParseError(c4::csubstr /*msg*/, const c4::yml::ErrorDataParse& /*errdata*/, void* userData) noexcept {
		auto* guard = static_cast<ParseGuard*>(userData);
		longjmp(guard->jump, 1);
	}

	static void onBasicError(c4::csubstr /*msg*/, const c4::yml::ErrorDataBasic& /*errdata*/, void* userData) noexcept {
		auto* guard = static_cast<ParseGuard*>(userData);
		longjmp(guard->jump, 1);
	}

	static bool looksLikeJson(std::string_view source) {
		size_t index = 0;
		if (source.size() >= 3 &&
		    static_cast<unsigned char>(source[0]) == 0xEF &&
		    static_cast<unsigned char>(source[1]) == 0xBB &&
		    static_cast<unsigned char>(source[2]) == 0xBF) {
			index = 3;
		}

		for (; index < source.size(); ++index) {
			const char ch = source[index];
			if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v') {
				continue;
			}
			return ch == '{' || ch == '[';
		}

		return false;
	}

	static std::optional<c4::yml::Tree> parseTree(c4::csubstr input, Format format) {
		ParseGuard guard {};
		c4::yml::Callbacks callbacks;
		callbacks.set_user_data(&guard);
		callbacks.set_error_basic(&onBasicError);
		callbacks.set_error_parse(&onParseError);

		auto* eventHandler = new c4::yml::Parser::handler_type(callbacks);
		auto* parser = new c4::yml::Parser(eventHandler, c4::yml::ParserOptions{});
		auto* tree = new c4::yml::Tree(callbacks);

		if (setjmp(guard.jump) != 0) {
			delete tree;
			delete parser;
			delete eventHandler;
			return std::nullopt;
		}

		if (format == Format::Json) {
			c4::yml::parse_json_in_arena(parser, input, tree);
		} else {
			c4::yml::parse_in_arena(parser, input, tree);
		}

		std::optional<c4::yml::Tree> parsed {std::in_place, std::move(*tree)};
		delete tree;
		delete parser;
		delete eventHandler;
		return parsed;
	}

	FlxValueDocument(std::string source, c4::yml::Tree tree)
	    : m_source(std::move(source))
	    , m_tree(std::move(tree)) {
	}

	static std::optional<FlxValueDocument> parse(std::string source, Format format) {
		if (source.empty()) {
			return std::nullopt;
		}

		const c4::csubstr input = c4::to_csubstr(source);
		auto tree = parseTree(input, format);
		if (!tree) {
			return std::nullopt;
		}

		return FlxValueDocument(std::move(source), std::move(*tree));
	}

	std::string m_source {};
	c4::yml::Tree m_tree {};
};

} // namespace flx::core
