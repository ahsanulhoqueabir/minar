# 5. Pigeons — `pigeons.h` / `pigeons.cpp`

Eight rock pigeons fly around the monument forever. The file has two halves: the **model**
(how one bird is built and flapped) and the **flights** (where each bird goes).

```cpp
void drawPigeons(float t);   // t = animation time in seconds
```

Because flight paths are periodic functions of `t`, no state is stored: the same `t` always
gives the same picture, and increasing `t` loops forever.

## The bird model

Local frame: **forward = +Z, up = +Y, left wing on +X**, units in metres (a real pigeon is
~0.33 m). `PIGEON_SCALE = 2.2` enlarges them so they are visible next to a 14 m monument.

### `drawPigeon(flapDeg)`

| Part | How |
|---|---|
| Body | `drawEllipsoid` 0.075 × 0.07 × 0.16 (long in Z) |
| Neck (iridescent green) | smaller ellipsoid, shifted forward/up |
| Head | ellipsoid further forward/up |
| Beak | a tapered `drawHex` (wide at the head, narrow at the tip) |
| Eyes | two tiny orange ellipsoids at x = ±0.034 |
| Tail | `drawTailSegment` ×2 — a fan-shaped hexahedron plus a dark band at the end |
| Wings | `drawWing(+1)` and `drawWing(−1)` |

### `drawWing(side, flapDeg)` — a two-joint hierarchy

`side` is +1 for the left wing, −1 for the right; multiplying x-values and angles by `side`
mirrors one definition into both wings.

```cpp
glPushMatrix();
glTranslatef(side*0.05, 0.03, 0.02);   // shoulder on the body
glRotatef(side*flapDeg, 0,0,1);        // shoulder joint: rotate about Z (flap up/down)
   draw inner wing (drawHex, length 0.2) + two dark bars
glTranslatef(side*0.2, 0, 0);          // move to the elbow (end of inner wing)
glRotatef(side*flapDeg*0.7, 0,0,1);    // elbow adds 70 % more bend
   draw outer wing (drawHex, length 0.24)
glPopMatrix();
```

Nested transforms mean that rotating the shoulder carries the elbow and outer wing with it;
then the elbow rotation *adds* to it. One angle (`flapDeg`) therefore creates a natural
whip-like flap. The wing tip ends up bending 1.7× the shoulder angle.

## Flight paths

```cpp
struct FlightPath { PathShape shape; float radius, height, speed, wobble, bob; int lobes; };
```

| Path | Shape | Notes |
|---|---|---|
| 0 | LOOP, radius 17, height 13, speed 0.32 rad/s, wobble 3 (3 lobes), bob 2 | counter-clockwise flock of 4 birds |
| 1 | LOOP, radius 21, height 16, speed **−0.24** | opposite direction, higher, 3 birds |
| 2 | FIGURE_EIGHT, radius 15, height 12.5, speed 0.30 | single bird crossing in front of the bowed head |

### `pathPoint(path, θ, p[3])`

*LOOP:* a circle with a wavy radius and height
```
r = radius + wobble·sin(lobes·θ)
x = r·cosθ,   y = height + bob·sin(2θ),   z = r·sinθ
```
*FIGURE_EIGHT:*
```
x = radius·sinθ,   y = height + bob·cosθ,   z = 9 + 6·sin(2θ)
```
x oscillates once while z oscillates twice per cycle → a lemniscate (∞) in the XZ plane.
`z` ranges from 3 to 15 m, so the bird always stays in front of the monument (the code
comment notes it is kept clear of the bowed head where the path crosses). The loops (radius
≥ 14 m after wobble) circle *outside* all frames.

### `headingDeg(path, θ, dir)`
Compares the path at `θ` and `θ + dir·0.01` and returns `atan2(dx, dz)` in degrees = the compass heading. Used to estimate **turn rate**.

## `drawPigeons(t)` — placing every bird

For each of the 8 entries in `BIRDS[]` (`path`, `lag`, formation `side`/`up`, `flapOffset`):

1. **Path parameter:** `th = speed·t − dir·lag`. `lag` places followers behind the leader (in path parameter units).
2. **Position:** `pathPoint(th)`.
3. **Orientation from the path:** take another point 0.01 ahead.
   * `yaw = atan2(dx, dz)` – facing direction;
   * `pitch = −atan2(dy, horizontalDistance)` – nose up/down while the path climbs/descends.
4. **Banking:** estimate how fast yaw changes (compare heading 0.2 s ahead), then `roll = −0.6·dYaw/0.2`, clamped to ±35°. Birds lean into turns like real ones.
   `dYaw` is wrapped to (−180, 180] so crossing the 0°/360° boundary does not create a huge false turn.
5. **Flapping:** a smooth glide/flap mix
   ```
   g    = 0.5 + 0.5·sin(0.7·t + 3·flapOffset)       // slow envelope 0..1
   k    = smoothstep-like ramp of g between 0.2 and 0.6   // 0 = glide, 1 = full flap
   flap = 8 + (10 + 28k)·sin(2π·4.5·t + flapOffset)  // degrees
   ```
   4.5 flaps/s. At `k = 0` the amplitude is ±10° (wings almost level, gliding); at `k = 1`, ±38°.
   `flapOffset` desynchronises birds.
6. **Matrix stack order** (this order matters!):
   ```
   translate(position) → rotateY(yaw) → translate(formation side, up)
   → rotateX(pitch) → rotateZ(roll) → scale(2.2) → drawPigeon
   ```
   The formation offset is applied *after* the yaw so it is in the bird's own frame (left/right of the leader, regardless of heading).

## Effects of editing

| Edit | Result |
|---|---|
| `PIGEON_SCALE` | Bigger/smaller birds. |
| `PATHS[].radius` too small | Birds fly through the monument. Keep loops ≥ ~14 m (the dais half-width is 16). |
| `speed` sign | Reverses direction (the code handles it via `dir`). |
| Larger `lag` | More spread-out flock. |
| `4.5` flap frequency | Faster/slower wing beats. |
| `-0.6f` bank factor | More/less leaning; clamp ±35. |
| Add a bird | Add a row to `BIRDS[]`, and raise the `8` in the `drawPigeons` loop and in the array size. |
| Add a path | Add to `PATHS[]` (array size `3`) and reference it by index from a bird. |
| `F` key | Toggles `showPigeons` in `app.cpp` — pigeons are also inside the model transform, so scaling/rotating the monument scales/rotates the flight. |
