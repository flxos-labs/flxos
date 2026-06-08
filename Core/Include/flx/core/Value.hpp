#pragma once

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <cJSON.h>
#include <flx/core/FlxYamlParser.hpp>

namespace flx::core {

class FlxValueView {
public:

	FlxValueView() = default;

	bool valid() const { return m_node != nullptr; }
	bool readable() const { return valid(); }
	bool hasChildren() const {
		return readable() && (cJSON_IsObject(m_node) || cJSON_IsArray(m_node)) && (m_node->child != nullptr);
	}
	bool isMap() const { return readable() && cJSON_IsObject(m_node); }
	bool isSeq() const { return readable() && cJSON_IsArray(m_node); }
	bool hasValue() const { return readable() && !cJSON_IsObject(m_node) && !cJSON_IsArray(m_node); }
	bool isNull() const { return readable() && cJSON_IsNull(m_node); }
	bool isQuoted() const { return readable() && cJSON_IsString(m_node); }
	bool isBoolScalar() const { return readable() && cJSON_IsBool(m_node); }
	bool isIntScalar() const { return readable() && cJSON_IsNumber(m_node); }

	FlxValueView child(std::string_view key) const {
		if (!isMap()) {
			return {};
		}

		const std::string keyString(key);
		const cJSON* childNode = cJSON_GetObjectItemCaseSensitive(m_node, keyString.c_str());
		if (!childNode) {
			return {};
		}
		return FlxValueView(m_owner, childNode);
	}

	bool hasChild(std::string_view key) const {
		if (!isMap()) {
			return false;
		}
		const std::string keyString(key);
		return cJSON_GetObjectItemCaseSensitive(m_node, keyString.c_str()) != nullptr;
	}

	std::string asString(const std::string& fallback = {}) const {
		if (!hasValue() || isNull()) {
			return fallback;
		}

		try {
			return scalarToString(m_node);
		} catch (...) {
			return fallback;
		}
	}

	int64_t asInt64(int64_t fallback = 0) const {
		if (!isIntScalar()) {
			return fallback;
		}

		try {
			return static_cast<int64_t>(m_node->valuedouble);
		} catch (...) {
			return fallback;
		}
	}

	bool asBool(bool fallback = false) const {
		if (!isBoolScalar()) {
			return fallback;
		}

		try {
			return cJSON_IsTrue(m_node);
		} catch (...) {
			return fallback;
		}
	}

	std::string toJsonString() const {
		if (!readable()) {
			return {};
		}

		try {
			char* printed = cJSON_PrintUnformatted(m_node);
			if (!printed) {
				return {};
			}
			std::string result(printed);
			cJSON_free(printed);
			return result;
		} catch (...) {
			return {};
		}
	}

	template<typename Fn>
	void forEachChild(Fn&& fn) const {
		if (!readable()) {
			return;
		}

		if (cJSON_IsArray(m_node) || cJSON_IsObject(m_node)) {
			const cJSON* child = m_node->child;
			while (child) {
				fn(FlxValueView(m_owner, child));
				child = child->next;
			}
		}
	}

	template<typename Fn>
	void forEachNamedChild(Fn&& fn) const {
		if (!isMap()) {
			return;
		}

		const cJSON* child = m_node->child;
		while (child) {
			const char* key = child->string ? child->string : "";
			fn(std::string_view(key), FlxValueView(m_owner, child));
			child = child->next;
		}
	}

	const cJSON* node() const { return m_node; }

private:

	friend class FlxValueDocument;

	FlxValueView(std::shared_ptr<const cJSON> owner, const cJSON* node)
		: m_owner(std::move(owner)), m_node(node) {
	}

	static std::string scalarToString(const cJSON* node) {
		if (cJSON_IsString(node)) {
			return node->valuestring ? node->valuestring : "";
		}

		if (cJSON_IsBool(node)) {
			return cJSON_IsTrue(node) ? "true" : "false";
		}

		if (cJSON_IsNumber(node)) {
			double val = node->valuedouble;
			if (val == static_cast<double>(static_cast<int64_t>(val))) {
				return std::to_string(static_cast<int64_t>(val));
			} else {
				std::ostringstream stream;
				stream.precision(std::numeric_limits<double>::max_digits10);
				stream << val;
				return stream.str();
			}
		}

		if (cJSON_IsNull(node)) {
			return "";
		}

		return {};
	}

	std::shared_ptr<const cJSON> m_owner {};
	const cJSON* m_node {nullptr};
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
	const cJSON* tree() const { return m_root.get(); }

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

	FlxValueDocument(std::string source, std::shared_ptr<const cJSON> root)
		: m_source(std::move(source)), m_root(std::move(root)) {
	}

	static std::optional<FlxValueDocument> parse(std::string source, Format format) {
		if (source.empty()) {
			return std::nullopt;
		}

		cJSON* parsed = nullptr;
		if (format == Format::Json) {
			parsed = cJSON_Parse(source.c_str());
		} else {
			parsed = FlxYamlParser::parse(source);
		}

		if (!parsed) {
			if (format == Format::Json) {
				const char* err = cJSON_GetErrorPtr();
				if (err) {
					size_t offset = err - source.c_str();
					std::string snippet;
					for (size_t i = 0; i < 15 && (err + i) < (source.c_str() + source.size()); ++i) {
						char c = err[i];
						if (c == '\0') break;
						if (std::isprint(static_cast<unsigned char>(c))) {
							snippet.push_back(c);
						} else {
							snippet += "\\x";
							char hex[3];
							std::snprintf(hex, sizeof(hex), "%02X", static_cast<unsigned char>(c));
							snippet += hex;
						}
					}
					std::printf("[cJSON Error] Parsing failed at offset %zu (near '%s')\n", offset, snippet.c_str());
				} else {
					std::printf("[cJSON Error] Parsing failed with null error pointer\n");
				}
			}
			return std::nullopt;
		}

		auto root = std::shared_ptr<const cJSON>(parsed, [](const cJSON* node) {
			cJSON_Delete(const_cast<cJSON*>(node));
		});
		return FlxValueDocument(std::move(source), std::move(root));
	}

	std::string m_source {};
	std::shared_ptr<const cJSON> m_root {};
};

} // namespace flx::core
