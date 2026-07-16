// kawr — mesh export to binary glTF (.glb). Apache-2.0.
#pragma once

#include <string>
#include <vector>

#include "Shape.h"

struct ExportStats {
    bool ok = false;
    int panels = 0;
    int verts = 0;
    int quads = 0;
};

// Write every closed shape to `path` as a binary glTF (.glb). Each shape becomes
// two cloth panels — front (y = 0) and back (front + gap offset) — with matching
// topology, triangulated (glTF has no quads). Positions + indices go in the GLB
// BIN chunk; the JSON chunk describes the meshes.
ExportStats exportGlb(const std::vector<Shape>& shapes, const std::string& path);
