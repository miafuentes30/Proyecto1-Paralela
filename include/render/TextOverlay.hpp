#pragma once

namespace fruitcat {

// Draws one line of text over the scene using a built-in 5x7 bitmap font.
// Coordinates are in framebuffer pixels with the origin at the top-left, and
// pixelSize is how many screen pixels one font pixel covers.
// The call installs and restores its own orthographic projection, so it must
// run after the 3D pass and before the buffer swap.
void drawScreenText(int framebufferWidth, int framebufferHeight,
                    float x, float y, float pixelSize, const char* text);

} // namespace fruitcat
