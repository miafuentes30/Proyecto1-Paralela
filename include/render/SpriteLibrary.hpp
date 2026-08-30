#pragma once

#include "core/Types.hpp"

namespace fruitcat {

enum class Sprite {
    BananaCat,
    AvocadoCat,
    BananaExplosion,
    AvocadoExplosion
};

Sprite catSprite(FruitType fruitType);
Sprite explosionSprite(FruitType fruitType);

// Returns the cached GL texture name for a sprite, uploading it on first use.
// Returns 0 when the PNG could not be read; callers must skip texturing then.
unsigned int spriteTexture(Sprite sprite);

} // namespace fruitcat
