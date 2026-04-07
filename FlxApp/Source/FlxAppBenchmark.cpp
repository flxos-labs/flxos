#include <flx/flxapp/FlxAppBenchmark.hpp>

#include <cJSON.h>
#include <esp_timer.h>
#include <flx/core/Logger.hpp>
#include <flx/core/Value.hpp>

#include <fstream>
#include <sstream>
#include <string>

namespace flx::flxapp {

namespace {

constexpr const char* TAG = "FlxAppBenchmark";

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return {};
    }
    std::ostringstream buf;
    buf << input.rdbuf();
    return buf.str();
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

FlxAppBenchmark::Result FlxAppBenchmark::run(const std::string& jsonPath,
                                              const std::string& yamlPath,
                                              uint32_t iterations) {
    Log::info(TAG, "=== FlxApp Parser Benchmark ===");
    Log::info(TAG, "  JSON file : %s", jsonPath.c_str());
    Log::info(TAG, "  YAML file : %s", yamlPath.c_str());
    Log::info(TAG, "  Iterations: %lu", (unsigned long)iterations);

    Result result;
    result.iterations = iterations;
    result.jsonTotalUs = benchmarkJson(jsonPath, iterations, result.jsonSuccess);
    result.yamlTotalUs = benchmarkYaml(yamlPath, iterations, result.yamlSuccess);

    logResult(result);
    return result;
}

void FlxAppBenchmark::logResult(const Result& result) {
    const uint32_t it = result.iterations;

    if (result.jsonSuccess && it > 0) {
        const uint64_t jsonAvg = result.jsonTotalUs / it;
        Log::info(TAG, "  [JSON/cJSON]  total=%llu us  avg=%llu us  (%lu iters)",
                  (unsigned long long)result.jsonTotalUs,
                  (unsigned long long)jsonAvg,
                  (unsigned long)it);
    } else {
        Log::warn(TAG, "  [JSON/cJSON]  FAILED or skipped");
    }

    if (result.yamlSuccess && it > 0) {
        const uint64_t yamlAvg = result.yamlTotalUs / it;
        Log::info(TAG, "  [YAML/ryml]   total=%llu us  avg=%llu us  (%lu iters)",
                  (unsigned long long)result.yamlTotalUs,
                  (unsigned long long)yamlAvg,
                  (unsigned long)it);
    } else {
        Log::warn(TAG, "  [YAML/ryml]   FAILED or skipped");
    }

    if (result.jsonSuccess && result.yamlSuccess && result.jsonTotalUs > 0) {
        // ratio: how many times slower is YAML compared to JSON?
        const float ratio = static_cast<float>(result.yamlTotalUs) /
                            static_cast<float>(result.jsonTotalUs);
        Log::info(TAG, "  Overhead ratio: YAML/JSON = %.2fx", ratio);
        if (ratio < 1.0f) {
            Log::info(TAG, "  Winner: ryml  (%.2fx faster)", 1.0f / ratio);
        } else {
            Log::info(TAG, "  Winner: cJSON  (%.2fx faster)", ratio);
        }
    }

    Log::info(TAG, "================================");
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

uint64_t FlxAppBenchmark::benchmarkJson(const std::string& path,
                                         uint32_t iterations,
                                         bool& success) {
    const std::string raw = readFile(path);
    if (raw.empty()) {
        Log::warn(TAG, "JSON benchmark: could not read %s", path.c_str());
        success = false;
        return 0;
    }

    uint64_t total = 0;
    success = false;

    for (uint32_t i = 0; i < iterations; ++i) {
        const int64_t t0 = esp_timer_get_time();
        cJSON* root = cJSON_Parse(raw.c_str());
        const int64_t t1 = esp_timer_get_time();

        if (root == nullptr) {
            Log::error(TAG, "JSON benchmark: cJSON_Parse failed on iteration %lu", (unsigned long)i);
            break;
        }

        cJSON_Delete(root);
        total += static_cast<uint64_t>(t1 - t0);
        success = true;
    }

    return total;
}

uint64_t FlxAppBenchmark::benchmarkYaml(const std::string& path,
                                         uint32_t iterations,
                                         bool& success) {
    const std::string raw = readFile(path);
    if (raw.empty()) {
        Log::warn(TAG, "YAML benchmark: could not read %s", path.c_str());
        success = false;
        return 0;
    }

    uint64_t total = 0;
    success = false;

    for (uint32_t i = 0; i < iterations; ++i) {
        const int64_t t0 = esp_timer_get_time();
        auto document = flx::core::FlxValueDocument::parseYaml(raw);
        const int64_t t1 = esp_timer_get_time();

        if (!document) {
            Log::error(TAG, "YAML benchmark: parse failed on iteration %lu", (unsigned long)i);
            break;
        }

        total += static_cast<uint64_t>(t1 - t0);
        success = true;
    }

    return total;
}

} // namespace flx::flxapp
