#pragma once

#include <flx/apps/AppManifest.hpp>
#include <optional>
#include <string>

namespace flx::flxapp {

class FlxAppManifest {
public:

    static std::optional<flx::apps::AppManifest> loadFromFile(const std::string& path);
};

} // namespace flx::flxapp
