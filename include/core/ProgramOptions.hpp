#pragma once

#include <cstdint>
#include <string>

namespace fruitcat {

enum class ExecutionMode {
    Sequential,
    Parallel,
    Benchmark
};

struct ProgramOptions {
    int catCount = 40;
    ExecutionMode mode = ExecutionMode::Sequential;
    int threadCount = 1;
    std::uint32_t seed = 20260828U;
    int benchmarkFrames = 0;
    int benchmarkRepetitions = 0;
    std::string benchmarkCsvPath;
    bool fullscreen = false;
};

enum class OptionsParseStatus {
    Success,
    Help,
    Error
};

struct OptionsParseResult {
    OptionsParseStatus status = OptionsParseStatus::Success;
    ProgramOptions options;
    std::string errorMessage;
};

OptionsParseResult parseProgramOptions(int argc, const char* const argv[]);
std::string programUsage(const char* executableName);
const char* executionModeName(ExecutionMode mode);

} // namespace fruitcat
