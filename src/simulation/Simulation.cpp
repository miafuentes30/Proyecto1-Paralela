#include "simulation/Simulation.hpp"

#include <algorithm>
#include <cmath>

namespace fruitcat {
namespace {

constexpr int MAX_IMPACTS = 7;
constexpr float HIT_COOLDOWN_SECONDS = 0.25F;
constexpr float EXPLOSION_SECONDS = 0.50F;
constexpr float RESPAWN_SECONDS = 0.75F;
constexpr float MIN_SPEED = 1.25F;
constexpr float MAX_SPEED = 2.80F;

std::uint32_t makeCatSeed(std::uint32_t baseSeed, std::uint32_t catId) {
    // Esta mezcla determinista conserva la reproducibilidad y asigna una
    // secuencia independiente a cada gato a partir de la semilla base.
    std::uint32_t value = baseSeed + 0x9E3779B9U * (catId + 1U);
    value ^= value >> 16U;
    value *= 0x85EBCA6BU;
    value ^= value >> 13U;
    value *= 0xC2B2AE35U;
    value ^= value >> 16U;
    return value;
}

float lengthSquared(const Vec3& vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

float nextRandomUnit(std::uint32_t& state) {
    state = state * 1664525U + 1013904223U;
    return static_cast<float>(state >> 8U) / static_cast<float>(0x00FFFFFFU);
}

float randomRange(std::uint32_t& state, float minimum, float maximum) {
    return minimum + (maximum - minimum) * nextRandomUnit(state);
}

} // namespace

/*
 * VERSION ANTERIOR:
 * Este inicializador conservaba un unico estado pseudoaleatorio compartido.
 * Se reemplazo porque produciria una condicion de carrera al actualizar
 * distintos gatos en paralelo.
 *
 * Codigo anterior:
 * Simulation::Simulation(int catCount, ArenaBounds bounds, std::uint32_t seed)
 *     : bounds_(bounds), randomState_(seed)
 */
Simulation::Simulation(int catCount, ArenaBounds bounds, std::uint32_t seed)
    : bounds_(bounds) {
    cats_.resize(static_cast<std::size_t>(catCount));
    for (int index = 0; index < catCount; ++index) {
        FruitCat& cat = cats_[static_cast<std::size_t>(index)];
        cat.id = index;
        /*
         * VERSION ANTERIOR:
         * Este bloque seleccionaba el tipo mediante el estado compartido.
         * Se reemplazo porque produciria una condicion de carrera al crear
         * o actualizar distintos gatos en paralelo.
         *
         * Codigo anterior:
         * cat.fruitType = nextRandomUnit(randomState_) < 0.5F
         *     ? FruitType::Banana
         *     : FruitType::Avocado;
         */
        cat.randomState = makeCatSeed(seed, static_cast<std::uint32_t>(index));
        cat.fruitType = nextRandomUnit(cat.randomState) < 0.5F ? FruitType::Banana : FruitType::Avocado;
        spawn(cat);
    }
}

/*
 * VERSION ANTERIOR:
 * Esta funcion reunia toda la actualizacion bajo un unico nombre.
 * Se separo para conservar una ruta secuencial y preparar una ruta paralela
 * que puedan compararse bajo las mismas condiciones.
 *
 * Codigo anterior:
 * void Simulation::update(float deltaTime) {
 *     const float safeDeltaTime = std::min(deltaTime, 0.05F);
 *     for (FruitCat& cat : cats_) {
 *         updateCat(cat, safeDeltaTime);
 *     }
 *     resolveCatCollisions();
 * }
 */
void Simulation::updateSequential(float deltaTime) {
    const float safeDeltaTime = std::min(deltaTime, 0.05F);
    for (FruitCat& cat : cats_) {
        updateCat(cat, safeDeltaTime);
    }
    resolveCatCollisions();
}

void Simulation::updateParallel(float deltaTime, int threadCount) {
    // Delegacion temporal: conserva un punto de comparacion identico. Los
    // hilos se usaran en la siguiente parte; esta ruta aun no mejora el
    // rendimiento y no debe medirse como una implementacion paralela.
    (void)threadCount;
    updateSequential(deltaTime);
}

const std::vector<FruitCat>& Simulation::cats() const {
    return cats_;
}

int Simulation::activeCats() const {
    return static_cast<int>(std::count_if(cats_.begin(), cats_.end(), [](const FruitCat& cat) {
        return cat.state == CatState::Alive;
    }));
}

int Simulation::totalCats() const {
    return static_cast<int>(cats_.size());
}

void Simulation::spawn(FruitCat& cat) {
    cat.state = CatState::Alive;
    cat.impacts = 0;
    cat.hitCooldown = 0.45F;
    cat.stateTimer = 0.0F;

    const float padding = cat.radius + 0.15F;
    /*
     * VERSION ANTERIOR:
     * Este bloque utilizaba un unico estado pseudoaleatorio compartido.
     * Se reemplazo porque produciria una condicion de carrera al actualizar
     * distintos gatos en paralelo.
     *
     * Codigo anterior:
     * cat.position = {
     *     randomRange(randomState_, bounds_.minX + padding, bounds_.maxX - padding),
     *     randomRange(randomState_, bounds_.minY + padding, bounds_.maxY - padding),
     *     randomRange(randomState_, bounds_.minZ + padding, bounds_.maxZ - padding)
     * };
     *
     * const float speed = randomRange(randomState_, MIN_SPEED, MAX_SPEED);
     * Vec3 direction{
     *     randomRange(randomState_, -1.0F, 1.0F),
     *     randomRange(randomState_, -1.0F, 1.0F),
     *     randomRange(randomState_, -1.0F, 1.0F)
     * };
     */
    cat.position = {
        randomRange(cat.randomState, bounds_.minX + padding, bounds_.maxX - padding),
        randomRange(cat.randomState, bounds_.minY + padding, bounds_.maxY - padding),
        randomRange(cat.randomState, bounds_.minZ + padding, bounds_.maxZ - padding)
    };

    const float speed = randomRange(cat.randomState, MIN_SPEED, MAX_SPEED);
    Vec3 direction{
        randomRange(cat.randomState, -1.0F, 1.0F),
        randomRange(cat.randomState, -1.0F, 1.0F),
        randomRange(cat.randomState, -1.0F, 1.0F)
    };
    const float length = std::sqrt(std::max(lengthSquared(direction), 0.001F));
    cat.velocity = {direction.x / length * speed, direction.y / length * speed, direction.z / length * speed};
}

void Simulation::updateCat(FruitCat& cat, float deltaTime) {
    if (cat.state == CatState::Exploding) {
        cat.stateTimer -= deltaTime;
        if (cat.stateTimer <= 0.0F) {
            cat.state = CatState::Respawning;
            cat.stateTimer = RESPAWN_SECONDS;
        }
        return;
    }

    if (cat.state == CatState::Respawning) {
        cat.stateTimer -= deltaTime;
        if (cat.stateTimer <= 0.0F) {
            spawn(cat);
        }
        return;
    }

    cat.hitCooldown = std::max(0.0F, cat.hitCooldown - deltaTime);
    cat.position.x += cat.velocity.x * deltaTime;
    cat.position.y += cat.velocity.y * deltaTime;
    cat.position.z += cat.velocity.z * deltaTime;
    resolveWallCollision(cat);
}

void Simulation::resolveWallCollision(FruitCat& cat) {
    bool hitWall = false;
    const float radius = cat.radius;

    if (cat.position.x - radius < bounds_.minX) {
        cat.position.x = bounds_.minX + radius;
        cat.velocity.x = std::abs(cat.velocity.x);
        hitWall = true;
    } else if (cat.position.x + radius > bounds_.maxX) {
        cat.position.x = bounds_.maxX - radius;
        cat.velocity.x = -std::abs(cat.velocity.x);
        hitWall = true;
    }

    if (cat.position.y - radius < bounds_.minY) {
        cat.position.y = bounds_.minY + radius;
        cat.velocity.y = std::abs(cat.velocity.y);
        hitWall = true;
    } else if (cat.position.y + radius > bounds_.maxY) {
        cat.position.y = bounds_.maxY - radius;
        cat.velocity.y = -std::abs(cat.velocity.y);
        hitWall = true;
    }

    if (cat.position.z - radius < bounds_.minZ) {
        cat.position.z = bounds_.minZ + radius;
        cat.velocity.z = std::abs(cat.velocity.z);
        hitWall = true;
    } else if (cat.position.z + radius > bounds_.maxZ) {
        cat.position.z = bounds_.maxZ - radius;
        cat.velocity.z = -std::abs(cat.velocity.z);
        hitWall = true;
    }

    if (hitWall && cat.hitCooldown <= 0.0F) {
        applyImpact(cat);
    }
}

void Simulation::resolveCatCollisions() {
    for (std::size_t first = 0; first < cats_.size(); ++first) {
        FruitCat& a = cats_[first];
        if (a.state != CatState::Alive) {
            continue;
        }

        for (std::size_t second = first + 1; second < cats_.size(); ++second) {
            FruitCat& b = cats_[second];
            if (b.state != CatState::Alive) {
                continue;
            }

            Vec3 difference{b.position.x - a.position.x, b.position.y - a.position.y, b.position.z - a.position.z};
            const float distanceSquared = lengthSquared(difference);
            const float minimumDistance = a.radius + b.radius;
            if (distanceSquared > minimumDistance * minimumDistance) {
                continue;
            }

            const float distance = std::sqrt(std::max(distanceSquared, 0.0001F));
            const Vec3 normal{difference.x / distance, difference.y / distance, difference.z / distance};
            const float overlap = minimumDistance - distance;
            a.position.x -= normal.x * overlap * 0.5F;
            a.position.y -= normal.y * overlap * 0.5F;
            a.position.z -= normal.z * overlap * 0.5F;
            b.position.x += normal.x * overlap * 0.5F;
            b.position.y += normal.y * overlap * 0.5F;
            b.position.z += normal.z * overlap * 0.5F;

            const Vec3 relativeVelocity{b.velocity.x - a.velocity.x, b.velocity.y - a.velocity.y, b.velocity.z - a.velocity.z};
            const float velocityAlongNormal = relativeVelocity.x * normal.x + relativeVelocity.y * normal.y + relativeVelocity.z * normal.z;
            if (velocityAlongNormal < 0.0F) {
                a.velocity.x += normal.x * velocityAlongNormal;
                a.velocity.y += normal.y * velocityAlongNormal;
                a.velocity.z += normal.z * velocityAlongNormal;
                b.velocity.x -= normal.x * velocityAlongNormal;
                b.velocity.y -= normal.y * velocityAlongNormal;
                b.velocity.z -= normal.z * velocityAlongNormal;
            }

            if (a.hitCooldown <= 0.0F) {
                applyImpact(a);
            }
            if (b.hitCooldown <= 0.0F) {
                applyImpact(b);
            }
        }
    }
}

void Simulation::applyImpact(FruitCat& cat) {
    cat.hitCooldown = HIT_COOLDOWN_SECONDS;
    ++cat.impacts;
    if (cat.impacts >= MAX_IMPACTS) {
        cat.state = CatState::Exploding;
        cat.stateTimer = EXPLOSION_SECONDS;
        cat.velocity = {0.0F, 0.0F, 0.0F};
    }
}

} // namespace fruitcat
