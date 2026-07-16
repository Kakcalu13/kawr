// kawr — sketch strokes and closed extruded shapes. Apache-2.0.
#pragma once

#include <vector>

#include "Vec3.h"

// Lavender fill/outline color shared across closed shapes.
inline constexpr float LAVENDER[3] = {0.78f, 0.70f, 0.96f};

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

    // Filled lattice mesh of the front face (y = 0): grid nodes inside the
    // outline, joined into quads. The back face is these vertices + offset.
    // This is the connected, cloth-ready mesh (and it fills concave shapes
    // cleanly, unlike a centroid fan).
    std::vector<Vec3> meshVerts;
    std::vector<int> quadIdx;  // 4 vertex indices per quad

    // (Re)generate meshVerts/quadIdx from loop. `divisions` = grid cells across
    // the shape's largest dimension (higher = finer subdivision).
    void buildMesh(int divisions);
    Vec3 centroid() const;
    void draw() const;
};
