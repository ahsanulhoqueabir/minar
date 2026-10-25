# 4. The Monument Model — `minar_model.h` / `minar_model.cpp`

This file knows what the Shaheed Minar looks like. It exposes four drawing functions and one
shared state struct ([minar_model.h](../src/minar_model.h)).

```cpp
extern const float BASE_Y;          // 1.2 – top of the marble dais
struct MinarState { float bendAngle, sunRise, explode; bool showSun; };

void drawPlatform();                // plinth, steps, dais
void drawDaisTop();                 // only the top face of the dais (shadow receiver)
void drawMonument(const MinarState&); // central frame + 4 side frames + sun
void setShadowPass(bool on);        // suppress colour changes while drawing shadows
```

## The real structure in one picture

```
 side    side     CENTRAL      side    side
 frame   frame    "mother"     frame   frame
 (outer) (inner)  frame        (inner) (outer)
 7.6 m   9.6 m    8.6 m lower  9.6 m   7.6 m
 turned  turned   + 5.4 m      turned  turned
 28°     14°      bowed head   14°     28°
```

Top view (plan) — the five frames stand on a shallow arc:

```
          z = -2.8   outer ·                       · outer
          z = -0.9      inner ·                 · inner
          z =  0              [ central frame ]
                                  ▲ faces +Z (visitor)
```

## Colours and the shadow-pass switch

```cpp
static bool g_shadowPass;
static void color(r,g,b) { if (!g_shadowPass) glColor3f(r,g,b); }
```

Every colour call goes through `color()`. When `app.cpp` draws the **shadow**, it calls
`setShadowPass(true)`, so the model keeps its geometry but ignores colour changes — the whole
monument is then drawn in the one flat translucent shadow colour set by the caller.
`concrete()` = (0.93, 0.93, 0.94); `iron()` = (0.17, 0.17, 0.19).

`lerp(a,b,t)` is the small linear-interpolation helper used all over.

## `drawPlatform()` — what is stacked where

| Part | Box (x, y, z) | Colour |
|---|---|---|
| Plinth | x −19…19, y 0…0.9, z −12…4 | warm grey |
| Lower step | x −9…9, y 0…0.6, z 4…5.2 | light |
| Upper step | x −9…9, y 0…0.3, z 5.2…6.4 | light |
| Dais | x −16…16, y 0.9…1.2, z −10…1.8 | white marble |
| Dais step | x −6…6, y 0.9…1.05, z 1.8…2.6 | white marble |

(The two "steps" are separate boxes of different heights, in front of the plinth, which is how the stairs in the screenshots appear.)

`drawDaisTop()` is a single quad at `y = BASE_Y` with the same footprint as the dais top.
It is **only** used by the shadow code, as the surface that receives shadows (see
[06](06-app-camera-projection.md#shadows)). Change the dais size in `drawPlatform` and you must change it here too.

## The central ("mother") frame — `drawCentralFrame(bendDeg, explode)`

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `C_POST_X[3]` | −3.6, 0, 3.6 | x-centres of the 3 posts → 2 openings |
| `C_FLARE[3]` | −0.45, 0, 0.45 | how far each post top moves sideways in the head |
| `C_POST_W` | 0.9 | post width |
| `C_DEPTH` | 0.75 | front-to-back thickness |
| `C_LOWER_H` | 8.6 | height of the bend line above the dais |
| `C_UPPER_L` | 5.4 | length of the bowed head |
| `C_SILL_H` | 0.6 | height of the bottom sill |
| `C_TOPBAR_H` | 0.9 | height of the top beam |
| `C_TOPBAR_LIP` | 0.2 | beam overhang each side |
| `C_RODS` | 5 | rods per opening |
| `ROD_R` | 0.05 | rod radius |

### Drawing sequence

1. **Lower part (frame coordinates, y from the dais):**
   * three vertical posts (`drawBox`);
   * a sill joining them at the bottom;
   * `drawBendJoint` for each post (below);
   * for each of the 2 openings: 5 vertical iron rods evenly spaced (`lerp` with `k/(C_RODS+1)`), from the sill up to the bend line, plus one horizontal **tie rod** at the bend line.
2. **Upper part (the head) — hierarchical modelling:**
   ```cpp
   glPushMatrix();
   glTranslatef(0, C_LOWER_H, 0);   // move origin to the bend line
   glRotatef(bendDeg, 1, 0, 0);     // rotate about X: positive = bow toward +Z
   ... draw head in its own local coordinates (y = 0 at the bend) ...
   glPopMatrix();
   ```
   The head is a **child** of the lower frame: it is drawn in local space with y from 0 to 5.4,
   and the matrix stack moves/rotates it. Changing `bendDeg` rotates the whole head with
   one number. This is the textbook example of hierarchical transformation in the project.
   * three `drawSlantedPost`s whose tops shift by `C_FLARE` (outer posts splay outward);
   * rods that **fan out** between the splayed posts and stop under the top beam (`rodTop = C_UPPER_L − C_TOPBAR_H`; the rod's top x uses `lerp(at, bt, t)` where `at`/`bt` are the opening edges at that height);
   * the **top beam** — a box slightly deeper (`±0.08`) and wider (lip + flare) than the posts.

### `drawBendJoint(x0, x1, bendDeg)` — the mitred corner

When the head bows forward by angle *a*, a V-shaped wedge opens on the *back* (outer side) of
the bend between the end of the lower post and the start of the rotated head. This function
fills that wedge with a hexahedron, so the corner looks like a clean mitre rather than a gap.

Points (in the lower frame's coordinates, `h = C_DEPTH/2`, `y = C_LOWER_H`):

| Point | Position | Meaning |
|---|---|---|
| `c` | (0, y, 0) | pivot on the centre line |
| `p` | (0, y, ±h) | corner of the lower post |
| `m` | (0, y + h·tan(a/2), ±h) | mitre point |
| `q` | (0, y + h·sin a, ±h·cos a) | corner of the rotated head |

`zs = −1` when `bendDeg > 0` (gap at the back, −Z), `+1` when it bows backward. The four 2-D
points are used twice (x0 face and x1 face) to form a thin wedge. If the angle is below 0.005
rad (~0.3°) nothing is drawn.

### Exploded view effect

`rodZ = explode * 2` slides all rods 2 m forward; the top beam lifts `explode * 2.5`. See the
exploded screenshot:

![Exploded](images/04-exploded.png)

## Side ("son") frames

```cpp
struct SideFrame { float x, z, yaw, width, height; };
SIDES[4] = {
  { -7.7, -0.9,  14, 4.4, 9.6 },  // inner left
  {  7.7, -0.9, -14, 4.4, 9.6 },  // inner right
  { -12.6,-2.8,  28, 4.0, 7.6 },  // outer left
  {  12.6,-2.8, -28, 4.0, 7.6 }   // outer right
};
```

**Instancing:** one function, `drawSideFrame(W, H)`, is called four times with different
`glTranslatef` + `glRotatef(yaw, 0,1,0)`. A positive yaw turns the left frames toward the
centre; the mirror image on the right uses a negative yaw.

`drawSideFrame` draws: two posts (`pw` 0.65), a top beam (height 0.7, lip 0.1, slightly deeper),
a bottom sill (0.5), and 4 vertical rods between the posts.

## The red sun — `drawSun(rise)`

* `R = 5.2` m radius, `T = 0.25` m thickness; mounted flush on the back of the central posts at `Z = −C_DEPTH/2 − 0.005 − T/2`.
* **Easing:** `e = rise²(3 − 2·rise)` (smoothstep) — starts and ends slowly.
* **Height:** `cy = BASE_Y + 8.2 − (1 − e)·15`. At `rise=0` the centre is 15 m lower (hidden inside the dais); at `rise=1` it sits at 9.4 m. The sun slides up behind the posts, not up *in front of* them.
* A concrete **support column** (`drawBox`) is drawn only while `cy > BASE_Y` — from the dais up to the disc centre.
* **Disc geometry:** two `GL_TRIANGLE_FAN`s (front and back faces, with opposite winding and normals `±Z`) plus a `GL_QUAD_STRIP` for the rim. N = 96 segments.
* **Gradient:** `sunColor(k)` blends bottom (0.72, 0.11, 0.20) to top (0.93, 0.10, 0.30) by the vertex height. Vertex colours + `GL_SMOOTH` shading give the gradient.

## `drawMonument(state)` — assembling everything

1. `glTranslatef(0, BASE_Y, 0)` → central frame (so frames stand on the dais).
2. For each side frame: translate by `(x ± push, BASE_Y, z − explode·1.5)`, rotate by yaw, draw. `push = explode × (3.5 inner, 6.5 outer)` pulls them sideways.
3. Sun: only if `showSun` **and** not (shadow pass while the sun is still rising). While rising, the disc is partly *inside* the platform, so it must not cast a shadow. `glTranslatef(0,0,−explode·5)` moves it back in the exploded view.

## Effects of editing constants (quick reference)

| Edit | Result |
|---|---|
| `C_LOWER_H` ↑ | Taller lower part; head higher. Rod/joint code follows automatically. Sun height (`BASE_Y + 8.2`) does not — adjust it too. |
| `C_UPPER_L` ↑ | Longer head. |
| `C_POST_X` | Wider/narrower openings; rods and sill adapt. Top beam is derived from the last post. |
| `C_FLARE` | More/less outward splay of the head. |
| `C_RODS` | More/fewer rods per opening. |
| `SIDES[…]` | Moves/rotates/resizes the side frames. |
| `BASE_Y` | Raises the whole monument; also changes where shadows land. Must match the dais height in `drawPlatform` (`drawBox(... 0.9f, BASE_Y ...)`). |
| `bendDeg` default (28 in `app.cpp`) | Initial bow angle. |
