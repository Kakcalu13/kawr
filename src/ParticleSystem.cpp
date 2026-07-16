// kawr — sparkle particle system implementation. Apache-2.0.
#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "GL.h"

static double randRange(double a, double b) {
    return a + (b - a) * (double(std::rand()) / double(RAND_MAX));
}

// Emit sparkles that fly outward from every edge of the shape's front loop.
void ParticleSystem::spawnFromShape(const Shape& sh) {
    const std::vector<Vec3>& loop = sh.loop;
    if (loop.size() < 2) return;

    Vec3 c = sh.centroid();

    Vec3 lo = loop[0], hi = loop[0];
    for (const Vec3& p : loop) {
        lo.x = std::min(lo.x, p.x); lo.z = std::min(lo.z, p.z);
        hi.x = std::max(hi.x, p.x); hi.z = std::max(hi.z, p.z);
    }
    double diag = (hi - lo).length();
    double step = std::max(0.15, diag * 0.03);  // arc-length between emitters

    const int kMaxParticles = 900;
    double carry = 0;

    for (size_t i = 0; i + 1 < loop.size(); ++i) {
        Vec3 a = loop[i], b = loop[i + 1];
        double segLen = (b - a).length();
        if (segLen < 1e-6) continue;

        for (double d = carry; d < segLen; d += step) {
            if (m_particles.size() >= size_t(kMaxParticles)) return;
            double u = d / segLen;
            Vec3 p = a + (b - a) * u;

            Vec3 outward = Vec3(p.x - c.x, 0, p.z - c.z).normalized();
            for (int k = 0; k < 2; ++k) {
                Particle pt;
                pt.pos = Vec3(p.x, 0.02, p.z);
                pt.vel = outward * randRange(1.0, 2.6) +
                         Vec3(0, randRange(1.0, 2.4), 0) +
                         Vec3(randRange(-0.5, 0.5), 0, randRange(-0.5, 0.5));
                pt.maxLife = randRange(0.5, 1.1);
                pt.life = pt.maxLife;
                pt.size = float(randRange(3.0, 7.0));
                m_particles.push_back(pt);
            }
        }
        carry = std::fmod(carry + segLen, step);
    }
}

void ParticleSystem::update(double dt) {
    const double gravity = 3.2;
    for (Particle& p : m_particles) {
        p.pos = p.pos + p.vel * dt;
        p.vel.y -= gravity * dt;
        p.life -= dt;
    }
    std::vector<Particle> alive;
    alive.reserve(m_particles.size());
    for (const Particle& p : m_particles)
        if (p.life > 0) alive.push_back(p);
    m_particles.swap(alive);
}

void ParticleSystem::draw() const {
    if (m_particles.empty()) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive glow
    glDepthMask(GL_FALSE);
    glEnable(GL_POINT_SMOOTH);
    for (const Particle& p : m_particles) {
        double a = p.life / p.maxLife;
        glPointSize(p.size * float(0.35 + 0.65 * a));
        glColor4f(0.92f, 0.86f, 1.0f, float(a));
        glBegin(GL_POINTS);
        glVertex3f(float(p.pos.x), float(p.pos.y), float(p.pos.z));
        glEnd();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
