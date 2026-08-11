#include "render/DebugCube.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

namespace fruitcat {

void drawDebugCube(float halfSize) {
    const float negative = -halfSize;
    const float positive = halfSize;

    glBegin(GL_QUADS);

    // Front face
    glColor3f(0.20F, 0.85F, 0.95F);
    glVertex3f(negative, negative, positive);
    glVertex3f(positive, negative, positive);
    glVertex3f(positive, positive, positive);
    glVertex3f(negative, positive, positive);

    // Back face
    glColor3f(0.08F, 0.35F, 0.65F);
    glVertex3f(positive, negative, negative);
    glVertex3f(negative, negative, negative);
    glVertex3f(negative, positive, negative);
    glVertex3f(positive, positive, negative);

    // Left face
    glColor3f(0.15F, 0.60F, 0.80F);
    glVertex3f(negative, negative, negative);
    glVertex3f(negative, negative, positive);
    glVertex3f(negative, positive, positive);
    glVertex3f(negative, positive, negative);

    // Right face
    glColor3f(0.25F, 0.95F, 0.75F);
    glVertex3f(positive, negative, positive);
    glVertex3f(positive, negative, negative);
    glVertex3f(positive, positive, negative);
    glVertex3f(positive, positive, positive);

    // Top face
    glColor3f(0.65F, 0.95F, 1.00F);
    glVertex3f(negative, positive, positive);
    glVertex3f(positive, positive, positive);
    glVertex3f(positive, positive, negative);
    glVertex3f(negative, positive, negative);

    // Bottom face
    glColor3f(0.05F, 0.20F, 0.35F);
    glVertex3f(negative, negative, negative);
    glVertex3f(positive, negative, negative);
    glVertex3f(positive, negative, positive);
    glVertex3f(negative, negative, positive);

    glEnd();
}

} // namespace fruitcat
