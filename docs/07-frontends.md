# 7. Front Ends — `main_win32.cpp` and `main_glut.cpp`

A **front end** creates the window and OpenGL context, translates OS events into the `app*`
calls, drives the timer, and provides `platformDrawString`. Both do the same job; pick one at
compile time.

| | Win32 (`main_win32.cpp`) | GLUT (`main_glut.cpp`) |
|---|---|---|
| Dependencies | none (Windows only) | freeglut |
| Event loop | hand-written `PeekMessage` loop | `glutMainLoop()` |
| Timing | `QueryPerformanceCounter` | `glutTimerFunc(16, …)` + `glutGet(GLUT_ELAPSED_TIME)` |
| Idle | `WaitMessage()` when nothing animates | always ticks every 16 ms |
| Text | `wglUseFontBitmapsA` with Consolas | `GLUT_BITMAP_8_BY_13` |
| Anti-aliasing | probes `wglChoosePixelFormatARB` | `GLUT_MULTISAMPLE` flag |
| vsync | `wglSwapIntervalEXT(1)` if available | driver default |

## Win32 version

### `platformDrawString(s)`
`glListBase(fontBase − 32)` then `glCallLists(len, GL_UNSIGNED_BYTE, s)`. The font was built as
96 display lists for characters 32…127, so subtracting 32 makes ASCII code → list index.

### `basicPixelFormat()`
A `PIXELFORMATDESCRIPTOR` asking for: draw to window, OpenGL, double buffer, RGBA, 32-bit colour,
**24-bit depth, 8-bit stencil**. Depth is for hidden-surface removal; stencil is for the shadows.

### `chooseMultisampleFormat(inst)` — why so long?
Multisampling is a WGL **extension**, and extensions can only be looked up with
`wglGetProcAddress` while a GL context is current. But a window's pixel format can only be
set **once**. So the code:

1. registers a dummy window class and creates a tiny hidden **probe window**;
2. gives it a basic pixel format, creates a temporary GL context;
3. looks up `wglChoosePixelFormatARB`;
4. tries 8, then 4, then 2 samples with an attribute list (draw-to-window, OpenGL, double buffer, RGBA, 32 colour, 24 depth, 8 stencil, `SAMPLE_BUFFERS=1`, `SAMPLES=n`) — the numeric attribute codes like `0x2041` are the WGL_*_ARB constants;
5. destroys the probe window and context;
6. returns the format number, or 0 if the driver has none.

`main` then uses that format for the real window (falling back to `ChoosePixelFormat` when 0).
The effect is smooth edges on thin rods.

### `WndProc` — the OS message → app mapping

| Message | Action |
|---|---|
| `WM_SIZE` | `appResize(width, height)` from `lParam` |
| `WM_CHAR` | `appKey(wParam)` — gives correct upper/lower case and `+ - [ ]` |
| `WM_KEYDOWN` | arrows / PgUp / PgDn → `appSpecialKey` (these do not produce `WM_CHAR`) |
| `WM_*BUTTONDOWN` | `SetCapture` (keep receiving moves outside the window) + `appMouseButton(button, true, x, y)` |
| `WM_*BUTTONUP` | `ReleaseCapture` + `appMouseButton(0, false, …)` |
| `WM_MOUSEMOVE` | `appMouseMotion` |
| `WM_MOUSEWHEEL` | `appMouseWheel(+1/−1)` from the sign of the wheel delta |
| `WM_PAINT` | `ValidateRect` only — drawing is done in the main loop, not here |
| `WM_ERASEBKGND` | return 1 — don't let Windows flash-erase the window |
| `WM_DESTROY` | `PostQuitMessage` |

After any handled message `dirty = true`, requesting a redraw. `(short)LOWORD(lp)` casts keep negative mouse coordinates correct.

### `main()`
1. Register the window class (`CS_OWNDC` so the device context persists for GL) and create a 1280×800 client-area window (`AdjustWindowRect` accounts for borders).
2. Choose a pixel format, create and activate the GL context, enable vsync.
3. Create the font and display lists.
4. `appInit()`, `appResize(client size)`, show window.
5. Loop:
   ```
   if (!animating && !dirty) WaitMessage();     // sleep until an event
   drain messages (PeekMessage → TranslateMessage → DispatchMessage)
   if (appWantsQuit()) DestroyWindow
   dt = elapsed seconds; animating = appUpdate(dt);
   if (animating || dirty) { appDisplay(); SwapBuffers; dirty = false; }
   ```
6. Clean up the context.

## GLUT version

* `glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE)` — all the same buffers.
* Callbacks: `display` (`appDisplay` + `glutSwapBuffers`), `keyboard` (`appKey`; exits if `appWantsQuit()`), `special` (maps `GLUT_KEY_*` to `KEY_*`), `mouse`, `motion`, and `glutReshapeFunc(appResize)`.
* **Mouse wheel:** freeglut reports it as buttons 3 and 4; code converts to `appMouseWheel(+1/−1)` on `GLUT_DOWN`.
* `timer`: every 16 ms (≈60 Hz) computes `dt`, calls `appUpdate`, and `glutPostRedisplay()` if something moved.
* Each input callback ends with `glutPostRedisplay()`.

## Adding a new front end

Implement a window with RGBA + depth 24 + stencil 8, call the `app*` functions from your event
handlers, call `appUpdate(dt)` each tick and `appDisplay()` + swap buffers, and define
`platformDrawString`. Nothing in `app.cpp` needs to change.

## Effects of editing

| Edit | Result |
|---|---|
| Change `1280, 800` | Initial window size (in both files; `winW/winH` in `app.cpp` just defaults). |
| `pfd.cStencilBits = 0` (and the matching `0x2023` attribute) | Shadows silently disappear (the stencil test never passes). |
| Remove `swapInterval(1)` | Higher frame rate, possible tearing, more CPU/GPU. |
| `glutTimerFunc(16, …)` | Larger number → lower frame rate in GLUT. |
| Remove `WaitMessage` branch | Constant 100 % CPU busy loop even when idle. |
