#include "render/Camera.hpp"

#include <algorithm>
#include <cmath>

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

namespace fruitcat {
namespace {

constexpr float PI = 3.14159265358979323846F;
constexpr float FIELD_OF_VIEW_RADIANS = 60.0F * PI / 180.0F;

// Eye height as a fraction of the orbit radius. Fixing the ratio keeps the
// three-quarter view angle identical at every distance, so reframing only ever
// zooms; it never tilts the composition.
constexpr float ELEVATION_RATIO = 0.46F;

// A sliver of air between the glass and the window edge.
constexpr float FRAMING_MARGIN = 1.03F;

// The whole orbit is sampled once per window shape and the tightest distance
// that works for every angle is kept, so the arena does not breathe in and out
// while the camera turns.
constexpr int ORBIT_SAMPLES = 120;

struct Vector3 {
    float x;
    float y;
    float z;
};

Vector3 subtract(const Vector3& left, const Vector3& right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

float dot(const Vector3& left, const Vector3& right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vector3 normalize(const Vector3& vector) {
    const float length = std::sqrt(dot(vector, vector));
    return {vector.x / length, vector.y / length, vector.z / length};
}

Vector3 cross(const Vector3& left, const Vector3& right) {
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

void applyLookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
    const Vector3 forward = normalize(subtract(target, eye));
    const Vector3 side = normalize(cross(forward, up));
    const Vector3 correctedUp = cross(side, forward);

    const float viewMatrix[16] = {
        side.x, correctedUp.x, -forward.x, 0.0F,
        side.y, correctedUp.y, -forward.y, 0.0F,
        side.z, correctedUp.z, -forward.z, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };

    glMultMatrixf(viewMatrix);
    glTranslatef(-eye.x, -eye.y, -eye.z);
}

} // namespace

void Camera::setSubject(float halfWidth, float halfHeight, float halfDepth) {
    subjectHalfWidth_ = halfWidth;
    subjectHalfHeight_ = halfHeight;
    subjectHalfDepth_ = halfDepth;
    // Force the next apply() to recompute against the new subject.
    framedAspectRatio_ = 0.0F;
}

void Camera::update(float deltaSeconds) {
    orbitAngleRadians_ = std::fmod(
        orbitAngleRadians_ + angularSpeedRadians_ * deltaSeconds,
        2.0F * PI
    );
}

void Camera::reframe(float aspectRatio) {
    const float tanVertical = std::tan(FIELD_OF_VIEW_RADIANS / 2.0F);
    const float tanHorizontal = tanVertical * aspectRatio;
    const float inverseLength = 1.0F / std::sqrt(1.0F + ELEVATION_RATIO * ELEVATION_RATIO);

    float eyeDistance = 0.0F;
    for (int sample = 0; sample < ORBIT_SAMPLES; ++sample) {
        const float angle = 2.0F * PI * static_cast<float>(sample) / static_cast<float>(ORBIT_SAMPLES);
        const Vector3 forward{
            -std::sin(angle) * inverseLength,
            -ELEVATION_RATIO * inverseLength,
            -std::cos(angle) * inverseLength
        };
        const Vector3 side = normalize(cross(forward, {0.0F, 1.0F, 0.0F}));
        const Vector3 up = cross(side, forward);

        for (int corner = 0; corner < 8; ++corner) {
            const Vector3 point{
                (corner & 1) != 0 ? subjectHalfWidth_ : -subjectHalfWidth_,
                (corner & 2) != 0 ? subjectHalfHeight_ : -subjectHalfHeight_,
                (corner & 4) != 0 ? subjectHalfDepth_ : -subjectHalfDepth_
            };
            // Sliding the eye along its fixed direction only shifts a corner's
            // view-space Z; its X and Y never move. The closest eye that still
            // holds the corner inside the frustum is therefore a closed form
            // rather than a search.
            const float needed = -dot(forward, point) + std::max(
                std::fabs(dot(side, point)) / tanHorizontal,
                std::fabs(dot(up, point)) / tanVertical
            );
            eyeDistance = std::max(eyeDistance, needed);
        }
    }

    eyeDistance *= FRAMING_MARGIN;
    orbitRadius_ = eyeDistance * inverseLength;
    height_ = orbitRadius_ * ELEVATION_RATIO;
    framedAspectRatio_ = aspectRatio;
}

void Camera::apply(int viewportWidth, int viewportHeight) {
    const int safeWidth = viewportWidth > 0 ? viewportWidth : 1;
    const int safeHeight = viewportHeight > 0 ? viewportHeight : 1;
    const float aspectRatio = static_cast<float>(safeWidth) / static_cast<float>(safeHeight);
    if (std::fabs(aspectRatio - framedAspectRatio_) > 0.001F) {
        reframe(aspectRatio);
    }

    constexpr float nearPlane = 0.1F;
    // A very tall or very narrow window pushes the eye far back, so the far
    // plane has to follow it instead of sitting at a fixed depth.
    const float subjectRadius = std::sqrt(
        subjectHalfWidth_ * subjectHalfWidth_
        + subjectHalfHeight_ * subjectHalfHeight_
        + subjectHalfDepth_ * subjectHalfDepth_
    );
    const float eyeDistance = std::sqrt(orbitRadius_ * orbitRadius_ + height_ * height_);
    const float farPlane = eyeDistance + subjectRadius + 10.0F;

    const float top = nearPlane * std::tan(FIELD_OF_VIEW_RADIANS / 2.0F);
    const float right = top * aspectRatio;

    glViewport(0, 0, safeWidth, safeHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-right, right, -top, top, nearPlane, farPlane);

    const Vector3 eye{
        orbitRadius_ * std::sin(orbitAngleRadians_),
        height_,
        orbitRadius_ * std::cos(orbitAngleRadians_)
    };

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    applyLookAt(eye, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
}

} // namespace fruitcat
