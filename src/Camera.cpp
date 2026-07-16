// kawr — orbit camera implementation. Apache-2.0.
#include "Camera.h"

#include <cmath>

#include "GL.h"

Vec3 Camera::eye() const {
    return Vec3(m_dist * std::cos(m_pitch) * std::sin(m_yaw),
                m_dist * std::sin(m_pitch),
                m_dist * std::cos(m_pitch) * std::cos(m_yaw));
}

void Camera::apply(int winW, int winH) {
    double aspect = (winH > 0) ? double(winW) / double(winH) : 1.0;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, aspect, 0.1, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec3 e = eye();
    gluLookAt(e.x, e.y, e.z, 0, 0, 0, 0, 1, 0);

    glGetDoublev(GL_MODELVIEW_MATRIX, m_model);
    glGetDoublev(GL_PROJECTION_MATRIX, m_proj);
    glGetIntegerv(GL_VIEWPORT, m_view);
}

bool Camera::screenToGround(int sx, int sy, Vec3& out) const {
    GLdouble nx, ny, nz, fx, fy, fz;
    double winX = double(sx);
    double winY = double(m_view[3]) - double(sy);  // GL origin is bottom-left

    if (gluUnProject(winX, winY, 0.0, m_model, m_proj, m_view, &nx, &ny, &nz) == GL_FALSE)
        return false;
    if (gluUnProject(winX, winY, 1.0, m_model, m_proj, m_view, &fx, &fy, &fz) == GL_FALSE)
        return false;

    Vec3 p0(nx, ny, nz), p1(fx, fy, fz);
    Vec3 d = p1 - p0;
    if (std::fabs(d.y) < 1e-9) return false;

    double t = -p0.y / d.y;
    if (t < 0) return false;
    out = p0 + d * t;
    return true;
}

void Camera::orbit(double dxPixels, double dyPixels) {
    m_yaw -= dxPixels * 0.01;
    m_pitch += dyPixels * 0.01;
    const double lim = 1.45;
    if (m_pitch > lim) m_pitch = lim;
    if (m_pitch < -lim) m_pitch = -lim;
}

void Camera::zoomBy(double factor) {
    m_dist *= factor;
    if (m_dist < 2.0) m_dist = 2.0;
    if (m_dist > 120.0) m_dist = 120.0;
}
