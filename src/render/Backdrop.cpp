#include "render/Backdrop.hpp"

#include <cmath>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

namespace fruitcat {
namespace {

constexpr float PI = 3.14159265358979323846F;
constexpr int STAR_COUNT = 2400;

// Any radius between the near and far planes works: the starfield ignores depth
// and never translates with the eye, so this only has to stay inside the frustum.
constexpr float STAR_SPHERE_RADIUS = 20.0F;

// Three buckets because glPointSize cannot change inside a glBegin block.
constexpr int SIZE_BUCKETS = 3;

struct Star {
    float x;
    float y;
    float z;
    float brightness;
};

// The same small LCG the simulation uses, kept local so the sky comes out
// identical on every run and on every machine.
unsigned int nextRandom(unsigned int& state) {
    state = state * 1664525U + 1013904223U;
    return state;
}

float randomUnit(unsigned int& state) {
    return static_cast<float>(nextRandom(state) % 100000U) / 100000.0F;
}

int sizeBucket(float brightness) {
    if (brightness > 0.80F) {
        return 2;
    }
    return brightness > 0.50F ? 1 : 0;
}

const std::vector<Star>& stars() {
    static const std::vector<Star> generated = [] {
        std::vector<Star> result;
        result.reserve(STAR_COUNT);
        unsigned int state = 987654321U;
        for (int index = 0; index < STAR_COUNT; ++index) {
            // The cosine of the polar angle is what has to be uniform; drawing
            // the angle itself would bunch the stars up around the poles.
            const float cosPolar = 2.0F * randomUnit(state) - 1.0F;
            const float sinPolar = std::sqrt(1.0F - cosPolar * cosPolar);
            const float azimuth = 2.0F * PI * randomUnit(state);
            // Squaring the brightness leaves a handful of bright stars over a
            // field of faint ones, which is what a real sky looks like.
            const float sample = randomUnit(state);
            result.push_back(Star{
                STAR_SPHERE_RADIUS * sinPolar * std::cos(azimuth),
                STAR_SPHERE_RADIUS * cosPolar,
                STAR_SPHERE_RADIUS * sinPolar * std::sin(azimuth),
                0.22F + 0.78F * sample * sample
            });
        }
        return result;
    }();
    return generated;
}

} // namespace

void drawSkyGradient() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glColor3f(0.015F, 0.016F, 0.030F);
    glVertex2f(0.0F, 0.0F);
    glVertex2f(1.0F, 0.0F);
    glColor3f(0.055F, 0.065F, 0.135F);
    glVertex2f(1.0F, 1.0F);
    glVertex2f(0.0F, 1.0F);
    glEnd();

    glColor3f(1.0F, 1.0F, 1.0F);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawStarfield() {
    float modelview[16] = {};
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    // Dropping the translation centres the sphere on the eye, so the stars turn
    // with the camera but never get nearer. That is what sells them as distant.
    modelview[12] = 0.0F;
    modelview[13] = 0.0F;
    modelview[14] = 0.0F;

    glPushMatrix();
    glLoadMatrixf(modelview);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float pointSizes[SIZE_BUCKETS] = {1.4F, 2.2F, 3.0F};
    const std::vector<Star>& field = stars();
    for (int bucket = 0; bucket < SIZE_BUCKETS; ++bucket) {
        glPointSize(pointSizes[bucket]);
        glBegin(GL_POINTS);
        for (const Star& star : field) {
            if (sizeBucket(star.brightness) != bucket) {
                continue;
            }
            glColor4f(1.0F, 0.98F, 0.92F, star.brightness);
            glVertex3f(star.x, star.y, star.z);
        }
        glEnd();
    }

    glPointSize(1.0F);
    glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}

} // namespace fruitcat
