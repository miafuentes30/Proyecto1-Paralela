#pragma once

namespace fruitcat {

struct Vec3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

enum class FruitType {
    Banana,
    Avocado
};

enum class CatState {
    Alive,
    Exploding,
    Respawning
};

struct FruitCat {
    int id = 0;
    FruitType fruitType = FruitType::Banana;
    CatState state = CatState::Alive;

    Vec3 position;
    Vec3 velocity;
    float radius = 0.42F;

    int impacts = 0;
    float hitCooldown = 0.0F;
    float stateTimer = 0.0F;
};

} // namespace fruitcat
