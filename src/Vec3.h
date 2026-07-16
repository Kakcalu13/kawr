// kawr — minimal 3D vector. Apache-2.0.
#pragma once

#include <cmath>

struct Vec3 {
    double x = 0, y = 0, z = 0;

    constexpr Vec3() {}
    constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }

    double length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        double l = length();
        return (l > 1e-9) ? Vec3(x / l, y / l, z / l) : Vec3();
    }
};
