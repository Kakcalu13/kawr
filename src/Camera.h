// kawr — orbit camera with screen-to-ground picking. Apache-2.0.
#pragma once

#include "Vec3.h"

// Spherical orbit camera around the origin. apply() sets the GL projection and
// modelview each frame and caches the transforms so screenToGround() can
// un-project mouse pixels onto the y = 0 ground plane.
class Camera {
public:
    void apply(int winW, int winH);
    bool screenToGround(int sx, int sy, Vec3& out) const;

    void orbit(double dxPixels, double dyPixels);
    void zoomBy(double factor);

    Vec3 eye() const;

private:
    double m_yaw = 0.7;
    double m_pitch = 0.7;
    double m_dist = 17.0;

    // Cached during apply() for picking.
    double m_model[16] = {0};
    double m_proj[16] = {0};
    int m_view[4] = {0};
};
