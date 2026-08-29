#pragma once

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

    void update(float deltaTime);

    [[nodiscard]] const std::vector<FruitCat>& cats() const;
    [[nodiscard]] int activeCats() const;
    [[nodiscard]] int totalCats() const;

private:
    void spawn(FruitCat& cat);
    void updateCat(FruitCat& cat, float deltaTime);
    void resolveWallCollision(FruitCat& cat);
    void resolveCatCollisions();
    void applyImpact(FruitCat& cat);

    ArenaBounds bounds_;
    std::vector<FruitCat> cats_;
    std::uint32_t randomState_;
};

} // namespace fruitcat
