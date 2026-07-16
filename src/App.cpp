// kawr — application implementation. Apache-2.0.
#include "App.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "GL.h"
#include "MeshExport.h"
#include "Shape.h"

// ---------------------------------------------------------------------------
// GLUT trampolines -> singleton methods
// ---------------------------------------------------------------------------
static void cbDisplay() { App::instance().display(); }
static void cbReshape(int w, int h) { App::instance().reshape(w, h); }
static void cbMouse(int b, int s, int x, int y) { App::instance().mouse(b, s, x, y); }
static void cbMotion(int x, int y) { App::instance().motion(x, y); }
static void cbPassive(int x, int y) { App::instance().passiveMotion(x, y); }
static void cbKeyboard(unsigned char k, int x, int y) { App::instance().keyboard(k, x, y); }
static void cbTimer(int) {
    App::instance().timer();
    glutTimerFunc(16, cbTimer, 0);
}

App& App::instance() {
    static App app;
    return app;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void App::init(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(m_winW, m_winH);
    glutCreateWindow("kawr — 3D sketch viewport");

    glEnable(GL_MULTISAMPLE);

    glutDisplayFunc(cbDisplay);
    glutReshapeFunc(cbReshape);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);
    glutPassiveMotionFunc(cbPassive);
    glutKeyboardFunc(cbKeyboard);

    m_prevTimeMs = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, cbTimer, 0);
}

void App::run() { glutMainLoop(); }

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
void App::display() {
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_camera.apply(m_winW, m_winH);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    drawGrid();
    drawSnapMarker();
    for (const Stroke& s : m_sketch.strokes()) s.draw(false);
    if (m_sketch.drawing()) m_sketch.current().draw(true);
    for (const Shape& sh : m_sketch.shapes()) sh.draw();
    m_particles.draw();

    drawOverlay();

    glutSwapBuffers();
}

void App::drawGrid() const {
    const double step = m_sketch.gridStep();
    const float ext = float(GRID_EXTENT);
    int n = int(std::floor(GRID_EXTENT / step));
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = -n; i <= n; ++i) {
        if (i == 0) continue;
        float c = float(i * step);
        glColor3f(0.20f, 0.22f, 0.28f);
        glVertex3f(c, 0, -ext); glVertex3f(c, 0, ext);
        glVertex3f(-ext, 0, c); glVertex3f(ext, 0, c);
    }
    glColor3f(0.55f, 0.25f, 0.30f);
    glVertex3f(-ext, 0, 0); glVertex3f(ext, 0, 0);
    glColor3f(0.25f, 0.35f, 0.55f);
    glVertex3f(0, 0, -ext); glVertex3f(0, 0, ext);
    glEnd();
}

void App::drawSnapMarker() const {
    if (!m_hoverValid) return;
    float r = float(m_sketch.gridStep()) * (m_hoverWeld ? 0.28f : 0.18f);
    float y = float(m_hover.y) + 0.02f;
    float x = float(m_hover.x), z = float(m_hover.z);
    glLineWidth(m_hoverWeld ? 2.5f : 1.5f);
    if (m_hoverWeld)
        glColor3f(1.0f, 0.45f, 0.75f);  // weld target
    else
        glColor3f(0.55f, 0.85f, 0.95f);  // plain grid snap
    glBegin(GL_LINE_LOOP);
    glVertex3f(x - r, y, z);
    glVertex3f(x, y, z - r);
    glVertex3f(x + r, y, z);
    glVertex3f(x, y, z + r);
    glEnd();
}

static void drawText(float x, float y, const std::string& s, void* font) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(font, c);
}

void App::drawOverlay() const {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, m_winW, 0, m_winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    glColor3f(0.75f, 0.78f, 0.85f);
    drawText(14, m_winH - 22,
             "L-drag: draw   R-drag: orbit   Scroll: grid   +/-: zoom",
             GLUT_BITMAP_9_BY_15);
    drawText(14, m_winH - 40,
             "[ ]: subdiv   , .: gap   S: snap   E: export   C: clear   Esc: quit",
             GLUT_BITMAP_9_BY_15);

    char grid[32], gap[32];
    std::snprintf(grid, sizeof(grid), "%.2f", m_sketch.gridStep());
    std::snprintf(gap, sizeof(gap), "%.2f", m_sketch.thickness());
    std::string status = std::string("snap: ") +
                         (m_sketch.snapEnabled() ? "on" : "off") +
                         "   grid: " + grid +
                         "   subdiv: " + std::to_string(m_sketch.subdiv()) +
                         "   gap: " + gap +
                         "   shapes: " +
                         std::to_string(m_sketch.shapes().size());
    glColor3f(LAVENDER[0], LAVENDER[1], LAVENDER[2]);
    drawText(14, 16, status, GLUT_BITMAP_9_BY_15);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void App::reshape(int w, int h) {
    m_winW = w;
    m_winH = (h > 0) ? h : 1;
    glViewport(0, 0, m_winW, m_winH);
}

void App::updateHover(int x, int y) {
    Vec3 p;
    if (!m_camera.screenToGround(x, y, p)) {
        m_hoverValid = false;
        return;
    }
    bool weld = false;
    m_hover = m_sketch.snapPoint(p, &weld);
    m_hoverWeld = weld;
    m_hoverValid = (m_sketch.snapEnabled() || weld);
}

void App::mouse(int button, int state, int x, int y) {
    // Mouse-wheel (buttons 3/4 on many GLUT builds) adjusts the grid size.
    if (button == 3 && state == GLUT_DOWN) {
        m_sketch.nudgeGrid(-0.25);
        updateHover(x, y);
        glutPostRedisplay();
        return;
    }
    if (button == 4 && state == GLUT_DOWN) {
        m_sketch.nudgeGrid(+0.25);
        updateHover(x, y);
        glutPostRedisplay();
        return;
    }

    if (state == GLUT_DOWN) {
        m_mouseButton = button;
        m_lastX = x;
        m_lastY = y;

        if (button == GLUT_LEFT_BUTTON) {
            m_sketch.startStroke();
            Vec3 p;
            if (m_camera.screenToGround(x, y, p)) m_sketch.addSample(p);
            updateHover(x, y);
        }
    } else if (state == GLUT_UP) {
        if (button == GLUT_LEFT_BUTTON && m_sketch.drawing()) {
            if (m_sketch.endStroke())
                m_particles.spawnFromShape(m_sketch.shapes().back());
        }
        m_mouseButton = -1;
    }
    glutPostRedisplay();
}

void App::motion(int x, int y) {
    if (m_mouseButton == GLUT_LEFT_BUTTON && m_sketch.drawing()) {
        Vec3 p;
        if (m_camera.screenToGround(x, y, p)) {
            m_sketch.addSample(p);
            updateHover(x, y);
        }
    } else if (m_mouseButton == GLUT_RIGHT_BUTTON) {
        m_camera.orbit(x - m_lastX, y - m_lastY);
        m_lastX = x;
        m_lastY = y;
    }
    glutPostRedisplay();
}

void App::passiveMotion(int x, int y) {
    updateHover(x, y);
    glutPostRedisplay();
}

void App::keyboard(unsigned char key, int, int) {
    switch (key) {
        case 27:  // Esc
        case 'q':
        case 'Q':
            std::exit(0);
            break;
        case 'c':
        case 'C':
            m_sketch.clear();
            m_particles.clear();
            break;
        case 's':
        case 'S':
            m_sketch.toggleSnap();
            break;
        case ']':
        case '}':
            m_sketch.subdivide(true);  // finer mesh
            break;
        case '[':
        case '{':
            m_sketch.subdivide(false);  // coarser mesh
            break;
        case '.':
        case '>':
            m_sketch.changeThickness(1.25);  // wider front/back gap
            break;
        case ',':
        case '<':
            m_sketch.changeThickness(0.8);  // narrower front/back gap
            break;
        case '+':
        case '=':
            m_camera.zoomBy(0.9);
            break;
        case '-':
        case '_':
            m_camera.zoomBy(1.1);
            break;
        case 'e':
        case 'E': {
            ExportStats st = exportGlb(m_sketch.shapes(), "kawr_export.glb");
            if (st.ok)
                std::printf("[kawr] exported %d panels, %d verts, %d tris "
                            "-> kawr_export.glb\n",
                            st.panels, st.verts, st.quads * 2);
            else
                std::printf("[kawr] export failed (could not open "
                            "kawr_export.glb)\n");
            std::fflush(stdout);
            break;
        }
        default:
            break;
    }
    glutPostRedisplay();
}

void App::timer() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    double dt = (now - m_prevTimeMs) / 1000.0;
    m_prevTimeMs = now;
    if (dt > 0.1) dt = 0.1;  // clamp after stalls

    if (!m_particles.empty()) {
        m_particles.update(dt);
        glutPostRedisplay();
    }
}
