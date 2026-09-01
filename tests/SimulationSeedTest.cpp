#include "simulation/Simulation.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

bool sameVector(const fruitcat::Vec3& left, const fruitcat::Vec3& right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

bool sameCat(const fruitcat::FruitCat& left, const fruitcat::FruitCat& right) {
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

bool sameSimulation(const fruitcat::Simulation& left, const fruitcat::Simulation& right) {
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

bool finiteVector(const fruitcat::Vec3& vector) {
    return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
}

bool allFinite(const fruitcat::Simulation& simulation) {
    for (const fruitcat::FruitCat& cat : simulation.cats()) {
        if (!finiteVector(cat.position) || !finiteVector(cat.velocity)) {
            return false;
        }
    }
    return true;
}

int fail(const char* message) {
    std::cerr << "FALLO: " << message << '\n';
    return 1;
}

} // namespace

int main() {
    constexpr int catCount = 40;
    constexpr std::uint32_t seed = 20260828U;
    const fruitcat::ArenaBounds bounds{-8.0F, 8.0F, -6.98F, 7.0F, -8.0F, 8.0F};

    fruitcat::Simulation first(catCount, bounds, seed);
    fruitcat::Simulation second(catCount, bounds, seed);
    fruitcat::Simulation different(catCount, bounds, seed + 1U);

    if (!sameSimulation(first, second)) {
        return fail("la inicializacion con la misma semilla no es reproducible");
    }
    if (sameSimulation(first, different)) {
        return fail("dos semillas distintas generaron el mismo estado");
    }
    if (!allFinite(first) || !allFinite(second) || !allFinite(different)) {
        return fail("la inicializacion produjo un valor no finito");
    }

    for (int frame = 0; frame < 600; ++frame) {
        first.updateSequential(1.0F / 60.0F);
        second.updateSequential(1.0F / 60.0F);
        different.updateSequential(1.0F / 60.0F);
    }

    if (!sameSimulation(first, second)) {
        return fail("las simulaciones iguales divergieron durante la actualizacion");
    }
    if (sameSimulation(first, different)) {
        return fail("las simulaciones con semillas distintas convergieron por completo");
    }
    if (!allFinite(first) || !allFinite(second) || !allFinite(different)) {
        return fail("la actualizacion produjo un valor no finito");
    }

    fruitcat::Simulation sequentialRoute(catCount, bounds, seed);
    fruitcat::Simulation preparatoryParallelRoute(catCount, bounds, seed);
    for (int frame = 0; frame < 600; ++frame) {
        sequentialRoute.updateSequential(1.0F / 60.0F);
        preparatoryParallelRoute.updateParallel(1.0F / 60.0F, 4);
    }

    if (!sameSimulation(sequentialRoute, preparatoryParallelRoute)) {
        return fail("la ruta paralela preparatoria no coincide con la ruta secuencial");
    }
    if (!allFinite(sequentialRoute) || !allFinite(preparatoryParallelRoute)) {
        return fail("la comparacion temporal entre rutas produjo un valor no finito");
    }

    std::cout << "OK: reproducibilidad y equivalencia preparatoria durante 600 frames\n";
    return 0;
}
