#include "render/Arena.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>

namespace fruitcat {
namespace {

struct Face {
    float normal[3];
    float vertices[4][3];
};

void drawFace(const Face& face) {
    glNormal3fv(face.normal);
    glBegin(GL_QUADS);
    for (const auto& vertex : face.vertices) {
        glVertex3fv(vertex);
    }
    glEnd();
}

// Six panels of the box, each with an outward-facing normal so the
// fixed-function lighting can produce a distinct specular highlight per pane.
void drawPanels(float halfWidth, float halfHeight, float halfDepth) {
    const float hw = halfWidth;
    const float hh = halfHeight;
    const float hd = halfDepth;

    const Face panels[6] = {
        // Front (+Z)
        {{0.0F, 0.0F, 1.0F}, {{-hw, -hh, hd}, {hw, -hh, hd}, {hw, hh, hd}, {-hw, hh, hd}}},
        // Back (-Z)
        {{0.0F, 0.0F, -1.0F}, {{hw, -hh, -hd}, {-hw, -hh, -hd}, {-hw, hh, -hd}, {hw, hh, -hd}}},
        // Left (-X)
        {{-1.0F, 0.0F, 0.0F}, {{-hw, -hh, -hd}, {-hw, -hh, hd}, {-hw, hh, hd}, {-hw, hh, -hd}}},
        // Right (+X)
        {{1.0F, 0.0F, 0.0F}, {{hw, -hh, hd}, {hw, -hh, -hd}, {hw, hh, -hd}, {hw, hh, hd}}},
        // Top (+Y)
        {{0.0F, 1.0F, 0.0F}, {{-hw, hh, hd}, {hw, hh, hd}, {hw, hh, -hd}, {-hw, hh, -hd}}},
        // Bottom (-Y)
        {{0.0F, -1.0F, 0.0F}, {{-hw, -hh, -hd}, {hw, -hh, -hd}, {hw, -hh, hd}, {-hw, -hh, hd}}},
    };

    for (const auto& panel : panels) {
        drawFace(panel);
    }
}

void drawEdges(float halfWidth, float halfHeight, float halfDepth) {
    const float hw = halfWidth;
    const float hh = halfHeight;
    const float hd = halfDepth;

    const float corners[8][3] = {
        {-hw, -hh, -hd}, {hw, -hh, -hd}, {hw, hh, -hd}, {-hw, hh, -hd},
        {-hw, -hh, hd}, {hw, -hh, hd}, {hw, hh, hd}, {-hw, hh, hd},
    };

    constexpr int edgeIndices[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // back face
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // front face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}, // connectors
    };

    glBegin(GL_LINES);
    for (const auto& edge : edgeIndices) {
        glVertex3fv(corners[edge[0]]);
        glVertex3fv(corners[edge[1]]);
    }
    glEnd();
}

} // namespace

void drawGlassArena(float halfWidth, float halfHeight, float halfDepth) {
    // Edges are drawn opaque and depth-written first so they read as a crisp
    // frame around the glass, matching the reference image.
    glDisable(GL_LIGHTING);
    glColor3f(0.55F, 0.85F, 0.95F);
    glLineWidth(1.5F);
    drawEdges(halfWidth, halfHeight, halfDepth);

    // Glass panels: lit so a moving specular highlight sells the material,
    // blended for transparency, and depth-write disabled so the far and near
    // panes stay visible through each other instead of occluding. Alpha is
    // kept low so the panels read as near-invisible glass, not tinted walls.
    const float glassColor[4] = {0.55F, 0.75F, 0.85F, 0.07F};
    const float specularColor[4] = {1.0F, 1.0F, 1.0F, 1.0F};

    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, glassColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 110.0F);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);

    // A convex box only needs two passes to get a reasonable back-to-front
    // approximation: far panels (back-facing) first, then near panels.
    glCullFace(GL_FRONT);
    drawPanels(halfWidth, halfHeight, halfDepth);

    glCullFace(GL_BACK);
    drawPanels(halfWidth, halfHeight, halfDepth);

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
}

} // namespace fruitcat
