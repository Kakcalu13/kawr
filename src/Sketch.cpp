// kawr — sketch model implementation. Apache-2.0.
#include "Sketch.h"

#include <algorithm>
#include <cmath>
#include <utility>

static double strokeDiagonal(const std::vector<Vec3>& pts) {
    if (pts.empty()) return 0;
    Vec3 lo = pts[0], hi = pts[0];
    for (const Vec3& p : pts) {
        lo.x = std::min(lo.x, p.x); lo.z = std::min(lo.z, p.z);
        hi.x = std::max(hi.x, p.x); hi.z = std::max(hi.z, p.z);
    }
    return (hi - lo).length();
}

// Signed-area magnitude of the polygon projected onto the ground (shoelace).
static double polygonAreaXZ(const std::vector<Vec3>& pts) {
    if (pts.size() < 3) return 0;
    double a = 0;
    for (size_t i = 0; i < pts.size(); ++i) {
        const Vec3& c = pts[i];
        const Vec3& n = pts[(i + 1) % pts.size()];
        a += c.x * n.z - n.x * c.z;
    }
    return std::fabs(a) * 0.5;
}

static double fmodPositive(double a, double m) {
    double r = std::fmod(a, m);
    if (r < 0) r += m;
    return r;
}

Vec3 Sketch::snapToGrid(const Vec3& p) const {
    double s = m_gridStep;
    return Vec3(std::round(p.x / s) * s, 0.0, std::round(p.z / s) * s);
}

// Closest point on any existing shape's boundary (its front-loop edges), within
// `tol`. Handles points anywhere along an edge, not just at the corners.
bool Sketch::nearestShapeBoundary(const Vec3& p, double tol, BoundaryHit& out) const {
    double best = tol;
    bool found = false;
    for (size_t si = 0; si < m_shapes.size(); ++si) {
        const std::vector<Vec3>& loop = m_shapes[si].loop;
        for (size_t e = 0; e + 1 < loop.size(); ++e) {
            Vec3 a = loop[e], b = loop[e + 1];
            double abx = b.x - a.x, abz = b.z - a.z;
            double len2 = abx * abx + abz * abz;
            double t = 0.0;
            if (len2 > 1e-12)
                t = ((p.x - a.x) * abx + (p.z - a.z) * abz) / len2;
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            double cx = a.x + abx * t, cz = a.z + abz * t;
            double dx = cx - p.x, dz = cz - p.z;
            double d = std::sqrt(dx * dx + dz * dz);
            if (d <= best) {
                best = d;
                out.shape = int(si);
                out.edge = int(e);
                out.t = t;
                out.point = Vec3(cx, 0.0, cz);
                out.dist = d;
                found = true;
            }
        }
    }
    return found;
}

void Sketch::appendSharedArc(const BoundaryHit& from, const BoundaryHit& to) {
    const std::vector<Vec3>& loop = m_shapes[from.shape].loop;
    int m = int(loop.size());
    if (m < 3) return;
    int k = m - 1;  // unique corners (loop[m-1] == loop[0])

    double posFrom = from.edge + from.t;  // boundary parameter in [0, k)
    double posTo = to.edge + to.t;
    double span = fmodPositive(posTo - posFrom, double(k));   // forward length
    double spanBack = double(k) - span;

    // Corners strictly inside each arc, ordered from `from` toward `to`.
    std::vector<std::pair<double, int>> fwd, bwd;
    for (int j = 0; j < k; ++j) {
        double dF = fmodPositive(double(j) - posFrom, double(k));
        if (dF > 1e-9 && dF < span - 1e-9) fwd.push_back({dF, j});
        double dB = fmodPositive(posFrom - double(j), double(k));
        if (dB > 1e-9 && dB < spanBack - 1e-9) bwd.push_back({dB, j});
    }
    std::sort(fwd.begin(), fwd.end());
    std::sort(bwd.begin(), bwd.end());

    // Pick the physically shorter arc.
    auto arcLen = [&](const std::vector<std::pair<double, int>>& corners) {
        double len = 0;
        Vec3 prev = from.point;
        for (const auto& c : corners) {
            Vec3 v = loop[c.second];
            len += (v - prev).length();
            prev = v;
        }
        return len + (to.point - prev).length();
    };
    const std::vector<std::pair<double, int>>& chosen =
        (arcLen(fwd) <= arcLen(bwd)) ? fwd : bwd;

    for (const auto& c : chosen) {
        const Vec3& v = loop[c.second];
        m_current.pts.push_back(Vec3(v.x, 0.0, v.z));
    }
}

Vec3 Sketch::snapPoint(const Vec3& groundP, bool* welded) const {
    Vec3 p = m_snap ? snapToGrid(groundP) : groundP;
    BoundaryHit h;
    bool w = nearestShapeBoundary(p, m_gridStep, h);  // live weld radius
    if (w) p = h.point;
    if (welded) *welded = w;
    return p;
}

void Sketch::nudgeGrid(double delta) {
    double g = std::round((m_gridStep + delta) / 0.25) * 0.25;
    if (g < GRID_MIN) g = GRID_MIN;
    if (g > GRID_MAX) g = GRID_MAX;
    m_gridStep = g;
}

void Sketch::startStroke() {
    m_drawing = true;
    m_current.pts.clear();
    m_current.closed = false;
}

void Sketch::addSample(const Vec3& groundPt) {
    if (!m_drawing) return;
    Vec3 sp = snapPoint(groundPt);
    // Snap mode: add a point on entering a new cell/vertex. Freehand: add once
    // we've moved far enough.
    double minStep = m_snap ? 1e-6 : MIN_SPACING;
    if (m_current.pts.empty() ||
        (sp - m_current.pts.back()).length() > minStep) {
        m_current.pts.push_back(sp);
    }
}

bool Sketch::endStroke() {
    m_drawing = false;
    if (m_current.pts.size() < 3) {
        m_current.pts.clear();
        return false;
    }

    // Weld the endpoints onto a nearby existing shape's edge (generous radius on
    // release) so a new loop can attach to shared geometry.
    double weldTol = m_gridStep * 2.5;
    BoundaryHit hStart, hEnd;
    bool startWeld = nearestShapeBoundary(m_current.pts.front(), weldTol, hStart);
    bool endWeld = nearestShapeBoundary(m_current.pts.back(), weldTol, hEnd);
    if (startWeld) m_current.pts.front() = hStart.point;
    if (endWeld) m_current.pts.back() = hEnd.point;

    // Both ends welded onto existing geometry closes the loop:
    //  - same shape  -> trace that shape's boundary between them (shared edge)
    //  - two shapes  -> bridge them with the straight closing edge below
    bool weldClosed = startWeld && endWeld;
    if (weldClosed && hStart.shape == hEnd.shape)
        appendSharedArc(hEnd, hStart);

    double diag = strokeDiagonal(m_current.pts);
    double gap = (m_current.pts.back() - m_current.pts.front()).length();
    double area = polygonAreaXZ(m_current.pts);

    // A loop closes if it returns to its own start or welded onto existing
    // geometry at both ends; either way it must enclose real area.
    double closeThreshold =
        m_snap ? m_gridStep * 0.75 : std::max(0.4, diag * 0.16);
    double minArea =
        m_snap ? std::max(0.02, m_gridStep * m_gridStep * 0.4) : 0.15;

    bool selfClosed = gap < closeThreshold;
    bool closed = (selfClosed || weldClosed) && area > minArea;

    bool formed = false;
    if (closed) {
        Stroke s = m_current;
        if ((s.pts.back() - s.pts.front()).length() > 1e-6)
            s.pts.push_back(s.pts.front());

        Shape sh;
        sh.loop = s.pts;
        sh.offset = Vec3(0.0, -m_thickness, 0.0);
        sh.buildMesh(m_meshDiv);  // fill the interior with a connected lattice
        m_shapes.push_back(sh);
        formed = true;
    } else {
        Stroke s = m_current;
        s.closed = false;
        m_strokes.push_back(s);
    }
    m_current.pts.clear();
    return formed;
}

void Sketch::subdivide(bool finer) {
    int v = finer ? m_meshDiv * 2 : m_meshDiv / 2;
    if (v < SUBDIV_MIN) v = SUBDIV_MIN;
    if (v > SUBDIV_MAX) v = SUBDIV_MAX;
    if (v != m_meshDiv) {
        m_meshDiv = v;
        rebuildAllMeshes();
    }
}

void Sketch::rebuildAllMeshes() {
    for (Shape& sh : m_shapes) sh.buildMesh(m_meshDiv);
}

void Sketch::changeThickness(double factor) {
    double t = m_thickness * factor;
    if (t < THICK_MIN) t = THICK_MIN;
    if (t > THICK_MAX) t = THICK_MAX;
    m_thickness = t;
    // The back panel is applied live at draw time (verts + offset), so just
    // updating each shape's offset is enough — no mesh rebuild.
    Vec3 off(0.0, -m_thickness, 0.0);
    for (Shape& sh : m_shapes) sh.offset = off;
}

void Sketch::clear() {
    m_strokes.clear();
    m_shapes.clear();
    m_current.pts.clear();
}
