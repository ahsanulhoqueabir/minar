# Documentation Index

Welcome! This folder explains the **Shaheed Minar OpenGL project** from the ground up.
You do not need prior graphics experience — each page starts with the idea, then shows
the code that implements it.

## Suggested reading order

1. **[Getting started](01-getting-started.md)** – build, run, play with the controls.
2. **[Architecture](02-architecture.md)** – the big picture: who calls whom.
3. **[Geometry primitives](03-geometry.md)** – the building blocks (boxes, rods, ellipsoids).
4. **[The monument model](04-monument-model.md)** – how the Minar itself is assembled.
5. **[Pigeons](05-pigeons.md)** – animation and parametric flight.
6. **[App, camera and projections](06-app-camera-projection.md)** – the heart of the course material.
7. **[Front ends](07-frontends.md)** – the window/OS glue (Win32 and GLUT).
8. **[Modification guide](08-modification-guide.md)** – safe experiments and their effects.
9. **[Glossary](09-glossary.md)** – quick definitions.

## One-paragraph summary

`main_*.cpp` creates a window and an OpenGL context, then forwards events (keys, mouse,
resize) and a per-frame timer to **`app.cpp`**. `app.cpp` owns all state (camera, model
transform, toggles, animation targets). Each frame it sets up the projection and view,
applies the model transform, and asks **`minar_model.cpp`** and **`pigeons.cpp`** to draw
the scene. Those two files use the primitives in **`geometry.cpp`** (hexahedra, rods,
ellipsoids). Everything is fixed-function OpenGL 1.1 (immediate mode `glBegin/glEnd`).

## Source file map

| File | Lines | Purpose |
|---|---|---|
| [src/main_win32.cpp](../src/main_win32.cpp) | ~200 | Win32 window, WGL context, anti-aliased pixel format, bitmap font, main loop |
| [src/main_glut.cpp](../src/main_glut.cpp) | ~90 | Same job with GLUT callbacks |
| [src/app.h](../src/app.h) | 27 | The interface between front end and application |
| [src/app.cpp](../src/app.cpp) | ~620 | State, transforms, projections, camera, shadows, HUD, animation, input |
| [src/minar_model.h](../src/minar_model.h) / [.cpp](../src/minar_model.cpp) | ~270 | Platform, central frame, side frames, sun |
| [src/pigeons.h](../src/pigeons.h) / [.cpp](../src/pigeons.cpp) | ~200 | Pigeon model, flight paths, flapping |
| [src/geometry.h](../src/geometry.h) / [.cpp](../src/geometry.cpp) | ~130 | Shared drawing primitives |
| [build.bat](../build.bat) | 11 | Windows build script |
| [ref/](../ref) | – | Two reference images of the real monument |
