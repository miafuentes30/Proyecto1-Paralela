#include "core/ProgramOptions.hpp"

#include <charconv>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

namespace fruitcat {
namespace {

constexpr int MIN_CAT_COUNT = 1;
constexpr int MAX_CAT_COUNT = 2000;
constexpr int MIN_THREAD_COUNT = 1;
constexpr int MAX_THREAD_COUNT = 256;

template <typename Integer>
bool parseInteger(const std::string& text, Integer& value) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = begin + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value, 10);
    return result.ec == std::errc{} && result.ptr == end;
}

OptionsParseResult errorResult(const std::string& message) {
    OptionsParseResult result;
    result.status = OptionsParseStatus::Error;
    result.errorMessage = message;
    return result;
}

bool isUnknownOption(const std::string& argument) {
    return argument.size() > 1 && argument[0] == '-'
        && (argument[1] < '0' || argument[1] > '9');
}

} // namespace

OptionsParseResult parseProgramOptions(int argc, const char* const argv[]) {
    OptionsParseResult result;
    std::vector<std::string> positional;
    bool sawFullscreen = false;
    bool sawHelp = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index] != nullptr ? argv[index] : "";
        if (argument == "-f" || argument == "--fullscreen") {
            if (sawFullscreen) {
                return errorResult("La opcion de pantalla completa fue proporcionada mas de una vez.");
            }
            sawFullscreen = true;
            result.options.fullscreen = true;
        } else if (argument == "-h" || argument == "--help") {
            if (sawHelp) {
                return errorResult("La opcion de ayuda fue proporcionada mas de una vez.");
            }
            sawHelp = true;
        } else if (isUnknownOption(argument)) {
            return errorResult("Opcion desconocida: " + argument);
        } else {
            positional.push_back(argument);
        }
    }

    if (sawHelp) {
        if (sawFullscreen || !positional.empty()) {
            return errorResult("La opcion de ayuda no puede combinarse con otros argumentos.");
        }
        result.status = OptionsParseStatus::Help;
        return result;
    }

    if (positional.size() > 4) {
        return errorResult("Se proporcionaron argumentos sobrantes.");
    }

    if (!positional.empty()) {
        int catCount = 0;
        if (!parseInteger(positional[0], catCount) || catCount < MIN_CAT_COUNT || catCount > MAX_CAT_COUNT) {
            return errorResult("N debe ser un entero entre 1 y 2000.");
        }
        result.options.catCount = catCount;
    }

    if (positional.size() >= 2) {
        if (positional[1] == "sequential") {
            result.options.mode = ExecutionMode::Sequential;
        } else if (positional[1] == "parallel") {
            result.options.mode = ExecutionMode::Parallel;
        } else {
            return errorResult("Modo desconocido: " + positional[1] + ". Use sequential o parallel.");
        }
    }

    const bool threadCountProvided = positional.size() >= 3;
    if (result.options.mode == ExecutionMode::Parallel && !threadCountProvided) {
        return errorResult("El modo parallel requiere una cantidad de hilos explicita entre 1 y 256.");
    }

    if (threadCountProvided) {
        int threadCount = 0;
        if (!parseInteger(positional[2], threadCount)
            || threadCount < MIN_THREAD_COUNT || threadCount > MAX_THREAD_COUNT) {
            return errorResult("La cantidad de hilos debe ser un entero entre 1 y 256.");
        }
        if (result.options.mode == ExecutionMode::Sequential && threadCount != 1) {
            return errorResult("El modo sequential requiere exactamente 1 hilo.");
        }
        result.options.threadCount = threadCount;
    }

    if (positional.size() >= 4) {
        std::uint32_t seed = 0U;
        if (!parseInteger(positional[3], seed)) {
            return errorResult("La semilla debe ser un entero sin signo entre 0 y 4294967295.");
        }
        result.options.seed = seed;
    }

    return result;
}

std::string programUsage(const char* executableName) {
    const std::string name = executableName != nullptr && executableName[0] != '\0'
        ? executableName
        : "fruitcat-chaos";
    return "Uso: " + name + " [N] [modo] [hilos] [semilla] [-f|--fullscreen]\n"
        "  N: entero entre 1 y 2000 (predeterminado: 40).\n"
        "  modo: sequential o parallel (predeterminado: sequential).\n"
        "  hilos: sequential requiere 1; parallel requiere 1 a 256 (predeterminado: 1).\n"
        "  semilla: entero sin signo entre 0 y 4294967295 (predeterminado: 20260828).\n"
        "  -f, --fullscreen: iniciar en pantalla completa.\n"
        "  -h, --help: mostrar esta ayuda sin abrir la ventana.\n"
        "Ejemplos:\n"
        "  fruitcat-chaos 2000 sequential 1 20260828\n"
        "  fruitcat-chaos 2000 parallel 8 20260828\n"
        "  fruitcat-chaos 100 sequential 1 42 --fullscreen\n";
}

const char* executionModeName(ExecutionMode mode) {
    return mode == ExecutionMode::Sequential ? "sequential" : "parallel";
}

} // namespace fruitcat
