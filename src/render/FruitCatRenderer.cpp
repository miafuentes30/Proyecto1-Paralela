#include "render/FruitCatRenderer.hpp"

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

void drawShadowEllipse(float x, float z, float radius, float alpha) {
    glColor4f(0.0F, 0.0F, 0.0F, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, 0.0F, z);
    constexpr int segments = 20;
    for (int index = 0; index <= segments; ++index) {
        const float angle = 2.0F * PI * static_cast<float>(index) / static_cast<float>(segments);
        glVertex3f(x + std::cos(angle) * radius, 0.0F, z + std::sin(angle) * radius * 0.55F);
    }
    glEnd();
}

void drawSphere(float radius) {
    constexpr int latitudeBands = 10;
    constexpr int longitudeBands = 14;
    for (int latitude = 0; latitude < latitudeBands; ++latitude) {
        const float phi0 = PI * static_cast<float>(latitude) / static_cast<float>(latitudeBands);
        const float phi1 = PI * static_cast<float>(latitude + 1) / static_cast<float>(latitudeBands);
        glBegin(GL_QUAD_STRIP);
        for (int longitude = 0; longitude <= longitudeBands; ++longitude) {
            const float theta = 2.0F * PI * static_cast<float>(longitude) / static_cast<float>(longitudeBands);
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);
            for (const float phi : {phi0, phi1}) {
                const float x = std::sin(phi) * cosTheta;
                const float y = std::cos(phi);
                const float z = std::sin(phi) * sinTheta;
                glNormal3f(x, y, z);
                glVertex3f(radius * x, radius * y, radius * z);
            }
        }
        glEnd();
    }
}

void baseColor(const FruitCat& cat, float& red, float& green, float& blue) {
    if (cat.fruitType == FruitType::Banana) {
        red = 1.0F;
        green = 0.78F;
        blue = 0.06F;
    } else {
        red = 0.26F;
        green = 0.82F;
        blue = 0.22F;
    }

    const float damage = std::min(1.0F, static_cast<float>(cat.impacts) / static_cast<float>(MAX_IMPACTS));
    red = red * (1.0F - damage * 0.45F) + damage * 0.95F;
    green *= 1.0F - damage * 0.60F;
    blue *= 1.0F - damage * 0.55F;
}

void drawExplosion(const FruitCat& cat) {
    const float progress = std::clamp(1.0F - cat.stateTimer / 0.50F, 0.0F, 1.0F);
    float red = 1.0F;
    float green = cat.fruitType == FruitType::Banana ? 0.62F : 0.92F;
    float blue = cat.fruitType == FruitType::Banana ? 0.08F : 0.16F;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(red, green, blue, 0.78F * (1.0F - progress));
    glPushMatrix();
    glTranslatef(cat.position.x, cat.position.y, cat.position.z);
    drawSphere(cat.radius * (1.0F + progress * 2.2F));
    glPopMatrix();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace

void drawFruitCats(const std::vector<FruitCat>& cats, float floorHeight) {
    for (const FruitCat& cat : cats) {
        if (cat.state == CatState::Respawning) {
            continue;
        }
        if (cat.state == CatState::Exploding) {
            drawExplosion(cat);
            continue;
        }

        const float heightOverFloor = std::max(0.0F, cat.position.y - floorHeight - cat.radius);
        glPushMatrix();
        glTranslatef(0.0F, floorHeight + 0.025F, 0.0F);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        drawShadowEllipse(
            cat.position.x + heightOverFloor * 0.42F,
            cat.position.z - heightOverFloor * 0.30F,
            cat.radius * (1.15F + heightOverFloor * 0.22F),
            0.22F
        );
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glPopMatrix();

        float red = 0.0F;
        float green = 0.0F;
        float blue = 0.0F;
        baseColor(cat, red, green, blue);
        const float diffuse[4] = {red, green, blue, 1.0F};
        const float ambient[4] = {red * 0.34F, green * 0.34F, blue * 0.34F, 1.0F};
        const float specular[4] = {1.0F, 0.92F, 0.70F, 1.0F};

        glEnable(GL_LIGHTING);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 38.0F);
        glPushMatrix();
        glTranslatef(cat.position.x, cat.position.y, cat.position.z);
        drawSphere(cat.radius);
        glPopMatrix();
        glDisable(GL_LIGHTING);
    }
}

} // namespace fruitcat
