#include "core/OpenMpRuntime.hpp"

#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FALLO: " << description << '\n';
        ++failures;
    }
}

void validatePositiveValues(const fruitcat::OpenMpRuntimeInfo& info) {
    expect(info.requestedThreads >= 1, "hilos solicitados positivos");
    expect(info.activeThreads >= 1, "hilos activos positivos");
    expect(info.maximumThreads >= 1, "maximo de hilos positivo");
    expect(info.processorCount >= 1, "cantidad de procesadores positiva");
    expect(info.maximumThreads == info.requestedThreads, "maximo coherente con la configuracion");
}

} // namespace

int main() {
    const fruitcat::OpenMpRuntimeInfo oneThread = fruitcat::configureAndInspectOpenMp(1);
    validatePositiveValues(oneThread);
    expect(oneThread.activeThreads == 1, "solicitar un hilo crea exactamente un hilo");

    if (oneThread.processorCount >= 2) {
        const fruitcat::OpenMpRuntimeInfo twoThreads = fruitcat::configureAndInspectOpenMp(2);
        validatePositiveValues(twoThreads);
        expect(twoThreads.activeThreads == 2, "solicitar dos hilos crea exactamente dos hilos");
        expect(twoThreads.processorCount == oneThread.processorCount, "cantidad estable de procesadores");
    }

    if (failures != 0) {
        return 1;
    }
    std::cout << "OK: configuracion y equipo de hilos OpenMP comprobados\n";
    return 0;
}
