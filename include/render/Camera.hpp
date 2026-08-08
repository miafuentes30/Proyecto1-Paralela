#pragma once

namespace fruitcat {

class Camera {
public:
    void update(float deltaSeconds);
    void apply(int viewportWidth, int viewportHeight) const;

private:
    float orbitAngleRadians_ = 0.0F;
    float orbitRadius_ = 26.0F;
    float height_ = 12.0F;
    float angularSpeedRadians_ = 0.22F;
};

} // namespace fruitcat
