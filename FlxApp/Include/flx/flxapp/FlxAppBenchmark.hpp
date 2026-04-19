#pragma once

#include <cstdint>
#include <string>

namespace flx::flxapp {

/**
 * @brief Benchmarks the JSON (fkyaml) vs YAML (fkyaml) parsers at runtime.
 *
 * Runs each parser N times against a given file path, measures wall-clock
 * time via esp_timer_get_time(), and logs a structured result table.
 *
 * Usage:
 *   FlxAppBenchmark::run("/data/apps/hello.flxapp",      // JSON file
 *                        "/data/apps/hello.flxapp.yaml", // YAML equivalent
 *                        20);                            // iterations
 */
class FlxAppBenchmark {
public:

    struct Result {
        uint64_t jsonTotalUs  = 0;   ///< Aggregate parse time for JSON (µs)
        uint64_t yamlTotalUs  = 0;   ///< Aggregate parse time for YAML (µs)
        uint32_t iterations   = 0;
        bool     jsonSuccess  = false;
        bool     yamlSuccess  = false;
    };

    /**
     * @brief Run benchmark: parse each file `iterations` times.
     * @param jsonPath  Path to the JSON .flxapp file
     * @param yamlPath  Path to the YAML .flxapp.yaml file
     * @param iterations  Number of parse repetitions per format
     * @return Populated Result struct
     */
    static Result run(const std::string& jsonPath,
                      const std::string& yamlPath,
                      uint32_t iterations = 20);

    /**
     * @brief Log a human-readable benchmark report to the console.
     */
    static void logResult(const Result& result);

private:

    static uint64_t benchmarkJson(const std::string& path,
                                  uint32_t iterations,
                                  bool& success);

    static uint64_t benchmarkYaml(const std::string& path,
                                  uint32_t iterations,
                                  bool& success);
};

} // namespace flx::flxapp
