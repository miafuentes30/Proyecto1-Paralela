#include "render/FruitCatRenderer.hpp"

#include "render/SpriteLibrary.hpp"

#include <algorithm>
#include <cmath>

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

namespace fruitcat {
namespace {

constexpr float PI = 3.14159265358979323846F;
constexpr int MAX_IMPACTS = 7;
// The sprite canvas is square and its artwork does not fill the whole frame,
// so the quad is drawn wider than the collision sphere to make the cat read
// at roughly the size the physics radius suggests.
constexpr float CAT_SPRITE_SCALE = 2.1F;
constexpr float EXPLOSION_START_SCALE = 2.0F;
constexpr float EXPLOSION_END_SCALE = 4.6F;
constexpr float EXPLOSION_SECONDS = 0.50F;

struct Basis {
    Vec3 right;
    Vec3 up;
};

// The camera's world-space axes are the rows of the modelview rotation block,
// which is exactly what a screen-aligned billboard needs. Reading the matrix
// once per frame keeps this off the per-cat path.
Basis cameraBasis() {
    float modelview[16] = {};
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    return Basis{
        {modelview[0], modelview[4], modelview[8]},
        {modelview[1], modelview[5], modelview[9]}
    };
}

// Emits one quad centred on the sprite's position. Must be called between
// glBegin(GL_QUADS) and glEnd() so a whole batch travels as a single block.
void emitBillboard(const Vec3& center, float halfSize, const Basis& basis) {
    const Vec3 right{basis.right.x * halfSize, basis.right.y * halfSize, basis.right.z * halfSize};
    const Vec3 up{basis.up.x * halfSize, basis.up.y * halfSize, basis.up.z * halfSize};

    glTexCoord2f(0.0F, 0.0F);
    glVertex3f(center.x - right.x + up.x, center.y - right.y + up.y, center.z - right.z + up.z);
    glTexCoord2f(0.0F, 1.0F);
    glVertex3f(center.x - right.x - up.x, center.y - right.y - up.y, center.z - right.z - up.z);
    glTexCoord2f(1.0F, 1.0F);
    glVertex3f(center.x + right.x - up.x, center.y + right.y - up.y, center.z + right.z - up.z);
    glTexCoord2f(1.0F, 0.0F);
    glVertex3f(center.x + right.x + up.x, center.y + right.y + up.y, center.z + right.z + up.z);
}

// Damage darkens the sprite towards red. The texture is modulated by this
// colour, so the artwork keeps its shading while the cat visibly heats up.
void damageTint(const FruitCat& cat, float& red, float& green, float& blue) {
    const float damage = std::min(1.0F, static_cast<float>(cat.impacts) / static_cast<float>(MAX_IMPACTS));
    red = 1.0F;
    green = 1.0F - damage * 0.55F;
    blue = 1.0F - damage * 0.70F;
}

void drawShadows(const std::vector<FruitCat>& cats, float floorHeight) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(0.0F, 0.0F, 0.0F, 0.22F);

    const float shadowHeight = floorHeight + 0.025F;
    for (const FruitCat& cat : cats) {
        if (cat.state != CatState::Alive) {
            continue;
        }

        const float heightOverFloor = std::max(0.0F, cat.position.y - floorHeight - cat.radius);
        const float centerX = cat.position.x + heightOverFloor * 0.42F;
        const float centerZ = cat.position.z - heightOverFloor * 0.30F;
        const float radius = cat.radius * (1.15F + heightOverFloor * 0.22F);

        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(centerX, shadowHeight, centerZ);
        constexpr int segments = 16;
        for (int index = 0; index <= segments; ++index) {
            const float angle = 2.0F * PI * static_cast<float>(index) / static_cast<float>(segments);
            glVertex3f(centerX + std::cos(angle) * radius, shadowHeight, centerZ + std::sin(angle) * radius * 0.55F);
        }
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// Alive cats are drawn opaque with an alpha test rather than with blending:
// the artwork has hard edges, so discarding transparent texels lets the depth
// buffer sort the whole flock correctly without sorting it on the CPU.
void drawAliveCats(const std::vector<FruitCat>& cats, const Basis& basis, FruitType fruitType) {
    const unsigned int texture = spriteTexture(catSprite(fruitType));
    if (texture == 0) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_QUADS);
    for (const FruitCat& cat : cats) {
        if (cat.state != CatState::Alive || cat.fruitType != fruitType) {
            continue;
        }
        float red = 1.0F;
        float green = 1.0F;
        float blue = 1.0F;
        damageTint(cat, red, green, blue);
        glColor4f(red, green, blue, 1.0F);
        emitBillboard(cat.position, cat.radius * CAT_SPRITE_SCALE, basis);
    }
    glEnd();
}

// Explosions expand and fade, so they need real blending and must come after
// every opaque cat with depth writes disabled.
void drawExplosions(const std::vector<FruitCat>& cats, const Basis& basis, FruitType fruitType) {
    const unsigned int texture = spriteTexture(explosionSprite(fruitType));
    if (texture == 0) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_QUADS);
    for (const FruitCat& cat : cats) {
        if (cat.state != CatState::Exploding || cat.fruitType != fruitType) {
            continue;
        }
        const float progress = std::clamp(1.0F - cat.stateTimer / EXPLOSION_SECONDS, 0.0F, 1.0F);
        const float scale = EXPLOSION_START_SCALE + (EXPLOSION_END_SCALE - EXPLOSION_START_SCALE) * progress;
        // Hold full brightness for the first instants, then fade out.
        const float fade = std::clamp(1.0F - (progress - 0.25F) / 0.75F, 0.0F, 1.0F);
        glColor4f(1.0F, 1.0F, 1.0F, fade);
        emitBillboard(cat.position, cat.radius * scale, basis);
    }
    glEnd();
}

} // namespace

void drawFruitCats(const std::vector<FruitCat>& cats, float floorHeight) {
    const Basis basis = cameraBasis();

    drawShadows(cats, floorHeight);

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_ALPHA_TEST);
    // Below one half the mip chain has already blurred the silhouette away;
    // this threshold keeps distant cats from thinning out.
    glAlphaFunc(GL_GREATER, 0.40F);
    drawAliveCats(cats, basis, FruitType::Banana);
    drawAliveCats(cats, basis, FruitType::Avocado);

    glAlphaFunc(GL_GREATER, 0.02F);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawExplosions(cats, basis, FruitType::Banana);
    drawExplosions(cats, basis, FruitType::Avocado);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);

    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
}

} // namespace fruitcat
