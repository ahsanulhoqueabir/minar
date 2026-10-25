# 6. App, Camera and Projections — `app.cpp`

This is the largest file and contains most of the Computer Graphics course content. Sections
below follow the file top to bottom.

## Matrix primer (for beginners)

OpenGL keeps two main matrices:

* **Projection** – turns 3D camera-space into the 2D screen (perspective, ortho, ...).
* **Modelview** – places objects (model transform) *and* the camera (view transform).

`glTranslate/Rotate/Scale` **multiply** the current matrix on the right, so the last call
acts on the vertices first. `glPushMatrix/glPopMatrix` save/restore the current matrix —
that is how a child part (head, wing) inherits its parent's transform.
Matrices passed to `glMultMatrixf` are **column-major**: `m[col*4 + row]`.

## State and presets

See the table in [02-architecture](02-architecture.md#state-ownership).
`PRESETS[6]` holds camera yaw/pitch/distance/target-height for keys `1`–`6`;
`applyPreset(i)` copies one in and stores its name for the HUD. `appInit` uses preset 5 (index
`5` = "Front, raised (ref. 2)") which is why the program opens on that view.

`LIGHT_DIR = (-0.35, 1.0, 0.55, 0)` is a **directional light** (w = 0): it comes from above,
slightly left and front. It is set with `glLightfv(GL_POSITION)` *every frame after the view is
set*, so it stays fixed in the world even as the camera orbits.

## `appInit()`

* `resetModel()` – zero translation/rotation, scale 1, no shear/mirror, no auto-rotate.
* `applyPreset(5)`.
* Animation start values: `bendAngle = bendTarget = 28`, `sunRise = sunTarget = 1`, `explode = 0`.
* GL state: `GL_DEPTH_TEST`; `GL_NORMALIZE` (re-normalises normals after `glScale`, otherwise lighting breaks when scaling); `GL_MULTISAMPLE` (anti-alias, harmless if absent); smooth shading; light 0 with ambient (0.38,0.38,0.40) and diffuse (0.72,0.72,0.70); `glColorMaterial` so `glColor` sets the material; one-sided lighting.

Raising the ambient makes shadowed sides brighter (flatter look); raising the diffuse
increases contrast with the light direction.

## Model transformation <a name="model-transform"></a>

```cpp
static void applyModelTransform() {
    glTranslatef(tx, ty, tz);
    glRotatef(rx, 1,0,0);  glRotatef(ry, 0,1,0);  glRotatef(rz, 0,0,1);
    glScalef(scaleF, scaleF, scaleF);
    if (shearXY != 0) { /* matrix with m[4] = k */ glMultMatrixf(sh); }
    if (mirrorZ) glScalef(1,1,-1);
}
```

The composite is **M = T · Rx · Ry · Rz · S · Shear · Mirror**. A vertex is first mirrored,
then sheared, scaled, rotated about Z, then Y, then X, and finally translated. (Remember: the
call nearest the vertex acts first.)

* **Shear:** the identity with element `[row 0, col 1] = k`, i.e. `m[4] = k`, giving
  `x' = x + k·y` — leaning the monument sideways like a deck of cards.
* **Mirror:** `glScalef(1,1,-1)` is a **reflection** through the XY plane (a negative scale).

### Mirror and winding <a name="mirror-and-winding"></a>

A reflection reverses the orientation of polygons: counter-clockwise faces become clockwise.
OpenGL decides which side is "front" (and so which side is lit) from the winding, so
`renderScene` calls `glFrontFace(mirrorZ ? GL_CW : GL_CCW)` while drawing the model, and
restores `GL_CCW` afterward. Without this, a mirrored model would be lit from inside.

The whole scene lives inside this transform: platform, monument, **shadows and pigeons**.
The ground, grid and axes are outside it (they do not move with the model).

## Projections <a name="projections"></a>

### `setProjection(aspect)`

| Mode | How |
|---|---|
| Perspective | `gluPerspective(fovY, aspect, 0.5, 600)` — distant things shrink. |
| Orthographic | `glOrtho(-h·aspect, h·aspect, -h, h, -600, 600)` with `h = camDist·tan(fovY/2)` — sized to match what the perspective camera sees at the target, so switching modes does not jump. No foreshortening. |
| Oblique (cabinet / cavalier) | Orthographic **times a shear** (`glMultMatrixf`) — see below. |

### Oblique projection

Parallel projection where the depth axis is drawn slanting at an angle:

```
x' = x + L·cos(α)·(z + d)
y' = y + L·sin(α)·(z + d)
```

With `α = 45°`; `L = 0.5` for **cabinet** (depth halved, looks natural) and `L = 1`
for **cavalier** (depth at full length, looks stretched). The code builds the shear matrix
`H` with `a = −L cos45`, `b = −L sin45` (negative because eye-space looks down −z) and
translation `(a·d, b·d)` where `d = camDist`, so the target plane (`z = −d` in eye space)
stays where it is rather than sliding off-screen.
`PROJ_NAMES[]` provides the text shown in the HUD.

Switch with `P`. Compare in the screenshot below (cabinet, with axes):

![Cabinet](images/07-oblique-cabinet.png)

### Camera: `setOrbitView()`

The camera orbits a target point `(0, camTargetY, 0)`:

```
eye.x = d·cos(pitch)·sin(yaw)
eye.y = targetY + d·sin(pitch)
eye.z = d·cos(pitch)·cos(yaw)
```
then `gluLookAt(eye, target, up=(0,1,0))`. Mouse drag changes yaw (−0.4°/px) and pitch
(clamped −30…89 so the camera never flips over the pole). The ceiling of 89° avoids the
degenerate "looking straight down" case where `up` is parallel to the view direction.

### Fixed views: `setFixedView(view, aspect)`

For the four-viewport layout, three fixed orthographic cameras:

| View | Half-height | Camera |
|---|---|---|
| Front | 11 | at (0, 8, 150) looking at (0, 8, 0) |
| Side | 11 | at (150, 8, 0) |
| Top (plan) | 19 | at (0, 150, −3) looking down, up = −Z (so +Z — the front — points toward the bottom of the pane) |

The far distance (150) with a near/far of 1/400 means perspective is not needed — orthographic
has no distance foreshortening, so the camera could be anywhere along the axis.

### Viewports <a name="viewports"></a>

`renderViewport(x, y, w, h, view, label)`:

1. `glViewport` — where on the window the picture goes. `glScissor` + `GL_SCISSOR_TEST` — restricts `glClear` so one pane does not wipe the others.
2. Clear depth+stencil only (colour is overwritten by the background).
3. Draw background, set projection and view, `renderScene()`, draw a label.

`appDisplay` in quad mode places Front (top-left), Right side (top-right), Top/plan
(bottom-left) and the perspective/orthographic camera (bottom-right); the `-1` sizes leave a 1 px divider line showing the dark clear colour.

![Four viewports](images/06-four-viewports.png)

## Scene drawing

| Function | Role |
|---|---|
| `drawBackground()` | Draws a screen-aligned gradient quad: pushes/loads identity on both matrices, depth test and lighting off, bottom colour (0.86,0.91,0.97) to top (0.52,0.69,0.90). |
| `drawGround()` | 400 × 400 green quad at y = 0, normal up. |
| `drawGrid()` | Lines every 2 m from −40 to 40, at y = 0.02 (slightly above ground to avoid z-fighting). |
| `drawAxes()` | Red X, green Y, blue Z lines of length 12, 3 px wide, depth test off so always visible, with letters via `glRasterPos3f` + `platformDrawString`. |
| `renderScene()` | Orchestrates the above; see order below. |

`renderScene` order: set light position → ground → grid → polygon mode (line if wireframe) →
push → `applyModelTransform` → `glFrontFace` → platform → monument → **shadows** (not in wireframe) →
pigeons → restore front face → pop → restore fill mode → axes.

### Wireframe

`glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` draws only edges; a one-line change gives a
completely different look:

![Wireframe](images/03-wireframe.png)

## Shadows <a name="shadows"></a>

A realistic shadow would need shadow maps; this project uses the classic **planar projected
shadow**: flatten the geometry onto the dais plane along the light direction, draw it
dark and translucent.

### `shadowMatrix(m, plane, L)`

For plane `P = (a,b,c,d)` (ax+by+cz+d = 0) and light direction `L`:

```
M = (P·L)·I − L·Pᵀ         (4×4)
```

Multiplying a vertex by `M` slides it along `L` until it lies on the plane. For the dais
`plane = (0, 1, 0, −BASE_Y)`.

### `solve3(M, b, x)`

Solves `A·x = b` (3×3 block of a column-major matrix) with Cramer's rule / adjugate;
returns false for a singular matrix. Used to compute `L_model = M⁻¹ · L_world`, because the
shadow is drawn **inside** the model transform and so the light direction must be expressed
in *model space* — otherwise rotating or scaling the monument would make the shadow wrong.
Only the 3×3 part matters since the light is a direction (`w = 0`).

### `drawShadows()` step by step

1. Get the current model matrix `M` (by loading identity, calling `applyModelTransform`, reading `GL_MODELVIEW_MATRIX`), solve for `L`. If that fails or `L.y < 0.05` (light too low/below the dais) → **no shadow**.
2. **Stencil pass 1** — mark where the dais top is *visible*: colour and depth writes off (`glColorMask`, `glDepthMask`), stencil `ALWAYS, ref 1, REPLACE`, depth func `LEQUAL`, polygon offset (−1,−1) to beat z-fighting, then `drawDaisTop()`. Pixels covered by the dais now have stencil = 1; shadows will appear **only on the dais**, never floating in the air or on the grass.
3. **Stencil pass 2** — enable blending (`SRC_ALPHA, ONE_MINUS_SRC_ALPHA`), colour `(0.05,0.05,0.1, 0.35)`, stencil test `EQUAL 1` with op **`ZERO`** on pass. The first fragment that hits a pixel clears its stencil to 0, so *overlapping* shadow polygons (rods in front of posts…) blend exactly once instead of darkening repeatedly. Multiply by `S`, `setShadowPass(true)`, `drawMonument(minar)`, reset.
4. Restore all state (blend, offset, depth func/mask, stencil, lighting).

Edit effects: alpha 0.35 → darker/lighter shadow; change `LIGHT_DIR` → shadow direction; remove
the stencil → double-dark overlaps and shadows spilling off the platform.

## HUD (heads-up display)

`begin2D(w,h)` sets an orthographic projection in **pixels** with origin bottom-left, disables depth and lighting, enables blending; `end2D()` restores depth. `drawPanel(x, y, lines, n)` draws a translucent black box sized to the longest line (8 px/char, 15 px/line) and the text on it (first line gold). A negative `x` right-aligns the panel to the window.

`drawHUD()` builds the status text with `sprintf` each frame (left panel) and the key help (right panel, toggled with `I`). Add a new status line by `sprintf(s[n++], ...)` — the buffer holds 12 lines, so raise `s[12]` if you exceed it. `drawViewLabel` is the small label for each quad-view pane.

## Animation <a name="animation"></a>

```cpp
static bool approach(float& v, float target, float step) // move v toward target by ≤ step
bool appUpdate(float dt)
```

* `dt` is clamped to 0.05 s so a stall (window drag, breakpoint) does not make a huge jump.
* Rates: bend **18°/s**, sun **0.25/s** (4 s sunrise), explode **0.7/s** (~1.4 s).
* Auto-rotate adds 25°/s to `ry`, wrapped with `fmodf(…,360)`.
* Pigeons add `dt` to `pigeonTime`.
* Returns `true` if anything moved — the front end uses that to decide whether to redraw.

Because motion is `dt`-based, speed is the same on a 30 Hz or 144 Hz screen.

## Input handling

### `appKey(key)` — one big `switch`
Lower/upper-case pairs implement ± (`x`/`X`, `s`/`S`, `h`/`H`, `y`/`Y`, `z`/`Z`). Notable:

| Key | Code effect |
|---|---|
| `s`/`S` | `scaleF` ÷/× 1.05, clamped 0.2–4 |
| `h`/`H` | `shearXY` ∓0.05, clamped ±1 |
| `B` | `bendTarget = bendTarget > 1 ? 0 : DEFAULT_BEND (28)` |
| `,` `.` | `bendTarget ∓2`, clamped −30…45 |
| `U` | `sunRise = 0; sunTarget = 1; showSun = true` — restarts the rise |
| `E` | `explodeTarget` flips between 0 and 1 |
| `0` | `applyPreset(5); fovY = 45` |
| `Q`, Esc (27) | `quitRequested = true` |

### `appSpecialKey` — arrows ±0.5 on tx/tz, PgUp/PgDn ±0.5 on ty.

### Mouse
`appMouseButton` remembers which button is down and the position. `appMouseMotion`:
left drag → yaw/pitch, right drag → `camDist *= (1 + dy·0.01)`; `presetName` becomes `"custom"`.
`appMouseWheel` zooms ×0.9 / ÷0.9 within 5–200.

## Utility functions

| Function | Purpose |
|---|---|
| `rad(deg)` | degrees → radians |
| `clampf(v, lo, hi)` | clamp |
| `resetModel()` | restore the model transform defaults |
| `appResize(w,h)` | store size (min 1 to avoid division by zero in aspect) |
| `appWantsQuit()` | returns `quitRequested` |
