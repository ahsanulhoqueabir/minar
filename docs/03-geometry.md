# 3. Geometry Primitives — `geometry.h` / `geometry.cpp`

All shapes in the project are made from these few functions. They use OpenGL
**immediate mode** (`glBegin … glEnd`) and always supply a **normal** per face/vertex so
lighting works.

## Types

```cpp
struct V3 { float x, y, z; };
inline V3 v3(float x, float y, float z);   // convenient constructor
```

## `drawHex(const V3 v[8])` — the workhorse

A *hexahedron* is any 6-faced solid with 8 corners. A box, a tapered post, a skewed
wedge, a wing, a beak and a tail feather are all hexahedra.

**Vertex layout (the caller's contract):**

```
 4────────7        vertices 0-3 = one face (e.g. bottom)
 │\       │\       vertices 4-7 = the opposite face (e.g. top)
 │ 5────────6      vertex i is joined to vertex i+4
 0─│──────3 │
  \│       \│
   1────────2
```

**How it works, step by step**

1. `F[6][4]` lists the six faces as vertex indices: bottom `{0,1,2,3}`, top `{4,5,6,7}`, and the four sides.
2. It computes the **centroid** `c` of all 8 vertices.
3. For each face it computes the normal with **Newell's method**: a sum over the face edges
   `n.x += (a.y−b.y)(a.z+b.z)` (and cyclic for y, z). Unlike a plain cross product this is
   robust for slightly non-planar quads.
4. It compares the normal with the vector *centroid → face centre*. If the dot product is
   negative, the normal points inward, so it sets `flip = true`, negates the normal, **and
   reverses the vertex order** so the face is still counter-clockwise when seen from outside.
5. Emits `glNormal3f` + four `glVertex3f` per face as `GL_QUADS`.
6. Degenerate faces (zero-area, `len < 1e-9`) are skipped — this is how `drawBendJoint` can pass repeated points.

**Consequence:** you may list the 8 points in *any winding*; faces and normals still come out
right. Back-face culling is not enabled in this project, but the correct winding matters for
`glFrontFace` under mirroring (see [06](06-app-camera-projection.md#mirror-and-winding)) and
for the lighting model (`GL_LIGHT_MODEL_TWO_SIDE` is off, so the *front* side is the lit one).

## `drawBox(x0, x1, y0, y1, z0, z1)`

Builds the 8 corners of an axis-aligned box and calls `drawHex`. Corners 0–3 are the bottom
(`y0`), 4–7 the top (`y1`). Used for plinth, steps, dais, frame posts, sills, top beams,
tail/wing bars, the sun's support column.

## `drawSlantedPost(xb0, xb1, xt0, xt1, y0, y1, z0, z1)`

Same as a box, but the x-extent is `[xb0, xb1]` at the bottom (`y0`) and `[xt0, xt1]` at the
top (`y1`). If the top x-values are shifted by the same amount the post leans; if they are
spread differently it tapers.
Used for the three posts of the bowed head: the outer posts get `+/−0.45` of "flare".

## `drawRod(a, b, radius, segments = 14)`

A capped cylinder between points `a` and `b` with **smooth** normals (looks round when lit).

1. `d` = normalised axis direction.
2. Choose a `helper` vector not parallel to `d` (`(0,1,0)` unless the rod is nearly vertical, then `(1,0,0)`).
3. `u = normalize(d × helper)`, `w = d × u` → an orthonormal frame perpendicular to the rod.
4. **Side:** a `GL_QUAD_STRIP`; for each angle `t` the outward normal is `n = u·cos t + w·sin t`; vertices are `a + n·r` and `b + n·r`.
5. **Caps:** two `GL_TRIANGLE_FAN`s with normal `±d`. The second cap runs the angle in the opposite direction (`sgn`) so its winding faces outward.

*Effect of `segments`:* 14 gives a smooth look; 6 shows visible facets; 30 is rounder but costs more vertices (the model has ~50 rods).

## `drawEllipsoid(center, rx, ry, rz, slices = 16, stacks = 10)`

A latitude/longitude sphere scaled per axis. For every stack, a `GL_QUAD_STRIP` runs around
the longitude. The surface point is `(cos φ sin θ, sin φ, cos φ cos θ)` scaled by the radii.
The **normal of an ellipsoid** is the unit-sphere point divided by the radii (then
normalised) — a nice small example of why normals do not scale like positions.
Used for pigeon bodies, necks, heads and eyes.

## Local helpers (not exported)

| Function | Purpose |
|---|---|
| `cross(a, b)` | vector cross product |
| `normalize(a)` | unit vector (returns `a` unchanged when length 0) |

## What would change if…

| Change | Effect |
|---|---|
| Remove the `flip` logic in `drawHex` | Faces on one side of every solid would be lit from the wrong side (dark) and normals point inward. |
| Use `glNormal` of a plain cross product | Fine for flat faces; wrong for badly twisted quads. |
| Increase `segments` default | Rods rounder, more triangles. |
| Make `drawEllipsoid` slices/stacks smaller | Pigeons look faceted (they are small on screen, so 8×6 is OK for eyes). |
