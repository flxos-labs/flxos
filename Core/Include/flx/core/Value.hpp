#pragma once

#include <cctype>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <fkYAML/node.hpp>

namespace flx::core {

class FlxValueView {
public:

	FlxValueView() = default;

	bool valid() const { return m_node != nullptr; }
	bool readable() const { return valid(); }
	bool hasChildren() const {
		return readable() && (m_node->is_mapping() || m_node->is_sequence()) && !m_node->empty();
	}
	bool isMap() const { return readable() && m_node->is_mapping(); }
	bool isSeq() const { return readable() && m_node->is_sequence(); }
	bool hasValue() const { return readable() && m_node->is_scalar(); }
	bool isNull() const { return readable() && m_node->is_null(); }
	bool isQuoted() const { return readable() && m_node->is_string(); }
	bool isBoolScalar() const { return readable() && m_node->is_boolean(); }
	bool isIntScalar() const { return readable() && m_node->is_integer(); }

	FlxValueView child(std::string_view key) const {
		if (!isMap()) {
			return {};
		}

		const std::string keyString(key);
		if (!m_node->contains(keyString)) {
			return {};
		}

		try {
			const auto& childNode = m_node->at(keyString);
			return FlxValueView(m_owner, &childNode);
		} catch (...) {
			return {};
		}
	}

	bool hasChild(std::string_view key) const {
		if (!isMap()) {
			return false;
		}
		const std::string keyString(key);
		return m_node->contains(keyString);
	}

	std::string asString(const std::string& fallback = {}) const {
		if (!hasValue() || isNull()) {
			return fallback;
		}

		try {
			return scalarToString(*m_node);
		} catch (...) {
			return fallback;
		}
	}

	int64_t asInt64(int64_t fallback = 0) const {
		if (!isIntScalar()) {
			return fallback;
		}

		try {
			return m_node->template get_value<int64_t>();
		} catch (...) {
			return fallback;
		}
	}

	bool asBool(bool fallback = false) const {
		if (!isBoolScalar()) {
			return fallback;
		}

		try {
			return m_node->template get_value<bool>();
		} catch (...) {
			return fallback;
		}
	}

	std::string toJsonString() const {
		if (!readable()) {
			return {};
		}

		try {
			std::string output;
			output.reserve(128);
			appendJson(*m_node, output);
			return output;
		} catch (...) {
			return {};
		}
	}

	template<typename Fn>
	void forEachChild(Fn&& fn) const {
		if (!readable()) {
			return;
		}

		if (m_node->is_sequence()) {
			const auto& seq = m_node->template get_value_ref<const fkyaml::node::sequence_type&>();
			for (const auto& childNode: seq) {
				fn(FlxValueView(m_owner, &childNode));
			}
			return;
		}

		if (m_node->is_mapping()) {
			for (auto it = m_node->begin(); it != m_node->end(); ++it) {
				fn(FlxValueView(m_owner, &it.value()));
			}
		}
	}

	template<typename Fn>
	void forEachNamedChild(Fn&& fn) const {
		if (!isMap()) {
			return;
		}

		for (auto it = m_node->begin(); it != m_node->end(); ++it) {
			const auto& keyNode = it.key();
			if (keyNode.is_string()) {
				const auto& key = keyNode.template get_value_ref<const fkyaml::node::string_type&>();
				fn(std::string_view(key.data(), key.size()), FlxValueView(m_owner, &it.value()));
				continue;
			}

			const std::string key = scalarToString(keyNode);
			fn(std::string_view(key), FlxValueView(m_owner, &it.value()));
		}
	}

	const fkyaml::node* node() const { return m_node; }

private:

	friend class FlxValueDocument;

	FlxValueView(std::shared_ptr<const fkyaml::node> owner, const fkyaml::node* node)
		: m_owner(std::move(owner)), m_node(node) {
	}

	static std::string scalarToString(const fkyaml::node& node) {
		if (node.is_string()) {
			return node.template get_value<std::string>();
		}

		if (node.is_boolean()) {
			return node.template get_value<bool>() ? "true" : "false";
		}

		if (node.is_integer()) {
			return std::to_string(node.template get_value<int64_t>());
		}

		if (node.is_float_number()) {
			std::ostringstream stream;
			stream.precision(std::numeric_limits<double>::max_digits10);
			stream << node.template get_value<double>();
			return stream.str();
		}

		if (node.is_null()) {
			return "";
		}

		return {};
	}

	static void appendEscapedJsonString(std::string_view input, std::string& output) {
		static constexpr char kHex[] = "0123456789abcdef";

		for (const char ch: input) {
			switch (ch) {
				case '"':
					output += "\\\"";
					break;
				case '\\':
					output += "\\\\";
					break;
				case '\b':
					output += "\\b";
					break;
				case '\f':
					output += "\\f";
					break;
				case '\n':
					output += "\\n";
					break;
				case '\r':
					output += "\\r";
					break;
				case '\t':
					output += "\\t";
					break;
				default: {
					const unsigned char byte = static_cast<unsigned char>(ch);
					if (byte < 0x20u) {
						output += "\\u00";
						output.push_back(kHex[(byte >> 4u) & 0x0Fu]);
						output.push_back(kHex[byte & 0x0Fu]);
					} else {
						output.push_back(ch);
					}
					break;
				}
			}
		}
	}

	static void appendJson(const fkyaml::node& node, std::string& output) {
		if (node.is_mapping()) {
			output.push_back('{');
			bool first = true;
			for (auto it = node.begin(); it != node.end(); ++it) {
				if (!first) {
					output.push_back(',');
				}
				first = false;

				output.push_back('"');
				appendEscapedJsonString(scalarToString(it.key()), output);
				output += "\":";
				appendJson(it.value(), output);
			}
			output.push_back('}');
			return;
		}

		if (node.is_sequence()) {
			output.push_back('[');
			bool first = true;
			const auto& seq = node.template get_value_ref<const fkyaml::node::sequence_type&>();
			for (const auto& childNode: seq) {
				if (!first) {
					output.push_back(',');
				}
				first = false;
				appendJson(childNode, output);
			}
			output.push_back(']');
			return;
		}

		if (node.is_null()) {
			output += "null";
			return;
		}

		if (node.is_boolean()) {
			output += node.template get_value<bool>() ? "true" : "false";
			return;
		}

		if (node.is_integer()) {
			output += std::to_string(node.template get_value<int64_t>());
			return;
		}

		if (node.is_float_number()) {
			std::ostringstream stream;
			stream.precision(std::numeric_limits<double>::max_digits10);
			stream << node.template get_value<double>();
			output += stream.str();
			return;
		}

		output.push_back('"');
		appendEscapedJsonString(node.template get_value<std::string>(), output);
		output.push_back('"');
	}

	std::shared_ptr<const fkyaml::node> m_owner {};
	const fkyaml::node* m_node {nullptr};
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
		if (!m_root) {
			return {};
		}
		return FlxValueView(m_root, m_root.get());
	}

	const std::string& source() const { return m_source; }
	const fkyaml::node& tree() const { return *m_root; }

private:

	static bool looksLikeJson(std::string_view source) {
		size_t index = 0;
		if (source.size() >= 3 &&
			static_cast<unsigned char>(source[0]) == 0xEF &&
			static_cast<unsigned char>(source[1]) == 0xBB &&
			static_cast<unsigned char>(source[2]) == 0xBF) {
			index = 3;
		}

		for (; index < source.size(); ++index) {
			const unsigned char ch = static_cast<unsigned char>(source[index]);
			if (std::isspace(ch)) {
				continue;
			}
			return ch == '{' || ch == '[';
		}

		return false;
	}

	static bool isValidJson(std::string_view input) {
		struct JsonValidator {
			explicit JsonValidator(std::string_view source) : m_source(source) {
				if (m_source.size() >= 3 &&
					static_cast<unsigned char>(m_source[0]) == 0xEF &&
					static_cast<unsigned char>(m_source[1]) == 0xBB &&
					static_cast<unsigned char>(m_source[2]) == 0xBF) {
					m_index = 3;
				}
			}

			bool parseDocument() {
				skipWhitespace();
				if (!parseValue()) {
					return false;
				}
				skipWhitespace();
				return m_index == m_source.size();
			}

		private:

			void skipWhitespace() {
				while (m_index < m_source.size()) {
					const unsigned char ch = static_cast<unsigned char>(m_source[m_index]);
					if (!std::isspace(ch)) {
						break;
					}
					++m_index;
				}
			}

			bool parseValue() {
				skipWhitespace();
				if (m_index >= m_source.size()) {
					return false;
				}

				const char ch = m_source[m_index];
				if (ch == '{') {
					return parseObject();
				}
				if (ch == '[') {
					return parseArray();
				}
				if (ch == '"') {
					return parseString();
				}
				if (ch == '-' || (ch >= '0' && ch <= '9')) {
					return parseNumber();
				}
				if (consumeLiteral("true")) {
					return true;
				}
				if (consumeLiteral("false")) {
					return true;
				}
				if (consumeLiteral("null")) {
					return true;
				}

				return false;
			}

			bool parseObject() {
				if (!consume('{')) {
					return false;
				}

				skipWhitespace();
				if (consume('}')) {
					return true;
				}

				while (true) {
					if (!parseString()) {
						return false;
					}

					skipWhitespace();
					if (!consume(':')) {
						return false;
					}

					if (!parseValue()) {
						return false;
					}

					skipWhitespace();
					if (consume('}')) {
						return true;
					}
					if (!consume(',')) {
						return false;
					}
				}
			}

			bool parseArray() {
				if (!consume('[')) {
					return false;
				}

				skipWhitespace();
				if (consume(']')) {
					return true;
				}

				while (true) {
					if (!parseValue()) {
						return false;
					}

					skipWhitespace();
					if (consume(']')) {
						return true;
					}
					if (!consume(',')) {
						return false;
					}
				}
			}

			bool parseString() {
				if (!consume('"')) {
					return false;
				}

				while (m_index < m_source.size()) {
					const unsigned char ch = static_cast<unsigned char>(m_source[m_index++]);

					if (ch == '"') {
						return true;
					}

					if (ch == '\\') {
						if (m_index >= m_source.size()) {
							return false;
						}

						const char escaped = m_source[m_index++];
						switch (escaped) {
							case '"':
							case '\\':
							case '/':
							case 'b':
							case 'f':
							case 'n':
							case 'r':
							case 't':
								break;
							case 'u':
								if (!consumeHex4()) {
									return false;
								}
								break;
							default:
								return false;
						}
						continue;
					}

					if (ch < 0x20u) {
						return false;
					}
				}

				return false;
			}

			bool parseNumber() {
				if (consume('-')) {
					if (m_index >= m_source.size()) {
						return false;
					}
				}

				if (!parseIntegerPart()) {
					return false;
				}

				if (consume('.')) {
					if (!consumeDigit()) {
						return false;
					}
					while (consumeDigit()) {
					}
				}

				if (peek('e') || peek('E')) {
					++m_index;
					if (peek('+') || peek('-')) {
						++m_index;
					}
					if (!consumeDigit()) {
						return false;
					}
					while (consumeDigit()) {
					}
				}

				return true;
			}

			bool parseIntegerPart() {
				if (consume('0')) {
					return !(peekDigit());
				}

				if (!consumeOneOf('1', '9')) {
					return false;
				}

				while (consumeDigit()) {
				}
				return true;
			}

			bool consume(char expected) {
				if (m_index >= m_source.size() || m_source[m_index] != expected) {
					return false;
				}
				++m_index;
				return true;
			}

			bool consumeLiteral(std::string_view literal) {
				if (m_index + literal.size() > m_source.size()) {
					return false;
				}
				if (m_source.substr(m_index, literal.size()) != literal) {
					return false;
				}
				m_index += literal.size();
				return true;
			}

			bool consumeHex4() {
				if (m_index + 4 > m_source.size()) {
					return false;
				}

				for (int i = 0; i < 4; ++i) {
					if (!isHexDigit(m_source[m_index + i])) {
						return false;
					}
				}
				m_index += 4;
				return true;
			}

			bool isHexDigit(char ch) const {
				return (ch >= '0' && ch <= '9') ||
					(ch >= 'a' && ch <= 'f') ||
					(ch >= 'A' && ch <= 'F');
			}

			bool consumeDigit() {
				if (!peekDigit()) {
					return false;
				}
				++m_index;
				return true;
			}

			bool peekDigit() const {
				if (m_index >= m_source.size()) {
					return false;
				}
				const char ch = m_source[m_index];
				return ch >= '0' && ch <= '9';
			}

			bool consumeOneOf(char first, char last) {
				if (m_index >= m_source.size()) {
					return false;
				}
				const char ch = m_source[m_index];
				if (ch < first || ch > last) {
					return false;
				}
				++m_index;
				return true;
			}

			bool peek(char ch) const {
				return m_index < m_source.size() && m_source[m_index] == ch;
			}

			std::string_view m_source;
			size_t m_index {0};
		};

		return JsonValidator(input).parseDocument();
	}

	static std::optional<fkyaml::node> parseTree(std::string_view input, Format format) {
		if (input.empty()) {
			return std::nullopt;
		}

		if (format == Format::Json) {
			if (!looksLikeJson(input)) {
				return std::nullopt;
			}
			// Enforce strict JSON syntax: reject YAML-only features
			if (!isValidJson(input)) {
				return std::nullopt;
			}
		}

		try {
			return fkyaml::node::deserialize(input);
		} catch (...) {
			return std::nullopt;
		}
	}

	FlxValueDocument(std::string source, std::shared_ptr<const fkyaml::node> root)
		: m_source(std::move(source)), m_root(std::move(root)) {
	}

	static std::optional<FlxValueDocument> parse(std::string source, Format format) {
		if (source.empty()) {
			return std::nullopt;
		}

		auto parsed = parseTree(source, format);
		if (!parsed) {
			return std::nullopt;
		}

		auto root = std::make_shared<fkyaml::node>(std::move(*parsed));
		return FlxValueDocument(std::move(source), std::move(root));
	}

	std::string m_source {};
	std::shared_ptr<const fkyaml::node> m_root {};
};

} // namespace flx::core
