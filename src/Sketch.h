// kawr — sketch model: strokes, grid snap, edge weld, closure into shapes.
// Apache-2.0.
#pragma once

#include <vector>

#include "Shape.h"
#include "Vec3.h"

// Holds the sketch state: the in-progress stroke, finished open strokes, and
// closed shapes. Owns the snap/weld behavior and decides when a stroke closes.
class Sketch {
public:
    void startStroke();
    void addSample(const Vec3& groundPt);
    bool endStroke();  // returns true if a new closed Shape was formed
    bool drawing() const { return m_drawing; }

    // Grid snap + weld onto a nearby existing shape edge. Reports weld via
    // `welded`.
    Vec3 snapPoint(const Vec3& groundPt, bool* welded = nullptr) const;

    bool snapEnabled() const { return m_snap; }
    void toggleSnap() { m_snap = !m_snap; }
    double gridStep() const { return m_gridStep; }
    void nudgeGrid(double delta);

    void clear();

    const std::vector<Stroke>& strokes() const { return m_strokes; }
    const std::vector<Shape>& shapes() const { return m_shapes; }
    const Stroke& current() const { return m_current; }

private:
    // A point projected onto an existing shape's boundary.
    struct BoundaryHit {
        int shape = -1;  // index into m_shapes
        int edge = 0;    // edge index within that shape's loop
        double t = 0;    // parameter [0,1] along the edge
        Vec3 point;      // the closest boundary point (y = 0)
        double dist = 0;
    };

    Vec3 snapToGrid(const Vec3& p) const;
    bool nearestShapeBoundary(const Vec3& p, double tol, BoundaryHit& out) const;
    // Append the shorter boundary arc between two hits on the same shape to the
    // current stroke, forming a shared edge.
    void appendSharedArc(const BoundaryHit& from, const BoundaryHit& to);

    std::vector<Stroke> m_strokes;  // open polylines
    std::vector<Shape> m_shapes;    // closed shapes
    Stroke m_current;               // stroke in progress
    bool m_drawing = false;

    bool m_snap = true;
    double m_gridStep = 1.0;

    static constexpr double GRID_MIN = 0.25;
    static constexpr double GRID_MAX = 4.0;
    static constexpr double MIN_SPACING = 0.08;  // freehand point spacing
};
