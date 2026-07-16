// kawr — a simple 3D viewport you can draw on with the mouse.
//
// Draw a stroke on the ground plane with the left mouse button. When the
// stroke closes on itself (or welds onto an existing shape's edge), it becomes
// a lavender extruded band and emits a sparkle burst from every edge.
//
// Controls:
//   Left-drag ........ draw a stroke (snaps to the grid; welds to vertices)
//   Right-drag ....... orbit the camera
//   Scroll ........... adjust the grid ("#") size
//   + / - ............ zoom
//   S ................ toggle snap-to-grid
//   C ................ clear the sketch
//   Esc / Q .......... quit
//
// Copyright 2026 kawr contributors
// Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <cstdlib>
#include <ctime>

#include "App.h"

int main(int argc, char** argv) {
    std::srand(unsigned(std::time(nullptr)));

    App& app = App::instance();
    app.init(argc, argv);
    app.run();
    return 0;
}
