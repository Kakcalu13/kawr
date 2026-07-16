// kawr — stroke and shape rendering. Apache-2.0.
#include "Shape.h"

#include "GL.h"

void Stroke::draw(bool inProgress) const {
    if (pts.size() < 2) {
        // Lone starting dot so the click reads as registered.
        if (pts.size() == 1) {
            glPointSize(6.0f);
            glColor3f(1.0f, 0.95f, 0.6f);
            glBegin(GL_POINTS);
            glVertex3f(float(pts[0].x), float(pts[0].y) + 0.01f, float(pts[0].z));
            glEnd();
        }
        return;
    }

    glLineWidth(inProgress ? 2.5f : 2.0f);
    glColor3f(1.0f, 0.93f, 0.55f);  // warm yellow while open
    glBegin(GL_LINE_STRIP);
    for (const Vec3& p : pts)
        glVertex3f(float(p.x), float(p.y) + 0.01f, float(p.z));
    glEnd();
}

Vec3 Shape::centroid() const {
    Vec3 c;
    for (const Vec3& p : loop) c = c + p;
    if (!loop.empty()) c = c * (1.0 / double(loop.size()));
    return c;
}

void Shape::draw() const {
    const std::vector<Vec3>& f = loop;
    size_t n = f.size();
    if (n < 3) return;

    Vec3 cf = centroid();
    Vec3 cb = cf + offset;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);  // translucent: test depth but don't write it

    // Front cap.
    glColor4f(LAVENDER[0], LAVENDER[1], LAVENDER[2], 0.20f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(float(cf.x), float(cf.y) + 0.005f, float(cf.z));
    for (const Vec3& p : f)
        glVertex3f(float(p.x), float(p.y) + 0.005f, float(p.z));
    glEnd();

    // Back cap.
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(float(cb.x), float(cb.y) + 0.005f, float(cb.z));
    for (const Vec3& p : f) {
        Vec3 b = p + offset;
        glVertex3f(float(b.x), float(b.y) + 0.005f, float(b.z));
    }
    glEnd();

    // Connecting side band — one quad per edge, spanning front to back.
    glColor4f(LAVENDER[0], LAVENDER[1], LAVENDER[2], 0.34f);
    glBegin(GL_QUADS);
    for (size_t i = 0; i + 1 < n; ++i) {
        Vec3 a = f[i], b = f[i + 1];
        Vec3 a2 = a + offset, b2 = b + offset;
        glVertex3f(float(a.x), float(a.y), float(a.z));
        glVertex3f(float(b.x), float(b.y), float(b.z));
        glVertex3f(float(b2.x), float(b2.y), float(b2.z));
        glVertex3f(float(a2.x), float(a2.y), float(a2.z));
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Front and back outlines.
    glLineWidth(3.0f);
    glColor3f(LAVENDER[0], LAVENDER[1], LAVENDER[2]);
    glBegin(GL_LINE_STRIP);
    for (const Vec3& p : f)
        glVertex3f(float(p.x), float(p.y) + 0.01f, float(p.z));
    glEnd();
    glBegin(GL_LINE_STRIP);
    for (const Vec3& p : f) {
        Vec3 b = p + offset;
        glVertex3f(float(b.x), float(b.y) + 0.01f, float(b.z));
    }
    glEnd();

    // Vertical seams linking each pair of corresponding vertices.
    glLineWidth(1.2f);
    glColor3f(LAVENDER[0] * 0.75f, LAVENDER[1] * 0.75f, LAVENDER[2] * 0.85f);
    glBegin(GL_LINES);
    for (size_t i = 0; i + 1 < n; ++i) {
        Vec3 a = f[i];
        Vec3 b = a + offset;
        glVertex3f(float(a.x), float(a.y) + 0.01f, float(a.z));
        glVertex3f(float(b.x), float(b.y) + 0.01f, float(b.z));
    }
    glEnd();
}
