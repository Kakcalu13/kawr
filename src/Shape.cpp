// kawr — stroke and shape rendering. Apache-2.0.
#include "Shape.h"

#include <algorithm>
#include <cmath>

#include "GL.h"

// Even-odd point-in-polygon test on the XZ plane. `poly` may be closed
// (last == first); degenerate edges don't affect the crossing count.
static bool pointInPolyXZ(const std::vector<Vec3>& poly, double x, double z) {
    bool in = false;
    int n = int(poly.size());
    for (int i = 0, j = n - 1; i < n; j = i++) {
        double xi = poly[i].x, zi = poly[i].z;
        double xj = poly[j].x, zj = poly[j].z;
        if (((zi > z) != (zj > z)) &&
            (x < (xj - xi) * (z - zi) / (zj - zi) + xi))
            in = !in;
    }
    return in;
}

void Shape::buildMesh(int divisions) {
    meshVerts.clear();
    quadIdx.clear();
    if (loop.size() < 3) return;

    double minx = loop[0].x, maxx = loop[0].x;
    double minz = loop[0].z, maxz = loop[0].z;
    for (const Vec3& p : loop) {
        minx = std::min(minx, p.x); maxx = std::max(maxx, p.x);
        minz = std::min(minz, p.z); maxz = std::max(maxz, p.z);
    }
    double maxDim = std::max(maxx - minx, maxz - minz);
    if (maxDim < 1e-6) return;
    int div = std::max(1, divisions);
    double cell = std::max(maxDim / double(div), 0.02);

    int nx = int(std::ceil((maxx - minx) / cell)) + 1;
    int nz = int(std::ceil((maxz - minz) / cell)) + 1;
    if (nx < 2 || nz < 2) return;

    // Grid nodes strictly inside the outline become mesh vertices.
    std::vector<int> idx(size_t(nx) * size_t(nz), -1);
    for (int j = 0; j < nz; ++j) {
        for (int i = 0; i < nx; ++i) {
            double x = minx + i * cell;
            double z = minz + j * cell;
            if (pointInPolyXZ(loop, x, z)) {
                idx[j * nx + i] = int(meshVerts.size());
                meshVerts.push_back(Vec3(x, 0.0, z));
            }
        }
    }
    // A cell whose four corners are all inside becomes a quad.
    for (int j = 0; j + 1 < nz; ++j) {
        for (int i = 0; i + 1 < nx; ++i) {
            int a = idx[j * nx + i];
            int b = idx[j * nx + i + 1];
            int c = idx[(j + 1) * nx + i + 1];
            int d = idx[(j + 1) * nx + i];
            if (a >= 0 && b >= 0 && c >= 0 && d >= 0) {
                quadIdx.push_back(a); quadIdx.push_back(b);
                quadIdx.push_back(c); quadIdx.push_back(d);
            }
        }
    }
}

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);  // translucent: test depth but don't write it

    glColor4f(LAVENDER[0], LAVENDER[1], LAVENDER[2], 0.20f);
    if (!quadIdx.empty()) {
        // Lattice fill: front face, then back face (verts + offset).
        glBegin(GL_QUADS);
        for (size_t q = 0; q + 4 <= quadIdx.size(); q += 4)
            for (int k = 0; k < 4; ++k) {
                const Vec3& v = meshVerts[quadIdx[q + k]];
                glVertex3f(float(v.x), float(v.y) + 0.005f, float(v.z));
            }
        for (size_t q = 0; q + 4 <= quadIdx.size(); q += 4)
            for (int k = 0; k < 4; ++k) {
                Vec3 v = meshVerts[quadIdx[q + k]] + offset;
                glVertex3f(float(v.x), float(v.y) + 0.005f, float(v.z));
            }
        glEnd();

        // Faint lattice wireframe so it reads as a mesh.
        glColor4f(LAVENDER[0], LAVENDER[1], LAVENDER[2], 0.22f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        for (size_t q = 0; q + 4 <= quadIdx.size(); q += 4)
            for (int k = 0; k < 4; ++k) {
                const Vec3& v0 = meshVerts[quadIdx[q + k]];
                const Vec3& v1 = meshVerts[quadIdx[q + (k + 1) % 4]];
                glVertex3f(float(v0.x), float(v0.y) + 0.006f, float(v0.z));
                glVertex3f(float(v1.x), float(v1.y) + 0.006f, float(v1.z));
            }
        glEnd();
    } else {
        // Fallback for tiny/degenerate shapes with no interior cells.
        Vec3 cf = centroid();
        Vec3 cb = cf + offset;
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(float(cf.x), float(cf.y) + 0.005f, float(cf.z));
        for (const Vec3& p : f)
            glVertex3f(float(p.x), float(p.y) + 0.005f, float(p.z));
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(float(cb.x), float(cb.y) + 0.005f, float(cb.z));
        for (const Vec3& p : f) {
            Vec3 b = p + offset;
            glVertex3f(float(b.x), float(b.y) + 0.005f, float(b.z));
        }
        glEnd();
    }

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
