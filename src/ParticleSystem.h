// kawr — sparkle particle system. Apache-2.0.
#pragma once

#include <vector>

#include "Shape.h"
#include "Vec3.h"

// A small additive-glow particle burst emitted from a shape's edges.
class ParticleSystem {
public:
    void spawnFromShape(const Shape& sh);
    void update(double dt);
    void draw() const;

    void clear() { m_particles.clear(); }
    bool empty() const { return m_particles.empty(); }

private:
    struct Particle {
        Vec3 pos;
        Vec3 vel;
        double life = 0;
        double maxLife = 1;
        float size = 5;
    };
    std::vector<Particle> m_particles;
};
