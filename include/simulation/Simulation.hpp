#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/Types.hpp"

namespace fruitcat {

struct ArenaBounds {
    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
};

class Simulation {
public:
    Simulation(int catCount, ArenaBounds bounds, std::uint32_t seed = 20260828U);

    void updateSequential(float deltaTime);
    void updateParallel(float deltaTime, int threadCount);

    [[nodiscard]] const std::vector<FruitCat>& cats() const;
    [[nodiscard]] int activeCats() const;
    [[nodiscard]] int totalCats() const;

private:
    struct CollisionEvent {
        std::size_t firstIndex;
        std::size_t secondIndex;
    };

    void spawn(FruitCat& cat);
    void updateCat(FruitCat& cat, float deltaTime);
    void resolveWallCollision(FruitCat& cat);
    [[nodiscard]] std::vector<CollisionEvent> detectCollisionsSequential() const;
    [[nodiscard]] std::vector<CollisionEvent> detectCollisionsParallel(int threadCount) const;
    void resolveCollisionEvents(std::vector<CollisionEvent> events);
    void resolveCollisionPair(FruitCat& first, FruitCat& second);
    void applyImpact(FruitCat& cat);

    ArenaBounds bounds_;
    std::vector<FruitCat> cats_;
    /*
     * VERSION ANTERIOR:
     * Este miembro almacenaba un unico estado pseudoaleatorio compartido.
     * Se reemplazo por FruitCat::randomState para que cada gato modifique su
     * propio estado y no se produzca una condicion de carrera en paralelo.
     *
     * Codigo anterior:
     * std::uint32_t randomState_;
     */
};

} // namespace fruitcat
