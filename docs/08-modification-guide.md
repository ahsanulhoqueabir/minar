# 8. Modification Guide — "If I change X, what happens?"

Practical recipes, grouped by what you want to do. After any edit, rebuild with `build.bat`.

## Appearance

| I want to… | Edit | Effect / caution |
|---|---|---|
| Change concrete or iron colour | `concrete()` / `iron()` in [minar_model.cpp](../src/minar_model.cpp) | Affects every frame. |
| Change sky | two `glColor3f` in `drawBackground()` ([app.cpp](../src/app.cpp)) | Bottom colour is at the horizon, top at the zenith of the screen. |
| Change grass | `drawGround()` colour | – |
| Change sun colour | `sunColor()` | Interpolates bottom → top. |
| Make lighting brighter | `ambient` / `diffuse` in `appInit()` | Values over ~1 saturate to white. |
| Move the light | `LIGHT_DIR` | The shadow direction follows automatically. If `y ≤ 0.05` after the model transform, shadows turn off (light below the dais). |
| Lighter/darker shadow | alpha in `glColor4f(0.05,0.05,0.1,0.35)` in `drawShadows` | – |
| Smoother rods | `segments` in `drawRod` (default 14) | More polygons. |
| Smoother sun | `N` in `drawSun` (96) | – |

## Shape and proportions

| I want to… | Edit | Effect |
|---|---|---|
| Taller/shorter central frame | `C_LOWER_H`, `C_UPPER_L` | Rods and joints update. The sun's `BASE_Y + 8.2` centre height is fixed; edit it to keep the sun centred. |
| Wider central frame | `C_POST_X` | Top beam is derived from the last post, so it follows. Sun radius `R = 5.2` may need to grow. |
| Thicker posts | `C_POST_W`, `C_DEPTH` | The sun's `Z` offset uses `C_DEPTH` and follows. |
| More rods | `C_RODS` (central), `rods` in `drawSideFrame` | Evenly respaced. |
| Move or add a side frame | `SIDES[4]` (and the loop count `4` in `drawMonument`) | `x` sign decides which side the explode motion pushes toward. |
| Bigger platform | `drawPlatform` **and** `drawDaisTop` | The shadow receiver must match the dais, or shadows are cut off / float. |
| Raise the dais | `BASE_Y` and the `0.9f…BASE_Y` box | Everything standing on it shifts; pigeons do not (they use absolute heights). |
| Add a new solid part | Use `drawBox` / `drawHex` inside `drawMonument` between `glPushMatrix`/`glPopMatrix`, after calling `concrete()` or `color()` | Because it is drawn from `drawMonument`, it automatically casts a shadow and takes part in wireframe/mirror/shear. |

## Behaviour and animation

| I want to… | Edit | Effect |
|---|---|---|
| Different starting bend | `DEFAULT_BEND` (`app.cpp`) | Also the value `B` returns to. |
| Slower/faster bow | `18 * dt` in `appUpdate` | degrees/s. |
| Longer sunrise | `0.25f * dt` | 0.25 → 4 s; 0.1 → 10 s. |
| Different starting camera | `applyPreset(5)` in `appInit` and `'0'` in `appKey` | Use another index 0–5. |
| New camera preset | add a row to `PRESETS` and raise the array size (6); the keys `1`–`6` are hard-wired in `appKey`, so extend that `case` list too | – |
| Faster auto-rotate | `25 * dt` | degrees/s about Y. |
| Zoom range | `clampf(…, 5, 200)` (appears in 4 places) | Keep consistent. |
| New toggle key | add a `static bool`, a `case` in `appKey`, a line in `drawHUD()` (status and help) | The help buffer `h[24]` and status buffer `s[12]` have limited rows. |
| No pigeons by default | `showPigeons = false` | With nothing else moving, the Win32 loop idles. |

## Projection / transform experiments

| Try | Where | You will see |
|---|---|---|
| Different oblique angle | `rad(45)` in `setProjection` | Depth axis slants more vertically (e.g. 30°) or horizontally (60°). |
| Cabinet `L` | `0.5f` | Closer to 1 = cavalier. |
| Different ortho size | `h = camDist * tanf(...)` | Smaller → zoom in. |
| Different shear | `sh[4]` → `sh[1]` (x/y swapped) | `y' = y + k·x`. |
| Reflect another plane | `glScalef(-1,1,1)` / `(1,-1,1)` | Remember the `glFrontFace` flip for any single negative scale. |
| Change composite order | reorder the `glRotate`/`glScale` lines in `applyModelTransform` | Matrix multiplication is non-commutative: rotating-then-translating ≠ translating-then-rotating, and rotating about X then Y differs from Y then X. |
| Remove `GL_NORMALIZE` | `appInit` | Scaling the model makes lighting brighter/darker; shadows unaffected. |
| Disable depth test in `appInit` | | Parts overlap in draw order, not distance. |

## Pigeons

See the table in [05-pigeons](05-pigeons.md#effects-of-editing).

## Extending: ideas with starting points

1. **Textures:** add `glGenTextures` + `glTexImage2D` in `appInit`, enable `GL_TEXTURE_2D` for the dais, and add `glTexCoord2f` calls in `drawDaisTop`/`drawBox`.
2. **Backface culling:** `glEnable(GL_CULL_FACE)` in `appInit` — works because `drawHex` guarantees outward counter-clockwise faces (the sun's fans/rods also face outward). Verify visually with `M`.
3. **Second light:** `GL_LIGHT1` configured in `appInit` and positioned in `renderScene`.
4. **Screenshots:** read pixels with `glReadPixels` in a key handler and write a BMP.
5. **Better shadows:** shadow maps need FBOs (not available in OpenGL 1.1 — would need an extension loader).

## Pitfalls checklist

* Always pair `glPushMatrix` / `glPopMatrix`; an unbalanced push eventually overflows the stack (limit ≥ 32) and corrupts later frames.
* Call `glNormal3f` before vertices in new immediate-mode shapes, or lighting looks wrong.
* Always route colours in `minar_model.cpp` through `color()` (not raw `glColor3f`) — otherwise the shadow turns coloured for that part. (The pigeons use raw `glColor3f` but are not part of the shadow pass.)
* Keep `drawDaisTop` consistent with the dais box.
* New `.cpp` files must be added to `build.bat` and the README's compile line.
* Do not call GL functions before `appInit()`; there is no context until the front end creates one.
