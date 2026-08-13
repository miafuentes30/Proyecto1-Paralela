#include "render/DebugSphere.hpp"

#include <cmath>
#include <initializer_list>

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

namespace fruitcat {
namespace {

constexpr float PI = 3.14159265358979323846F;

void drawShadowEllipse(float x, float z, float radiusX, float radiusZ, float alpha) {
    glColor4f(0.0F, 0.0F, 0.0F, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, 0.0F, z);
    constexpr int segments = 36;
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = 2.0F * PI * static_cast<float>(segment) / static_cast<float>(segments);
        glVertex3f(
            x + std::cos(angle) * radiusX,
            0.0F,
            z + std::sin(angle) * radiusZ
        );
    }
    glEnd();
}

void drawDirectionalSoftShadow(float x, float z, float heightAboveFloor, float radius) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // The point lamp is above-left-front at (-10, 10, +8), so the shadow is
    // cast away from it: toward +X and -Z. Height makes it longer and softer.
    const float offsetX = heightAboveFloor * 0.67F;
    const float offsetZ = -heightAboveFloor * 0.43F;
    drawShadowEllipse(x + offsetX, z + offsetZ, radius * 1.75F, radius * 0.72F, 0.13F);
    drawShadowEllipse(x + offsetX * 0.78F, z + offsetZ * 0.78F, radius * 1.10F, radius * 0.45F, 0.18F);
    drawShadowEllipse(x + offsetX * 0.58F, z + offsetZ * 0.58F, radius * 0.48F, radius * 0.25F, 0.38F);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawUvSphere(float radius) {
    constexpr int latitudeBands = 18;
    constexpr int longitudeBands = 24;

    for (int latitude = 0; latitude < latitudeBands; ++latitude) {
        const float phi0 = PI * static_cast<float>(latitude) / static_cast<float>(latitudeBands);
        const float phi1 = PI * static_cast<float>(latitude + 1) / static_cast<float>(latitudeBands);

        glBegin(GL_QUAD_STRIP);
        for (int longitude = 0; longitude <= longitudeBands; ++longitude) {
            const float theta = 2.0F * PI * static_cast<float>(longitude) / static_cast<float>(longitudeBands);
            for (const float phi : {phi0, phi1}) {
                const float x = std::sin(phi) * std::cos(theta);
                const float y = std::cos(phi);
                const float z = std::sin(phi) * std::sin(theta);
                glNormal3f(x, y, z);
                glVertex3f(radius * x, radius * y, radius * z);
            }
        }
        glEnd();
    }
}

} // namespace

void drawDebugSphere(float /*timeSeconds*/, float floorHeight) {
    constexpr float radius = 0.85F;
    constexpr float x = 0.0F;
    constexpr float z = 0.0F;
    constexpr float heightAboveFloor = 1.15F;

    glPushMatrix();
    glTranslatef(0.0F, floorHeight + 0.025F, 0.0F);
    drawDirectionalSoftShadow(x, z, heightAboveFloor, radius);
    glPopMatrix();

    const float diffuseColor[4] = {1.0F, 0.34F, 0.06F, 1.0F};
    const float ambientColor[4] = {0.35F, 0.08F, 0.01F, 1.0F};
    const float specularColor[4] = {1.0F, 0.78F, 0.42F, 1.0F};

    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 48.0F);

    glPushMatrix();
    glTranslatef(x, floorHeight + radius + heightAboveFloor, z);
    drawUvSphere(radius);
    glPopMatrix();

    glDisable(GL_LIGHTING);
}

} // namespace fruitcat
