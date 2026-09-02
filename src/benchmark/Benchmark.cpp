#include "benchmark/Benchmark.hpp"

#include "simulation/Simulation.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

namespace fruitcat {
namespace {

constexpr float DELTA_TIME = 1.0F / 60.0F;
constexpr int WARMUP_FRAMES = 5;
constexpr std::uint32_t REPETITION_SEED_STEP = 0x9E3779B9U;

struct Measurement {
    int repetition = 0;
    std::uint32_t seed = 0U;
    double sequentialMs = 0.0;
    double parallelMs = 0.0;
    double sequentialMsPerFrame = 0.0;
    double parallelMsPerFrame = 0.0;
    double sequentialFps = 0.0;
    double parallelFps = 0.0;
    double speedup = 0.0;
    double efficiencyPercentage = 0.0;
};

bool sameVector(const Vec3& left, const Vec3& right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

bool sameCat(const FruitCat& left, const FruitCat& right) {
    return left.id == right.id
        && left.randomState == right.randomState
        && left.fruitType == right.fruitType
        && left.state == right.state
        && sameVector(left.position, right.position)
        && sameVector(left.velocity, right.velocity)
        && left.radius == right.radius
        && left.impacts == right.impacts
        && left.hitCooldown == right.hitCooldown
        && left.stateTimer == right.stateTimer;
}

bool sameSimulation(const Simulation& left, const Simulation& right) {
    if (left.cats().size() != right.cats().size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.cats().size(); ++index) {
        if (!sameCat(left.cats()[index], right.cats()[index])) {
            return false;
        }
    }
    return true;
}

template <typename Update>
double measureUpdates(int frames, Update update) {
    // El intervalo incluye solo las actualizaciones: excluye construccion,
    // comparacion, escritura del CSV e impresion en consola.
    const auto start = std::chrono::steady_clock::now();
    for (int frame = 0; frame < frames; ++frame) {
        update();
    }
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

std::uint32_t derivedSeed(std::uint32_t baseSeed, int repetition) {
    return baseSeed + REPETITION_SEED_STEP * static_cast<std::uint32_t>(repetition);
}

bool validPositive(double value) {
    return std::isfinite(value) && value > 0.0;
}

} // namespace

int runBenchmark(const ProgramOptions& options) {
    const ArenaBounds bounds{-8.0F, 8.0F, -6.98F, 7.0F, -8.0F, 8.0F};

    std::ofstream csvProbe(options.benchmarkCsvPath, std::ios::out | std::ios::trunc);
    if (!csvProbe) {
        std::fprintf(stderr, "Error: no se pudo crear el archivo CSV: %s\n",
                     options.benchmarkCsvPath.c_str());
        return 1;
    }
    csvProbe.close();

    Simulation sequentialWarmup(options.catCount, bounds, options.seed);
    Simulation parallelWarmup(options.catCount, bounds, options.seed);
    for (int frame = 0; frame < WARMUP_FRAMES; ++frame) {
        sequentialWarmup.updateSequential(DELTA_TIME);
        parallelWarmup.updateParallel(DELTA_TIME, options.threadCount);
    }

    std::printf("Benchmark de simulacion sin renderizado\n");
    std::printf("N: %d\n", options.catCount);
    std::printf("Hilos: %d\n", options.threadCount);
    std::printf("Semilla base: %u\n", static_cast<unsigned int>(options.seed));
    std::printf("Frames por medicion: %d\n", options.benchmarkFrames);
    std::printf("Repeticiones: %d\n", options.benchmarkRepetitions);
    std::puts("Los tiempos y FPS corresponden unicamente a la simulacion, sin renderizado.");

    std::vector<Measurement> measurements;
    measurements.reserve(static_cast<std::size_t>(options.benchmarkRepetitions));
    double sequentialTotal = 0.0;
    double parallelTotal = 0.0;

    for (int repetition = 0; repetition < options.benchmarkRepetitions; ++repetition) {
        const std::uint32_t seed = derivedSeed(options.seed, repetition);
        // La misma semilla derivada garantiza estados iniciales identicos para
        // comparar justamente las rutas secuencial y paralela.
        Simulation sequential(options.catCount, bounds, seed);
        Simulation parallel(options.catCount, bounds, seed);

        double sequentialMs = 0.0;
        double parallelMs = 0.0;
        // Se alterna el orden para reducir el sesgo por calentamiento o carga
        // transitoria del sistema entre ambas rutas.
        if (repetition % 2 == 0) {
            sequentialMs = measureUpdates(options.benchmarkFrames, [&] {
                sequential.updateSequential(DELTA_TIME);
            });
            parallelMs = measureUpdates(options.benchmarkFrames, [&] {
                parallel.updateParallel(DELTA_TIME, options.threadCount);
            });
        } else {
            parallelMs = measureUpdates(options.benchmarkFrames, [&] {
                parallel.updateParallel(DELTA_TIME, options.threadCount);
            });
            sequentialMs = measureUpdates(options.benchmarkFrames, [&] {
                sequential.updateSequential(DELTA_TIME);
            });
        }

        if (!sameSimulation(sequential, parallel)) {
            std::fprintf(stderr, "Error: las simulaciones divergieron en la repeticion %d.\n", repetition + 1);
            return 1;
        }
        if (!validPositive(sequentialMs) || !validPositive(parallelMs)) {
            std::fprintf(stderr, "Error: se obtuvo un tiempo no finito o no positivo en la repeticion %d.\n",
                         repetition + 1);
            return 1;
        }

        Measurement measurement;
        measurement.repetition = repetition + 1;
        measurement.seed = seed;
        measurement.sequentialMs = sequentialMs;
        measurement.parallelMs = parallelMs;
        measurement.sequentialMsPerFrame = sequentialMs / options.benchmarkFrames;
        measurement.parallelMsPerFrame = parallelMs / options.benchmarkFrames;
        measurement.sequentialFps = 1000.0 / measurement.sequentialMsPerFrame;
        measurement.parallelFps = 1000.0 / measurement.parallelMsPerFrame;
        if (!validPositive(measurement.sequentialFps) || !validPositive(measurement.parallelFps)) {
            std::fprintf(stderr, "Error: se obtuvo un FPS no finito o no positivo en la repeticion %d.\n",
                         repetition + 1);
            return 1;
        }
        // speedup = secuencial / paralelo; eficiencia = speedup / hilos * 100.
        measurement.speedup = sequentialMs / parallelMs;
        measurement.efficiencyPercentage = measurement.speedup / options.threadCount * 100.0;
        measurements.push_back(measurement);
        sequentialTotal += sequentialMs;
        parallelTotal += parallelMs;

        std::printf("Repeticion %d: secuencial: total=%.6f ms, %.9f ms/frame, %.3f FPS; "
                    "paralelo: total=%.6f ms, %.9f ms/frame, %.3f FPS; "
                    "speedup=%.6fx, eficiencia=%.6f%%\n",
                    measurement.repetition, measurement.sequentialMs, measurement.sequentialMsPerFrame,
                    measurement.sequentialFps, measurement.parallelMs, measurement.parallelMsPerFrame,
                    measurement.parallelFps,
                    measurement.speedup, measurement.efficiencyPercentage);
    }

    const double averageSequential = sequentialTotal / options.benchmarkRepetitions;
    const double averageParallel = parallelTotal / options.benchmarkRepetitions;
    // El resultado final usa el cociente de promedios, no el promedio de speedups.
    const double finalSpeedup = averageSequential / averageParallel;
    const double finalEfficiency = finalSpeedup / options.threadCount * 100.0;
    const double averageSequentialFps = options.benchmarkFrames * 1000.0 / averageSequential;
    const double averageParallelFps = options.benchmarkFrames * 1000.0 / averageParallel;
    if (!validPositive(averageSequentialFps) || !validPositive(averageParallelFps)) {
        std::fputs("Error: se obtuvo un FPS promedio no finito o no positivo.\n", stderr);
        return 1;
    }

    std::ofstream csv(options.benchmarkCsvPath, std::ios::out | std::ios::trunc);
    if (!csv) {
        std::fprintf(stderr, "Error: no se pudo escribir el archivo CSV: %s\n",
                     options.benchmarkCsvPath.c_str());
        return 1;
    }
    csv << "tipo,repeticion,n,hilos,semilla,frames,tiempo_secuencial_ms,tiempo_paralelo_ms,"
           "tiempo_secuencial_ms_por_frame,tiempo_paralelo_ms_por_frame,fps_secuencial,fps_paralelo,"
           "speedup,eficiencia_porcentaje\n";
    csv << std::fixed << std::setprecision(9);
    for (const Measurement& measurement : measurements) {
        csv << "medicion," << measurement.repetition << ',' << options.catCount << ','
            << options.threadCount << ',' << measurement.seed << ',' << options.benchmarkFrames << ','
            << measurement.sequentialMs << ',' << measurement.parallelMs << ','
            << measurement.sequentialMsPerFrame << ',' << measurement.parallelMsPerFrame << ','
            << measurement.sequentialFps << ',' << measurement.parallelFps << ','
            << measurement.speedup << ',' << measurement.efficiencyPercentage << '\n';
    }
    csv << "promedio," << options.benchmarkRepetitions << ',' << options.catCount << ','
        << options.threadCount << ',' << options.seed << ',' << options.benchmarkFrames << ','
        << averageSequential << ',' << averageParallel << ','
        << averageSequential / options.benchmarkFrames << ','
        << averageParallel / options.benchmarkFrames << ','
        << averageSequentialFps << ',' << averageParallelFps << ','
        << finalSpeedup << ',' << finalEfficiency << '\n';
    csv.close();
    if (!csv) {
        std::fprintf(stderr, "Error: fallo al completar el archivo CSV: %s\n",
                     options.benchmarkCsvPath.c_str());
        return 1;
    }

    std::printf("Promedio secuencial: total=%.6f ms, %.9f ms/frame, %.3f FPS\n",
                averageSequential, averageSequential / options.benchmarkFrames, averageSequentialFps);
    std::printf("Promedio paralelo: total=%.6f ms, %.9f ms/frame, %.3f FPS\n",
                averageParallel, averageParallel / options.benchmarkFrames, averageParallelFps);
    std::printf("Speedup final: %.6fx\n", finalSpeedup);
    std::printf("Eficiencia final: %.6f%%\n", finalEfficiency);
    std::printf("CSV: %s\n", options.benchmarkCsvPath.c_str());
    std::puts("Equivalencia secuencial/paralela: OK");
    std::puts("BENCHMARK_OK");
    return 0;
}

} // namespace fruitcat
