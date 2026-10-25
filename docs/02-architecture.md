# 2. Architecture

## Layers

```
┌───────────────────────────────────────────────────────────┐
│  Front end   main_win32.cpp   OR   main_glut.cpp          │  OS-specific: window, GL context,
│  (one of the two is compiled)                             │  events, timer, font, SwapBuffers
└──────────────▲────────────────────────────┬───────────────┘
               │ platformDrawString()       │ appInit / appResize / appKey / appSpecialKey
               │                            │ appMouse* / appUpdate(dt) / appDisplay()
┌──────────────┴────────────────────────────▼───────────────┐
│  app.cpp   (interface: app.h)                             │  Window-system independent:
│  state, camera, projections, transforms, shadows, HUD,    │  everything you can see or press
│  animation, input handling                                │
└───────┬───────────────────────────────┬───────────────────┘
        │ drawPlatform / drawMonument   │ drawPigeons(t)
┌───────▼──────────────┐       ┌────────▼──────────┐
│  minar_model.cpp     │       │  pigeons.cpp      │
└───────┬──────────────┘       └────────┬──────────┘
        └──────────────┬────────────────┘
               ┌───────▼────────┐
               │ geometry.cpp   │  drawHex, drawBox, drawRod, drawEllipsoid
               └────────────────┘
```

**Why split like this?** The graphics logic never touches Windows or GLUT directly. The
only thing `app.cpp` needs from the platform is one function, `platformDrawString`, so the
same code runs on both front ends (and could run on Linux).

## The interface: [app.h](../src/app.h)

| Function | Direction | Meaning |
|---|---|---|
| `appInit()` | front end → app | Call once after the GL context exists. Sets defaults, GL state, light. |
| `appResize(w, h)` | → app | Window size changed (clamped to ≥ 1). |
| `appDisplay()` | → app | Draw one frame. The front end swaps buffers afterward. |
| `appUpdate(dt)` | → app | Advance animations by `dt` seconds; returns `true` while anything moves. |
| `appKey(c)` | → app | An ASCII key was typed. |
| `appSpecialKey(k)` | → app | Arrow / PgUp / PgDn, mapped to the `SpecialKey` enum. |
| `appMouseButton(b, down, x, y)` | → app | 0 left, 1 middle, 2 right. |
| `appMouseMotion(x, y)` | → app | Mouse moved (only used while dragging). |
| `appMouseWheel(dir)` | → app | +1 wheel away from user (zoom in), −1 toward. |
| `appWantsQuit()` | → app | Polled by the front end after input. |
| `platformDrawString(s)` | app → front end | Draw ASCII text at the current raster position (HUD). |

The front end must create a **depth buffer and an 8-bit stencil buffer** — the shadows need the stencil.

## Life of one frame

1. The front end measures `dt` (time since last frame).
2. `appUpdate(dt)` moves bend angle, sun, explode amount, auto-rotation and pigeon time **toward their targets**; returns whether anything changed.
3. If something changed (or an input event arrived), `appDisplay()` runs:
   1. Clear colour, depth and stencil.
   2. For each viewport (1, or 4 in quad mode) — `renderViewport()`:
      * set viewport + scissor, clear depth/stencil;
      * `drawBackground()` (sky gradient);
      * set projection & camera (`setProjection` + `setOrbitView`, or `setFixedView`);
      * `renderScene()`: ground → grid → [model transform → platform → monument → shadows → pigeons] → axes.
   3. Draw the HUD (single-view mode only).
4. The front end calls `SwapBuffers` / `glutSwapBuffers`.

## State ownership

All mutable state is `static` in [app.cpp](../src/app.cpp) (no classes, easy to read in class):

| Group | Variables |
|---|---|
| Window | `winW`, `winH`, `quitRequested` |
| Camera | `camYaw`, `camPitch`, `camDist`, `camTargetY`, `fovY`, `projMode`, `quadView` |
| Model transform | `tx ty tz`, `rx ry rz`, `scaleF`, `shearXY`, `mirrorZ` |
| Animation | `minar` (a `MinarState`), `bendTarget`, `sunTarget`, `explodeTarget`, `autoRotate`, `pigeonTime` |
| Toggles | `lighting`, `wireframe`, `shadows`, `showGrid`, `showAxes`, `showHelp`, `showPigeons` |
| Mouse | `dragButton`, `lastX`, `lastY` |

Only `MinarState` crosses into another file: it is passed (read-only) to `drawMonument`.

## Animation model: current value → target value

Most animations are *eased toward a target* with `approach(value, target, step)`:

* pressing `B` only changes `bendTarget`;
* each frame `appUpdate` moves `minar.bendAngle` toward it at 18°/s.

That is why changes are smooth and why a key press is cheap. See [06](06-app-camera-projection.md#animation).

## Idle behaviour (Win32)

`appUpdate` returns `false` when nothing animates. The Win32 loop then calls `WaitMessage()`
and consumes no CPU until input arrives. The `dirty` flag forces a redraw after any event.
GLUT instead uses a 16 ms timer and `glutPostRedisplay()`.

## Coordinate system

* Units are metres. **Y up**, monument faces **+Z**, ground at y = 0.
* The marble dais top is at `BASE_Y = 1.2`. All frames are drawn after `glTranslatef(0, BASE_Y, 0)`.
* The monument sits centred on x = 0, with its back (the sun) toward −Z.

## Build graph

```
main_win32.cpp ─┐
(or main_glut)  ├──► shaheed_minar.exe
app.cpp         │
minar_model.cpp │
geometry.cpp    │
pigeons.cpp   ──┘
```

Each `.cpp` includes `<windows.h>` first on Windows (needed before `<GL/gl.h>` for the `WINGDIAPI`/`APIENTRY` macros).
