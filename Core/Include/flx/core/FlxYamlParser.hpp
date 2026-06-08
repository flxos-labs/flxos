#pragma once

#include <cJSON.h>
#include <string_view>

namespace flx::core {

class FlxYamlParser {
public:

	/**
     * @brief Parses a YAML string and returns a cJSON tree.
     * @param input The YAML content to parse.
     * @return cJSON* Root of the parsed tree, or nullptr on failure.
     *         The caller is responsible for deleting the tree via cJSON_Delete().
     */
	static cJSON* parse(std::string_view input);
};

} // namespace flx::core
