#include "render/SpriteLibrary.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

#include <algorithm>
#include <cstdio>
#include <vector>

#include <stb_image.h>

namespace fruitcat {
namespace {

constexpr int SPRITE_COUNT = 4;

const char* spritePath(Sprite sprite) {
    switch (sprite) {
        case Sprite::BananaCat: return "assets/sprites/banana_cat.png";
        case Sprite::AvocadoCat: return "assets/sprites/avocado_cat.png";
        case Sprite::BananaExplosion: return "assets/sprites/banana_explosion.png";
        case Sprite::AvocadoExplosion: return "assets/sprites/avocado_explosion.png";
    }
    return "";
}

// Averages a 2x2 block weighting each texel by its alpha. The chroma-key
// background is transparent but still carries its original RGB, so a plain
// average would bleed that colour into the silhouette on the smaller levels.
void downsample(const std::vector<unsigned char>& source, int width, int height,
                std::vector<unsigned char>& target, int& targetWidth, int& targetHeight) {
    targetWidth = width > 1 ? width / 2 : 1;
    targetHeight = height > 1 ? height / 2 : 1;
    target.assign(static_cast<std::size_t>(targetWidth) * targetHeight * 4, 0);

    for (int y = 0; y < targetHeight; ++y) {
        for (int x = 0; x < targetWidth; ++x) {
            int red = 0;
            int green = 0;
            int blue = 0;
            int alpha = 0;
            int alphaWeight = 0;
            for (int offsetY = 0; offsetY < 2; ++offsetY) {
                for (int offsetX = 0; offsetX < 2; ++offsetX) {
                    const int sampleX = std::min(2 * x + offsetX, width - 1);
                    const int sampleY = std::min(2 * y + offsetY, height - 1);
                    const std::size_t index = (static_cast<std::size_t>(sampleY) * width + sampleX) * 4;
                    const int sampleAlpha = source[index + 3];
                    red += source[index + 0] * sampleAlpha;
                    green += source[index + 1] * sampleAlpha;
                    blue += source[index + 2] * sampleAlpha;
                    alpha += sampleAlpha;
                    alphaWeight += sampleAlpha;
                }
            }

            const std::size_t out = (static_cast<std::size_t>(y) * targetWidth + x) * 4;
            if (alphaWeight > 0) {
                target[out + 0] = static_cast<unsigned char>(red / alphaWeight);
                target[out + 1] = static_cast<unsigned char>(green / alphaWeight);
                target[out + 2] = static_cast<unsigned char>(blue / alphaWeight);
            }
            target[out + 3] = static_cast<unsigned char>(alpha / 4);
        }
    }
}

unsigned int uploadSprite(Sprite sprite) {
    const char* path = spritePath(sprite);
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        std::fprintf(stderr, "No se pudo cargar el sprite: %s\n", path);
        return 0;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    // Magnification stays nearest so the pixel art keeps hard edges up close;
    // minification uses the mip chain because a 1254px sprite covering ~60px
    // on screen would otherwise shimmer badly as the camera orbits.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Mip levels are built by hand instead of with GL_GENERATE_MIPMAP so the
    // loader stays within OpenGL 1.1 on every platform the project targets.
    std::vector<unsigned char> level(pixels, pixels + static_cast<std::size_t>(width) * height * 4);
    stbi_image_free(pixels);

    int levelWidth = width;
    int levelHeight = height;
    int levelIndex = 0;
    std::vector<unsigned char> smaller;
    while (true) {
        glTexImage2D(GL_TEXTURE_2D, levelIndex, GL_RGBA, levelWidth, levelHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, level.data());
        if (levelWidth == 1 && levelHeight == 1) {
            break;
        }
        int smallerWidth = 0;
        int smallerHeight = 0;
        downsample(level, levelWidth, levelHeight, smaller, smallerWidth, smallerHeight);
        level.swap(smaller);
        levelWidth = smallerWidth;
        levelHeight = smallerHeight;
        ++levelIndex;
    }

    return texture;
}

} // namespace

Sprite catSprite(FruitType fruitType) {
    return fruitType == FruitType::Banana ? Sprite::BananaCat : Sprite::AvocadoCat;
}

Sprite explosionSprite(FruitType fruitType) {
    return fruitType == FruitType::Banana ? Sprite::BananaExplosion : Sprite::AvocadoExplosion;
}

unsigned int spriteTexture(Sprite sprite) {
    static unsigned int textures[SPRITE_COUNT] = {};
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        for (int index = 0; index < SPRITE_COUNT; ++index) {
            textures[index] = uploadSprite(static_cast<Sprite>(index));
        }
    }
    return textures[static_cast<int>(sprite)];
}

} // namespace fruitcat
