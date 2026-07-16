// kawr — application: owns the scene and drives the GLUT event loop. Apache-2.0.
#pragma once

#include "Camera.h"
#include "ParticleSystem.h"
#include "Sketch.h"
#include "Vec3.h"

// Single application instance. GLUT's C callbacks trampoline into these
// methods (see App.cpp).
class App {
public:
    static App& instance();

    void init(int argc, char** argv);
    void run();

    void display();
    void reshape(int w, int h);
    void mouse(int button, int state, int x, int y);
    void motion(int x, int y);
    void passiveMotion(int x, int y);
    void keyboard(unsigned char key, int x, int y);
    void timer();

private:
    App() {}

    void updateHover(int x, int y);
    void drawGrid() const;
    void drawSnapMarker() const;
    void drawOverlay() const;

    Camera m_camera;
    Sketch m_sketch;
    ParticleSystem m_particles;

    int m_winW = 1024, m_winH = 720;
    int m_mouseButton = -1;
    int m_lastX = 0, m_lastY = 0;
    int m_prevTimeMs = 0;

    Vec3 m_hover;
    bool m_hoverValid = false;
    bool m_hoverWeld = false;

    static constexpr double GRID_EXTENT = 20.0;  // grid half-size in world units
};
