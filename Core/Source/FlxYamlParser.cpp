#include <flx/core/FlxYamlParser.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace flx::core {

namespace {

void trim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(),
		s.end());
}

std::string unquote(std::string_view val) {
	if (val.size() < 2) return std::string(val);
	char quote = val.front();
	if ((quote == '"' && val.back() == '"') || (quote == '\'' && val.back() == '\'')) {
		std::string result;
		result.reserve(val.size() - 2);
		if (quote == '"') {
			for (size_t i = 1; i < val.size() - 1; ++i) {
				if (val[i] == '\\' && i + 1 < val.size() - 1) {
					i++;
					switch (val[i]) {
						case 'n':
							result.push_back('\n');
							break;
						case 't':
							result.push_back('\t');
							break;
						case 'r':
							result.push_back('\r');
							break;
						case 'b':
							result.push_back('\b');
							break;
						case 'f':
							result.push_back('\f');
							break;
						case '"':
							result.push_back('"');
							break;
						case '\\':
							result.push_back('\\');
							break;
						case '/':
							result.push_back('/');
							break;
						default:
							result.push_back('\\');
							result.push_back(val[i]);
							break;
					}
				} else {
					result.push_back(val[i]);
				}
			}
		} else {
			for (size_t i = 1; i < val.size() - 1; ++i) {
				if (val[i] == '\\' && i + 1 < val.size() - 1) {
					i++;
					if (val[i] == '\'' || val[i] == '\\') {
						result.push_back(val[i]);
					} else {
						result.push_back('\\');
						result.push_back(val[i]);
					}
				} else {
					result.push_back(val[i]);
				}
			}
		}
		return result;
	}
	return std::string(val);
}

std::string_view stripComment(std::string_view line_text) {
	size_t i = 0;
	bool in_double_quote = false;
	bool in_single_quote = false;
	while (i < line_text.size()) {
		char ch = line_text[i];
		if (ch == '"' && (i == 0 || line_text[i - 1] != '\\')) {
			in_double_quote = !in_double_quote;
		} else if (ch == '\'' && !in_double_quote) {
			in_single_quote = !in_single_quote;
		} else if (!in_double_quote && !in_single_quote) {
			if (ch == '#') {
				if (i == 0 || std::isspace(static_cast<unsigned char>(line_text[i - 1]))) {
					return line_text.substr(0, i);
				}
			}
		}
		i++;
	}
	return line_text;
}

bool splitKeyValue(std::string_view line_text, std::string& key, std::string& value) {
	size_t i = 0;
	bool in_double_quote = false;
	bool in_single_quote = false;
	while (i < line_text.size()) {
		char ch = line_text[i];
		if (ch == '"' && (i == 0 || line_text[i - 1] != '\\')) {
			in_double_quote = !in_double_quote;
		} else if (ch == '\'' && !in_double_quote) {
			in_single_quote = !in_single_quote;
		} else if (!in_double_quote && !in_single_quote) {
			if (ch == ':') {
				if (i + 1 == line_text.size() || std::isspace(static_cast<unsigned char>(line_text[i + 1]))) {
					key = std::string(line_text.substr(0, i));
					value = std::string(line_text.substr(i + 1));
					trim(key);
					trim(value);
					key = unquote(key);
					return true;
				}
			}
		}
		i++;
	}
	return false;
}

cJSON* parseScalar(std::string_view valStr) {
	std::string val(valStr);
	trim(val);
	if (val == "true" || val == "True" || val == "TRUE") {
		return cJSON_CreateTrue();
	}
	if (val == "false" || val == "False" || val == "FALSE") {
		return cJSON_CreateFalse();
	}
	if (val == "null" || val == "Null" || val == "NULL" || val == "~") {
		return cJSON_CreateNull();
	}

	if ((val.size() >= 2 && val.front() == '"' && val.back() == '"') ||
		(val.size() >= 2 && val.front() == '\'' && val.back() == '\'')) {
		return cJSON_CreateString(unquote(val).c_str());
	}

	char* endptr = nullptr;
	double d = std::strtod(val.c_str(), &endptr);
	if (endptr != val.c_str() && *endptr == '\0') {
		return cJSON_CreateNumber(d);
	}

	return cJSON_CreateString(val.c_str());
}

class FlowParser {
public:

	explicit FlowParser(std::string_view text) : m_text(text), m_pos(0) {}

	cJSON* parse() {
		skipSpaces();
		if (m_pos >= m_text.size()) return nullptr;
		if (m_text[m_pos] == '{') {
			return parseObject();
		} else if (m_text[m_pos] == '[') {
			return parseArray();
		} else {
			std::string scalar = readScalar();
			return parseScalar(scalar);
		}
	}

private:

	void skipSpaces() {
		while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos]))) {
			m_pos++;
		}
	}

	cJSON* parseObject() {
		if (m_pos >= m_text.size() || m_text[m_pos] != '{') return nullptr;
		m_pos++; // skip '{'
		cJSON* obj = cJSON_CreateObject();

		while (true) {
			skipSpaces();
			if (m_pos >= m_text.size()) {
				cJSON_Delete(obj);
				return nullptr;
			}
			if (m_text[m_pos] == '}') {
				m_pos++;
				return obj;
			}

			std::string key = readScalar();
			if (key.empty()) {
				cJSON_Delete(obj);
				return nullptr;
			}

			skipSpaces();
			if (m_pos >= m_text.size() || m_text[m_pos] != ':') {
				cJSON_Delete(obj);
				return nullptr;
			}
			m_pos++; // skip ':'

			skipSpaces();
			cJSON* val = parseValue();
			if (!val) {
				cJSON_Delete(obj);
				return nullptr;
			}

			cJSON_AddItemToObject(obj, key.c_str(), val);

			skipSpaces();
			if (m_pos < m_text.size() && m_text[m_pos] == ',') {
				m_pos++;
			} else if (m_pos < m_text.size() && m_text[m_pos] == '}') {
				// Will be handled in the next loop iteration
			} else {
				cJSON_Delete(obj);
				return nullptr;
			}
		}
	}

	cJSON* parseArray() {
		if (m_pos >= m_text.size() || m_text[m_pos] != '[') return nullptr;
		m_pos++; // skip '['
		cJSON* arr = cJSON_CreateArray();

		while (true) {
			skipSpaces();
			if (m_pos >= m_text.size()) {
				cJSON_Delete(arr);
				return nullptr;
			}
			if (m_text[m_pos] == ']') {
				m_pos++;
				return arr;
			}

			cJSON* val = parseValue();
			if (!val) {
				cJSON_Delete(arr);
				return nullptr;
			}

			cJSON_AddItemToArray(arr, val);

			skipSpaces();
			if (m_pos < m_text.size() && m_text[m_pos] == ',') {
				m_pos++;
			} else if (m_pos < m_text.size() && m_text[m_pos] == ']') {
				// Will be handled in the next loop iteration
			} else {
				cJSON_Delete(arr);
				return nullptr;
			}
		}
	}

	cJSON* parseValue() {
		skipSpaces();
		if (m_pos >= m_text.size()) return nullptr;
		if (m_text[m_pos] == '{') {
			return parseObject();
		} else if (m_text[m_pos] == '[') {
			return parseArray();
		} else {
			std::string scalar = readScalar();
			return parseScalar(scalar);
		}
	}

	std::string readScalar() {
		skipSpaces();
		if (m_pos >= m_text.size()) return {};

		char quote = m_text[m_pos];
		if (quote == '"' || quote == '\'') {
			size_t start = m_pos;
			m_pos++; // skip open quote
			while (m_pos < m_text.size()) {
				if (m_text[m_pos] == quote) {
					if (quote == '"' && m_text[m_pos - 1] == '\\') {
						m_pos++;
						continue;
					}
					m_pos++; // skip close quote
					break;
				}
				m_pos++;
			}
			return unquote(m_text.substr(start, m_pos - start));
		} else {
			size_t start = m_pos;
			while (m_pos < m_text.size()) {
				char ch = m_text[m_pos];
				if (std::isspace(static_cast<unsigned char>(ch)) ||
					ch == ',' || ch == ':' || ch == '}' || ch == ']' || ch == '[' || ch == '{') {
					break;
				}
				m_pos++;
			}
			return std::string(m_text.substr(start, m_pos - start));
		}
	}

	std::string_view m_text;
	size_t m_pos;
};

struct LineInfo {
	std::string raw;
	int indent;
	bool isEmpty;
	bool isComment;
	std::string content;
};

std::vector<LineInfo> preprocess(std::string_view input) {
	std::vector<LineInfo> lines;
	size_t pos = 0;
	while (pos < input.size()) {
		size_t next_line = input.find('\n', pos);
		std::string_view line_view;
		if (next_line == std::string_view::npos) {
			line_view = input.substr(pos);
			pos = input.size();
		} else {
			line_view = input.substr(pos, next_line - pos);
			pos = next_line + 1;
		}

		if (!line_view.empty() && line_view.back() == '\r') {
			line_view.remove_suffix(1);
		}

		LineInfo info;
		info.raw = std::string(line_view);
		info.isEmpty = false;
		info.isComment = false;
		info.indent = 0;

		size_t i = 0;
		while (i < line_view.size() && line_view[i] == ' ') {
			i++;
		}
		info.indent = static_cast<int>(i);

		std::string_view stripped = line_view.substr(i);
		if (stripped.empty()) {
			info.isEmpty = true;
			info.indent = -1;
		} else if (stripped[0] == '#') {
			info.isComment = true;
			info.indent = -1;
		} else {
			std::string_view no_comment = stripComment(stripped);
			std::string content_str(no_comment);
			trim(content_str);
			info.content = content_str;
			if (info.content.empty()) {
				info.isEmpty = true;
				info.indent = -1;
			}
		}
		lines.push_back(info);
	}
	return lines;
}

void skipEmptyAndComments(const std::vector<LineInfo>& lines, size_t& idx) {
	while (idx < lines.size() && (lines[idx].isEmpty || lines[idx].isComment)) {
		idx++;
	}
}

std::string parseBlockScalar(const std::vector<LineInfo>& lines, size_t& idx, int parent_indent) {
	std::string result;
	int base_indent = -1;
	size_t scan_idx = idx;
	while (scan_idx < lines.size()) {
		std::string_view raw = lines[scan_idx].raw;
		size_t first_non_space = raw.find_first_not_of(' ');
		if (first_non_space != std::string_view::npos && raw[first_non_space] != '\r') {
			base_indent = static_cast<int>(first_non_space);
			break;
		}
		scan_idx++;
	}

	if (base_indent == -1 || base_indent <= parent_indent) {
		return {};
	}

	while (idx < lines.size()) {
		std::string_view raw = lines[idx].raw;
		if (!raw.empty() && raw.back() == '\r') {
			raw.remove_suffix(1);
		}

		size_t first_non_space = raw.find_first_not_of(' ');
		bool is_empty = (first_non_space == std::string_view::npos);

		if (!is_empty) {
			int indent = static_cast<int>(first_non_space);
			if (indent < base_indent) {
				break;
			}
			result += std::string(raw.substr(base_indent)) + "\n";
		} else {
			result += "\n";
		}
		idx++;
	}

	return result;
}

cJSON* parseBlock(const std::vector<LineInfo>& lines, size_t& idx, int indent);
cJSON* parseMapping(const std::vector<LineInfo>& lines, size_t& idx, int indent, cJSON* existingObj = nullptr);
cJSON* parseSequence(const std::vector<LineInfo>& lines, size_t& idx, int indent);

cJSON* parseBlock(const std::vector<LineInfo>& lines, size_t& idx, int indent) {
	skipEmptyAndComments(lines, idx);
	if (idx >= lines.size()) {
		return cJSON_CreateNull();
	}

	int line_indent = lines[idx].indent;
	if (line_indent < indent) {
		return cJSON_CreateNull();
	}

	std::string_view content = lines[idx].content;
	if (content.rfind("-", 0) == 0) {
		return parseSequence(lines, idx, line_indent);
	} else {
		std::string key, val;
		if (splitKeyValue(content, key, val)) {
			return parseMapping(lines, idx, line_indent);
		} else {
			idx++;
			return parseScalar(content);
		}
	}
}

cJSON* parseMapping(const std::vector<LineInfo>& lines, size_t& idx, int indent, cJSON* existingObj) {
	cJSON* obj = existingObj ? existingObj : cJSON_CreateObject();
	while (idx < lines.size()) {
		skipEmptyAndComments(lines, idx);
		if (idx >= lines.size()) {
			break;
		}

		int line_indent = lines[idx].indent;
		if (line_indent < indent) {
			break;
		}

		if (line_indent > indent) {
			break;
		}

		std::string_view content = lines[idx].content;
		std::string key, val;
		if (!splitKeyValue(content, key, val)) {
			break;
		}

		idx++;

		cJSON* valueNode = nullptr;
		if (val == "|") {
			std::string blockStr = parseBlockScalar(lines, idx, indent);
			valueNode = cJSON_CreateString(blockStr.c_str());
		} else if (!val.empty() && (val.front() == '{' || val.front() == '[')) {
			valueNode = FlowParser(val).parse();
		} else if (!val.empty()) {
			valueNode = parseScalar(val);
		} else {
			size_t next_idx = idx;
			skipEmptyAndComments(lines, next_idx);
			if (next_idx < lines.size() && lines[next_idx].indent > indent) {
				valueNode = parseBlock(lines, idx, indent + 1);
			} else {
				valueNode = cJSON_CreateNull();
			}
		}

		if (valueNode) {
			cJSON_AddItemToObject(obj, key.c_str(), valueNode);
		}
	}
	return obj;
}

cJSON* parseSequence(const std::vector<LineInfo>& lines, size_t& idx, int indent) {
	cJSON* arr = cJSON_CreateArray();
	while (idx < lines.size()) {
		skipEmptyAndComments(lines, idx);
		if (idx >= lines.size()) {
			break;
		}

		int line_indent = lines[idx].indent;
		if (line_indent < indent) {
			break;
		}

		if (line_indent > indent) {
			break;
		}

		std::string_view content = lines[idx].content;
		if (content.rfind("-", 0) != 0) {
			break;
		}

		std::string_view item_content = content.substr(1);
		size_t non_space = item_content.find_first_not_of(' ');
		if (non_space != std::string_view::npos) {
			item_content = item_content.substr(non_space);
		} else {
			item_content = {};
		}

		idx++;

		cJSON* valueNode = nullptr;
		if (!item_content.empty()) {
			std::string key, val;
			if (splitKeyValue(item_content, key, val)) {
				cJSON* obj = cJSON_CreateObject();
				cJSON* firstVal = nullptr;
				if (val == "|") {
					std::string blockStr = parseBlockScalar(lines, idx, indent + 2);
					firstVal = cJSON_CreateString(blockStr.c_str());
				} else if (val.front() == '{' || val.front() == '[') {
					firstVal = FlowParser(val).parse();
				} else {
					firstVal = parseScalar(val);
				}
				if (firstVal) {
					cJSON_AddItemToObject(obj, key.c_str(), firstVal);
				}

				size_t next_idx = idx;
				skipEmptyAndComments(lines, next_idx);
				if (next_idx < lines.size() && lines[next_idx].indent > indent) {
					int map_indent = lines[next_idx].indent;
					parseMapping(lines, idx, map_indent, obj);
				}
				valueNode = obj;
			} else if (item_content.front() == '{' || item_content.front() == '[') {
				valueNode = FlowParser(item_content).parse();
			} else {
				valueNode = parseScalar(item_content);
			}
		} else {
			size_t next_idx = idx;
			skipEmptyAndComments(lines, next_idx);
			if (next_idx < lines.size() && lines[next_idx].indent > indent) {
				valueNode = parseBlock(lines, idx, lines[next_idx].indent);
			} else {
				valueNode = cJSON_CreateNull();
			}
		}

		if (valueNode) {
			cJSON_AddItemToArray(arr, valueNode);
		}
	}
	return arr;
}

} // namespace

cJSON* FlxYamlParser::parse(std::string_view input) {
	if (input.empty()) {
		return nullptr;
	}

	std::vector<LineInfo> lines = preprocess(input);
	size_t idx = 0;
	skipEmptyAndComments(lines, idx);
	if (idx >= lines.size()) {
		return nullptr;
	}

	return parseBlock(lines, idx, 0);
}

} // namespace flx::core
