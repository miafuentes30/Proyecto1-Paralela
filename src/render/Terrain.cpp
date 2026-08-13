#include "render/Terrain.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace fruitcat {
namespace {

constexpr char GRASS_TEXTURE_PATH[] = "assets/textures/grass.png";
constexpr float TEXTURE_TILES = 6.0F;

unsigned int loadGrassTexture() {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(GRASS_TEXTURE_PATH, &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        std::fprintf(stderr, "No se pudo cargar la textura: %s\n", GRASS_TEXTURE_PATH);
        return 0;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    stbi_image_free(pixels);
    return texture;
}

unsigned int grassTexture() {
    static const unsigned int texture = loadGrassTexture();
    return texture;
}

} // namespace

void drawTerrain(float halfWidth, float halfDepth, float height) {
    // The texture is multiplied by this material, keeping it responsive to
    // the scene light while preserving a saturated grass palette.
    const float diffuseColor[4] = {0.92F, 1.0F, 0.88F, 1.0F};
    const float ambientColor[4] = {0.45F, 0.62F, 0.45F, 1.0F};
    const float specularColor[4] = {0.12F, 0.26F, 0.14F, 1.0F};
    // A small base emission makes the grass readable against the black void;
    // directional light still adds the brighter warm-side variation.
    const float emissionColor[4] = {0.035F, 0.10F, 0.035F, 1.0F};

    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissionColor);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10.0F);

    const unsigned int texture = grassTexture();
    if (texture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    glBegin(GL_QUADS);
    glNormal3f(0.0F, 1.0F, 0.0F);
    glTexCoord2f(0.0F, 0.0F);
    glVertex3f(-halfWidth, height, -halfDepth);
    glTexCoord2f(TEXTURE_TILES, 0.0F);
    glVertex3f(halfWidth, height, -halfDepth);
    glTexCoord2f(TEXTURE_TILES, TEXTURE_TILES);
    glVertex3f(halfWidth, height, halfDepth);
    glTexCoord2f(0.0F, TEXTURE_TILES);
    glVertex3f(-halfWidth, height, halfDepth);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    const float noEmission[4] = {0.0F, 0.0F, 0.0F, 1.0F};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmission);
    glDisable(GL_LIGHTING);
}

} // namespace fruitcat
