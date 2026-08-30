#pragma once

namespace fruitcat {

// Fills the colour buffer with a vertical gradient so the space around the
// arena reads as a night sky instead of an empty buffer. Installs its own
// orthographic projection and leaves the depth buffer untouched.
void drawSkyGradient();

// Stars on an infinitely distant sphere: they turn with the camera but never
// come closer. Must run after the camera has set the modelview matrix.
void drawStarfield();

} // namespace fruitcat
