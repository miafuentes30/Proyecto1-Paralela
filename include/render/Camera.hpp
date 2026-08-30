#pragma once

namespace fruitcat {

class Camera {
public:
    // Half extents of the box the camera must keep fully in frame.
    void setSubject(float halfWidth, float halfHeight, float halfDepth);

    void update(float deltaSeconds);
    void apply(int viewportWidth, int viewportHeight);

private:
    // Pulls the eye as close as the window shape allows while still containing
    // the subject. Cheap enough to run whenever the window is resized.
    void reframe(float aspectRatio);

    float orbitAngleRadians_ = 0.0F;
    float angularSpeedRadians_ = 0.22F;

    float subjectHalfWidth_ = 8.0F;
    float subjectHalfHeight_ = 5.0F;
    float subjectHalfDepth_ = 8.0F;

    float orbitRadius_ = 26.0F;
    float height_ = 12.0F;
    float framedAspectRatio_ = 0.0F;
};

} // namespace fruitcat
