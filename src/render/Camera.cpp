#include "render/Camera.hpp"

#include <cmath>

#include <windows.h>
#include <gl/GL.h>

namespace fruitcat {
namespace {

constexpr float PI = 3.14159265358979323846F;

struct Vector3 {
    float x;
    float y;
    float z;
};

Vector3 subtract(const Vector3& left, const Vector3& right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vector3 normalize(const Vector3& vector) {
    const float length = std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
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

void Camera::update(float deltaSeconds) {
    orbitAngleRadians_ = std::fmod(
        orbitAngleRadians_ + angularSpeedRadians_ * deltaSeconds,
        2.0F * PI
    );
}

void Camera::apply(int viewportWidth, int viewportHeight) const {
    const int safeHeight = viewportHeight > 0 ? viewportHeight : 1;
    const float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(safeHeight);
    constexpr float nearPlane = 0.1F;
    constexpr float farPlane = 100.0F;
    constexpr float fieldOfViewRadians = 60.0F * PI / 180.0F;
    const float top = nearPlane * std::tan(fieldOfViewRadians / 2.0F);
    const float right = top * aspectRatio;

    glViewport(0, 0, viewportWidth, safeHeight);
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
