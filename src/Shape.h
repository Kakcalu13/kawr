// kawr — sketch strokes and closed extruded shapes. Apache-2.0.
#pragma once

#include <vector>

#include "Vec3.h"

// Lavender fill/outline color shared across closed shapes.
inline constexpr float LAVENDER[3] = {0.78f, 0.70f, 0.96f};

// A closed shape spawns an identical copy offset along the drawing plane's
// normal. We draw on the XZ ground plane (a "y-based" plane), so the copy keeps
// the same X and Z and is shifted in Y.
inline constexpr Vec3 DUPLICATE_OFFSET(0.0, -0.8, 0.0);

// An open polyline (drawn in progress or a stroke that never closed).
struct Stroke {
    std::vector<Vec3> pts;
    bool closed = false;
    void draw(bool inProgress) const;
};

// A closed shape extruded into a band: `loop` is the closed front outline
// (last point == first), the back outline is `loop + offset`, and side faces
// span the two — the connected surface a cloth would use.
struct Shape {
    std::vector<Vec3> loop;
    Vec3 offset;

    Vec3 centroid() const;
    void draw() const;
};
