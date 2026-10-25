// main_win32.cpp - plain Win32 + WGL front end (no GLUT needed).
//
// Build:  g++ -O2 -o shaheed_minar.exe src/main_win32.cpp src/app.cpp
//             src/minar_model.cpp src/geometry.cpp src/pigeons.cpp
//             -lopengl32 -lglu32 -lgdi32 -mwindows

#include <windows.h>
#include <GL/gl.h>
#include <string.h>
#include "app.h"

static HDC hdc;
static GLuint fontBase;
static bool dirty = true;

void platformDrawString(const char* s)
{
    glListBase(fontBase - 32);
    glCallLists((GLsizei)strlen(s), GL_UNSIGNED_BYTE, s);
}

static PIXELFORMATDESCRIPTOR basicPixelFormat()
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    return pfd;
}

// Finds a multisampled (anti-aliased) pixel format via WGL_ARB_pixel_format.
// That extension can only be queried from a live context, and a window's
// pixel format can be set only once, so a throwaway window is used.
// Returns 0 when the driver offers no multisampling.
static int chooseMultisampleFormat(HINSTANCE inst)
{
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = inst;
    wc.lpszClassName = "ShaheedMinarProbe";
    RegisterClassA(&wc);
    HWND probe = CreateWindowA("ShaheedMinarProbe", "", WS_OVERLAPPEDWINDOW,
                               0, 0, 16, 16, NULL, NULL, inst, NULL);
    HDC dc = GetDC(probe);
    PIXELFORMATDESCRIPTOR pfd = basicPixelFormat();
    SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd);
    HGLRC rc = wglCreateContext(dc);
    wglMakeCurrent(dc, rc);

    typedef BOOL (WINAPI *ChooseFormatFn)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
    ChooseFormatFn choose = (ChooseFormatFn)wglGetProcAddress("wglChoosePixelFormatARB");
    int format = 0;
    for (int samples = 8; choose && samples >= 2 && !format; samples /= 2) {
        const int attribs[] = {
            0x2001, 1,          // WGL_DRAW_TO_WINDOW_ARB
            0x2010, 1,          // WGL_SUPPORT_OPENGL_ARB
            0x2011, 1,          // WGL_DOUBLE_BUFFER_ARB
            0x2013, 0x202B,     // WGL_PIXEL_TYPE_ARB = WGL_TYPE_RGBA_ARB
            0x2014, 32,         // WGL_COLOR_BITS_ARB
            0x2022, 24,         // WGL_DEPTH_BITS_ARB
            0x2023, 8,          // WGL_STENCIL_BITS_ARB
            0x2041, 1,          // WGL_SAMPLE_BUFFERS_ARB
            0x2042, samples,    // WGL_SAMPLES_ARB
            0
        };
        int f;
        UINT count = 0;
        if (choose(dc, attribs, NULL, 1, &f, &count) && count > 0) format = f;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(probe, dc);
    DestroyWindow(probe);
    return format;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_SIZE:
        appResize(LOWORD(lp), HIWORD(lp));
        break;
    case WM_CHAR:
        appKey((unsigned char)wp);
        break;
    case WM_KEYDOWN:
        switch (wp) {
        case VK_LEFT:  appSpecialKey(KEY_LEFT); break;
        case VK_RIGHT: appSpecialKey(KEY_RIGHT); break;
        case VK_UP:    appSpecialKey(KEY_UP); break;
        case VK_DOWN:  appSpecialKey(KEY_DOWN); break;
        case VK_PRIOR: appSpecialKey(KEY_PAGE_UP); break;
        case VK_NEXT:  appSpecialKey(KEY_PAGE_DOWN); break;
        default: return DefWindowProc(hwnd, msg, wp, lp);
        }
        break;
    case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
        SetCapture(hwnd);
        appMouseButton(msg == WM_LBUTTONDOWN ? 0 : msg == WM_MBUTTONDOWN ? 1 : 2,
                       true, (short)LOWORD(lp), (short)HIWORD(lp));
        break;
    case WM_LBUTTONUP: case WM_MBUTTONUP: case WM_RBUTTONUP:
        ReleaseCapture();
        appMouseButton(0, false, (short)LOWORD(lp), (short)HIWORD(lp));
        break;
    case WM_MOUSEMOVE:
        appMouseMotion((short)LOWORD(lp), (short)HIWORD(lp));
        break;
    case WM_MOUSEWHEEL:
        appMouseWheel((short)HIWORD(wp) > 0 ? 1 : -1);
        break;
    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    dirty = true;
    return 0;
}

int main()
{
    HINSTANCE inst = GetModuleHandle(NULL);
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "ShaheedMinar";
    RegisterClassA(&wc);

    RECT r = { 0, 0, 1280, 800 };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindowA("ShaheedMinar", "Shaheed Minar - OpenGL 3D Model",
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                              CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                              NULL, NULL, inst, NULL);
    hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = basicPixelFormat();
    int format = chooseMultisampleFormat(inst);
    if (!format) format = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, format, &pfd);
    HGLRC rc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, rc);

    // Turn on vsync if the driver supports it
    typedef BOOL (WINAPI *SwapIntervalFn)(int);
    SwapIntervalFn swapInterval = (SwapIntervalFn)wglGetProcAddress("wglSwapIntervalEXT");
    if (swapInterval) swapInterval(1);

    HFONT font = CreateFontA(-14, 8, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS,
                             CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FIXED_PITCH, "Consolas");
    SelectObject(hdc, font);
    fontBase = glGenLists(96);
    wglUseFontBitmapsA(hdc, 32, 96, fontBase);

    appInit();
    GetClientRect(hwnd, &r);
    appResize(r.right, r.bottom);
    ShowWindow(hwnd, SW_SHOW);

    LARGE_INTEGER freq, last, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);
    bool running = true, animating = true;
    while (running) {
        MSG msg;
        if (!animating && !dirty) WaitMessage();
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;
        if (appWantsQuit()) { DestroyWindow(hwnd); continue; }

        QueryPerformanceCounter(&now);
        float dt = (float)(now.QuadPart - last.QuadPart) / freq.QuadPart;
        last = now;
        animating = appUpdate(dt);
        if (animating || dirty) {
            appDisplay();
            SwapBuffers(hdc);
            dirty = false;
        }
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    return 0;
}
