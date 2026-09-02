#include "core/ProgramOptions.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

fruitcat::OptionsParseResult parse(std::initializer_list<const char*> arguments) {
    std::vector<const char*> argv(arguments);
    return fruitcat::parseProgramOptions(static_cast<int>(argv.size()), argv.data());
}

int failures = 0;

void expect(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FALLO: " << description << '\n';
        ++failures;
    }
}

void expectError(std::initializer_list<const char*> arguments, const char* description) {
    expect(parse(arguments).status == fruitcat::OptionsParseStatus::Error, description);
}

} // namespace

int main() {
    const auto defaults = parse({"fruitcat-chaos"});
    expect(defaults.status == fruitcat::OptionsParseStatus::Success, "argumentos predeterminados validos");
    expect(defaults.options.catCount == 40, "N predeterminado");
    expect(defaults.options.mode == fruitcat::ExecutionMode::Sequential, "modo predeterminado");
    expect(defaults.options.threadCount == 1, "hilos predeterminados");
    expect(defaults.options.seed == 20260828U, "semilla predeterminada");
    expect(!defaults.options.fullscreen, "ventana predeterminada");

    const auto onlyN = parse({"fruitcat-chaos", "100"});
    expect(onlyN.status == fruitcat::OptionsParseStatus::Success && onlyN.options.catCount == 100,
           "compatibilidad con solamente N");

    const auto sequential = parse({"fruitcat-chaos", "2000", "sequential", "1", "20260828"});
    expect(sequential.status == fruitcat::OptionsParseStatus::Success, "comando sequential completo");
    expect(sequential.options.mode == fruitcat::ExecutionMode::Sequential
           && sequential.options.threadCount == 1 && sequential.options.seed == 20260828U,
           "valores del comando sequential");

    const auto parallel = parse({"fruitcat-chaos", "2000", "parallel", "8", "20260828"});
    expect(parallel.status == fruitcat::OptionsParseStatus::Success, "comando parallel completo");
    expect(parallel.options.mode == fruitcat::ExecutionMode::Parallel && parallel.options.threadCount == 8,
           "valores del comando parallel");

    const auto benchmark = parse({"fruitcat-chaos", "100", "benchmark", "4", "20260828",
                                  "20", "10", "benchmark.csv"});
    expect(benchmark.status == fruitcat::OptionsParseStatus::Success, "comando benchmark completo");
    expect(benchmark.options.mode == fruitcat::ExecutionMode::Benchmark
           && benchmark.options.threadCount == 4 && benchmark.options.benchmarkFrames == 20
           && benchmark.options.benchmarkRepetitions == 10
           && benchmark.options.benchmarkCsvPath == "benchmark.csv", "valores del comando benchmark");

    const auto zeroSeed = parse({"fruitcat-chaos", "40", "sequential", "1", "0"});
    expect(zeroSeed.status == fruitcat::OptionsParseStatus::Success && zeroSeed.options.seed == 0U,
           "semilla minima");

    const auto maximumSeed = parse({"fruitcat-chaos", "40", "sequential", "1", "4294967295"});
    expect(maximumSeed.status == fruitcat::OptionsParseStatus::Success
           && maximumSeed.options.seed == std::numeric_limits<std::uint32_t>::max(), "semilla maxima");

    const auto fullscreen = parse({"fruitcat-chaos", "100", "-f"});
    expect(fullscreen.status == fruitcat::OptionsParseStatus::Success && fullscreen.options.fullscreen,
           "pantalla completa compatible");

    expect(parse({"fruitcat-chaos", "--help"}).status == fruitcat::OptionsParseStatus::Help, "ayuda");
    expectError({"fruitcat-chaos", "0"}, "N igual a cero");
    expectError({"fruitcat-chaos", "2001"}, "N fuera del maximo");
    expectError({"fruitcat-chaos", "100", "invalid"}, "modo desconocido");
    expectError({"fruitcat-chaos", "100", "sequential", "4"}, "sequential con varios hilos");
    expectError({"fruitcat-chaos", "100", "parallel"}, "parallel sin hilos");
    expectError({"fruitcat-chaos", "100", "parallel", "0"}, "cero hilos");
    expectError({"fruitcat-chaos", "100", "parallel", "257"}, "hilos fuera del maximo");
    expectError({"fruitcat-chaos", "100", "sequential", "1", "-1"}, "semilla negativa");
    expectError({"fruitcat-chaos", "100", "sequential", "1", "4294967296"}, "semilla fuera de uint32");
    expectError({"fruitcat-chaos", "100", "sequential", "1", "42", "extra"}, "argumentos sobrantes");
    expectError({"fruitcat-chaos", "100", "benchmark", "4", "20260828", "20", "9", "out.csv"},
                "benchmark con menos de diez repeticiones");
    expectError({"fruitcat-chaos", "100", "benchmark", "4", "20260828", "0", "10", "out.csv"},
                "benchmark sin frames positivos");
    expectError({"fruitcat-chaos", "100", "benchmark", "4", "20260828", "20", "10"},
                "benchmark sin archivo CSV");
    expectError({"fruitcat-chaos", "100", "benchmark", "4", "20260828", "20", "10", "out.csv", "extra"},
                "benchmark con argumentos sobrantes");
    expectError({"fruitcat-chaos", "100", "benchmark", "4", "20260828", "20", "10", "out.csv", "--fullscreen"},
                "benchmark con pantalla completa");
    expectError({"fruitcat-chaos", "--unknown"}, "opcion desconocida");
    expectError({"fruitcat-chaos", "-f", "--fullscreen"}, "opcion repetida");

    if (failures != 0) {
        return 1;
    }
    std::cout << "OK: argumentos validos, limites y errores comprobados\n";
    return 0;
}
