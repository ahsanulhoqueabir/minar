# 9. Glossary

| Term | Plain-language meaning | Where used |
|---|---|---|
| **Fixed-function pipeline** | Old-style OpenGL where you set states (`glEnable(GL_LIGHTING)`) instead of writing shaders. | whole project |
| **Immediate mode** | Sending vertices one by one between `glBegin` and `glEnd`. | `geometry.cpp`, everywhere |
| **Vertex / normal** | A corner point / the direction a surface faces (needed for lighting). | `drawHex` |
| **Newell's method** | A robust way to find a polygon's normal by summing edge terms. | `drawHex` |
| **Hexahedron** | Any solid with 6 quad faces and 8 corners (boxes, tapered posts, wings). | `drawHex` |
| **Winding** | The vertex order (clockwise/counter-clockwise) seen from the front. Defines which side is "front". | `drawHex`, `glFrontFace` |
| **Model / View / Projection** | Place the object in the world / position the camera / flatten to the screen. | `app.cpp` |
| **Matrix stack** | `glPushMatrix`/`glPopMatrix` — save/restore the current transform. Basis of hierarchies. | head, wings, frames |
| **Hierarchical modelling** | Child parts move with their parent (head with frame, wing with body). | `drawCentralFrame`, `drawWing` |
| **Instancing** | Drawing one shape several times under different transforms. | side frames |
| **Column-major** | OpenGL stores matrices column by column: `m[col*4+row]`. | `glMultMatrixf` calls |
| **Translate / rotate / scale** | Basic transforms. | `applyModelTransform` |
| **Shear** | Slants a shape: `x' = x + k·y`. | `shearXY` |
| **Reflection (mirror)** | A negative scale that flips the object like a mirror. | `mirrorZ` |
| **Composite transform** | Several transforms multiplied into one; order matters. | `T·Rx·Ry·Rz·S·Shear·Mirror` |
| **Perspective projection** | Far things look smaller (like a camera/eye). | `gluPerspective` |
| **Orthographic projection** | Parallel projection; no shrinking with distance (engineering drawings). | `glOrtho` |
| **Oblique projection** | Orthographic where the depth axis is drawn slanting. Cabinet: depth ×0.5; cavalier: depth ×1. | `setProjection` |
| **Field of view (FOV)** | Vertical opening angle of the perspective camera. | `fovY`, `[` `]` |
| **Aspect ratio** | width ÷ height; keeps shapes from stretching. | `setProjection` |
| **Viewport / scissor** | The part of the window drawn to / clipping of clears. | quad view |
| **Orbit camera** | Camera on a sphere around a target, controlled by yaw, pitch and distance. | `setOrbitView` |
| **`gluLookAt`** | Builds the view matrix from eye, target, up. | cameras |
| **Directional light** | A light infinitely far away (parallel rays), `w = 0`. | `LIGHT_DIR` |
| **Ambient / diffuse** | Constant base light / light depending on the angle to the surface. | `appInit` |
| **Gouraud (smooth) shading** | Colours interpolated across a polygon. | sun gradient, rods |
| **Z-buffer (depth buffer)** | Per-pixel nearest distance; hides what is behind. | `GL_DEPTH_TEST` |
| **Z-fighting** | Flicker when two surfaces are at nearly equal depth; fixed by moving one slightly. | grid at 0.02, polygon offset |
| **Stencil buffer** | Per-pixel integer used as a mask. | shadows |
| **Planar projected shadow** | Squashing geometry onto a plane along the light direction. | `shadowMatrix` |
| **Blending** | Mixing the new colour with what is already on screen using alpha. | shadow, HUD |
| **Multisampling (MSAA)** | Anti-aliasing: several samples per pixel to smooth edges. | front ends |
| **Double buffering** | Draw to a hidden buffer then swap, avoiding flicker. | `SwapBuffers` |
| **vsync** | Swap only at monitor refresh, preventing tearing. | `wglSwapIntervalEXT` |
| **HUD** | Heads-up display: 2-D text drawn over the 3-D view. | `drawHUD` |
| **Delta time (dt)** | Seconds since the last frame, making speeds frame-rate independent. | `appUpdate` |
| **Easing / smoothstep** | A curve `3t²−2t³` that starts and ends slowly. | sun rise |
| **Parametric path** | Position as a function of a parameter θ: `(x(θ), y(θ), z(θ))`. | pigeons |
| **Yaw / pitch / roll** | Rotation about up / sideways / forward axes (heading, nose up/down, banking). | pigeons, camera |
| **Mitre joint** | A diagonal cut where two members meet at an angle. | `drawBendJoint` |
| **Exploded view** | Parts pulled apart to show how they fit. | `E` key |
| **WGL** | Windows' glue between OpenGL and the window system. | `main_win32.cpp` |
| **GLUT / freeglut** | Portable toolkit for windows and input in OpenGL demos. | `main_glut.cpp` |

## Mnemonic: reading a transform sequence

```
glTranslatef(...)   // 3rd – applied last to the vertex
glRotatef(...)      // 2nd
glScalef(...)       // 1st – applied first to the vertex
drawThing();
```
Read **bottom-up** to follow what happens to a vertex; read **top-down** to follow how the
coordinate system moves. Both views are equivalent — pick whichever is easier.
