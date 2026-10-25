// app.cpp - camera, projections, model transformations, animation and HUD
// for the Shaheed Minar demo. Uses only OpenGL 1.1 + GLU (fixed function), so
// every transformation is an explicit glTranslate / glRotate / glScale /
// glMultMatrix call that can be pointed at in class.

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "minar_model.h"
#include "pigeons.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

static const float PI = 3.14159265f;
static float rad(float d) { return d * PI / 180; }
static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ------------------------------------------------------------------ state --

static int winW = 1280, winH = 800;
static bool quitRequested = false;

// Camera: orbits a target point on a sphere (yaw/pitch in degrees)
static float camYaw, camPitch, camDist, camTargetY;
static float fovY = 45;

enum ProjMode { PROJ_PERSPECTIVE, PROJ_ORTHO, PROJ_CABINET, PROJ_CAVALIER, PROJ_COUNT };
static const char* PROJ_NAMES[PROJ_COUNT] = {
    "Perspective", "Orthographic (parallel)", "Oblique - Cabinet (L=0.5, 45 deg)",
    "Oblique - Cavalier (L=1, 45 deg)"
};
static int projMode = PROJ_PERSPECTIVE;
static bool quadView = false;

// Model transformation, applied as  T * Rx * Ry * Rz * S * Shear * Mirror
static float tx, ty, tz, rx, ry, rz, scaleF, shearXY;
static bool mirrorZ;

// Animation
static MinarState minar;
static float bendTarget, sunTarget, explodeTarget;
static bool autoRotate;
static const float DEFAULT_BEND = 28;
static bool showPigeons = true;
static float pigeonTime = 0;

// Display toggles
static bool lighting = true, wireframe = false, shadows = true;
static bool showGrid = false, showAxes = false, showHelp = true;

// Mouse
static int dragButton = -1, lastX, lastY;

// Directional light in world space (towards the light)
static const float LIGHT_DIR[4] = { -0.35f, 1.0f, 0.55f, 0.0f };

struct Preset { const char* name; float yaw, pitch, dist, targetY; };
static const Preset PRESETS[6] = {
    { "Front",                     0,  6,     46, 7 },
    { "Right side",               90,  6,     46, 7 },
    { "Top",                       0, 89,     55, 0 },
    { "Isometric",                45, 35.26f, 52, 6 },
    { "Low angle (like ref. 1)",  18, -14,    26, 10 },
    { "Front, raised (ref. 2)",    0, 12,     34, 7.5f }
};
static const char* presetName = "";

static void applyPreset(int i)
{
    camYaw = PRESETS[i].yaw;
    camPitch = PRESETS[i].pitch;
    camDist = PRESETS[i].dist;
    camTargetY = PRESETS[i].targetY;
    presetName = PRESETS[i].name;
}

static void resetModel()
{
    tx = ty = tz = rx = ry = rz = 0;
    scaleF = 1;
    shearXY = 0;
    mirrorZ = false;
    autoRotate = false;
}

void appInit()
{
    resetModel();
    applyPreset(5);
    minar.bendAngle = bendTarget = DEFAULT_BEND;
    minar.sunRise = sunTarget = 1;
    minar.explode = explodeTarget = 0;
    minar.showSun = true;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);            // keeps normals unit length under glScale
    glEnable(GL_MULTISAMPLE);          // anti-aliasing, if the front end got a multisampled buffer
    glShadeModel(GL_SMOOTH);

    GLfloat ambient[] = { 0.38f, 0.38f, 0.40f, 1 };
    GLfloat diffuse[] = { 0.72f, 0.72f, 0.70f, 1 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glEnable(GL_LIGHT0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
}

void appResize(int w, int h)
{
    winW = w > 1 ? w : 1;
    winH = h > 1 ? h : 1;
}

bool appWantsQuit() { return quitRequested; }

// ---------------------------------------------------- matrices & helpers --

static void applyModelTransform()
{
    glTranslatef(tx, ty, tz);
    glRotatef(rx, 1, 0, 0);
    glRotatef(ry, 0, 1, 0);
    glRotatef(rz, 0, 0, 1);
    glScalef(scaleF, scaleF, scaleF);
    if (shearXY != 0) {
        // x' = x + k*y   (column-major: element [row 0, col 1] is m[4])
        GLfloat sh[16] = { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 };
        sh[4] = shearXY;
        glMultMatrixf(sh);
    }
    if (mirrorZ) glScalef(1, 1, -1);   // reflection through the XY plane
}

// Planar projected shadow: flattens geometry onto plane P along direction L.
//   M = (P.L) I - L P^T
static void shadowMatrix(GLfloat m[16], const float plane[4], const float L[4])
{
    float dot = plane[0] * L[0] + plane[1] * L[1] + plane[2] * L[2] + plane[3] * L[3];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            m[c * 4 + r] = (r == c ? dot : 0) - L[r] * plane[c];
}

// Solves A x = b for the 3x3 upper-left block of a column-major matrix.
static bool solve3(const GLfloat m[16], const float b[3], float x[3])
{
    // Row-major view of the block:  | A B C |
    //                               | D E F |
    //                               | G H I |
    float A = m[0], B = m[4], C = m[8];
    float D = m[1], E = m[5], F = m[9];
    float G = m[2], H = m[6], I = m[10];
    float det = A * (E * I - F * H) - B * (D * I - F * G) + C * (D * H - E * G);
    if (fabsf(det) < 1e-8f) return false;
    x[0] = ((E * I - F * H) * b[0] + (C * H - B * I) * b[1] + (B * F - C * E) * b[2]) / det;
    x[1] = ((F * G - D * I) * b[0] + (A * I - C * G) * b[1] + (C * D - A * F) * b[2]) / det;
    x[2] = ((D * H - E * G) * b[0] + (B * G - A * H) * b[1] + (A * E - B * D) * b[2]) / det;
    return true;
}

// ------------------------------------------------------------ projections --

static void setProjection(float aspect)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (projMode == PROJ_PERSPECTIVE) {
        gluPerspective(fovY, aspect, 0.5, 600);
    } else {
        // Size the parallel view volume so it frames what the perspective
        // camera sees at the target distance.
        float h = camDist * tanf(rad(fovY / 2));
        glOrtho(-h * aspect, h * aspect, -h, h, -600, 600);
        if (projMode == PROJ_CABINET || projMode == PROJ_CAVALIER) {
            // Oblique: x' = x + L cos(a) (z + d),  y' = y + L sin(a) (z + d)
            // (d = camera distance so the target plane stays in place)
            float L = projMode == PROJ_CABINET ? 0.5f : 1.0f;
            float a = -L * cosf(rad(45)), b = -L * sinf(rad(45));
            GLfloat H[16] = { 1,0,0,0,  0,1,0,0,  a,b,1,0,  a * camDist, b * camDist, 0, 1 };
            glMultMatrixf(H);
        }
    }
    glMatrixMode(GL_MODELVIEW);
}

static void setOrbitView()
{
    float cp = cosf(rad(camPitch)), sp = sinf(rad(camPitch));
    float ex = camDist * cp * sinf(rad(camYaw));
    float ey = camTargetY + camDist * sp;
    float ez = camDist * cp * cosf(rad(camYaw));
    glLoadIdentity();
    gluLookAt(ex, ey, ez, 0, camTargetY, 0, 0, 1, 0);
}

enum FixedView { VIEW_CAMERA, VIEW_FRONT, VIEW_SIDE, VIEW_TOP };

// Orthographic elevations used by the 4-viewport layout
static void setFixedView(int view, float aspect)
{
    float h = view == VIEW_TOP ? 19.0f : 11.0f;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-h * aspect, h * aspect, -h, h, 1, 400);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (view == VIEW_FRONT)      gluLookAt(0, 8, 150,  0, 8, 0,   0, 1, 0);
    else if (view == VIEW_SIDE)  gluLookAt(150, 8, 0,  0, 8, 0,   0, 1, 0);
    else                         gluLookAt(0, 150, -3, 0, 0, -3,  0, 0, -1);
}

// ------------------------------------------------------------------ scene --

static void drawBackground()
{
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glColor3f(0.86f, 0.91f, 0.97f); glVertex2f(-1, -1); glVertex2f(1, -1);
    glColor3f(0.52f, 0.69f, 0.90f); glVertex2f(1, 1);   glVertex2f(-1, 1);
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

static void drawGround()
{
    glColor3f(0.42f, 0.60f, 0.34f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-200, 0, 200); glVertex3f(200, 0, 200);
    glVertex3f(200, 0, -200); glVertex3f(-200, 0, -200);
    glEnd();
}

static void drawGrid()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.25f, 0.35f, 0.22f);
    glBegin(GL_LINES);
    for (int i = -40; i <= 40; i += 2) {
        glVertex3f((float)i, 0.02f, -40); glVertex3f((float)i, 0.02f, 40);
        glVertex3f(-40, 0.02f, (float)i); glVertex3f(40, 0.02f, (float)i);
    }
    glEnd();
}

static void drawAxes()
{
    const float L = 12;
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.9f, 0.1f, 0.1f); glVertex3f(0, 0, 0); glVertex3f(L, 0, 0);
    glColor3f(0.1f, 0.7f, 0.1f); glVertex3f(0, 0, 0); glVertex3f(0, L, 0);
    glColor3f(0.1f, 0.2f, 0.9f); glVertex3f(0, 0, 0); glVertex3f(0, 0, L);
    glEnd();
    glLineWidth(1);
    glColor3f(0.9f, 0.1f, 0.1f); glRasterPos3f(L + 0.5f, 0, 0); platformDrawString("X");
    glColor3f(0.1f, 0.6f, 0.1f); glRasterPos3f(0, L + 0.5f, 0); platformDrawString("Y");
    glColor3f(0.1f, 0.2f, 0.9f); glRasterPos3f(0, 0, L + 0.5f); platformDrawString("Z");
    glEnable(GL_DEPTH_TEST);
}

// Called with the model transform already on the modelview stack.
static void drawShadows()
{
    // Express the world light direction in model space: L_model = M^-1 L_world
    GLfloat M[16];
    glPushMatrix();
    glLoadIdentity();
    applyModelTransform();
    glGetFloatv(GL_MODELVIEW_MATRIX, M);
    glPopMatrix();
    float L[4] = { 0, 0, 0, 0 };
    if (!solve3(M, LIGHT_DIR, L) || L[1] < 0.05f) return;   // light below the dais

    // 1) Mark visible pixels of the dais top in the stencil buffer
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1, -1);
    drawDaisTop();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // 2) Project the monument onto the dais plane y = BASE_Y. The stencil is
    //    cleared on first hit so overlapping shadow polygons blend only once.
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    glPolygonOffset(-2, -2);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.05f, 0.05f, 0.1f, 0.35f);

    float plane[4] = { 0, 1, 0, -BASE_Y };
    GLfloat S[16];
    shadowMatrix(S, plane, L);
    glPushMatrix();
    glMultMatrixf(S);
    setShadowPass(true);
    drawMonument(minar);
    setShadowPass(false);
    glPopMatrix();

    glDisable(GL_BLEND);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    if (lighting) glEnable(GL_LIGHTING);
}

static void renderScene()
{
    glLightfv(GL_LIGHT0, GL_POSITION, LIGHT_DIR);   // fixed in world space
    if (lighting) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);

    drawGround();
    if (showGrid) drawGrid();
    if (lighting) glEnable(GL_LIGHTING);

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glPushMatrix();
    applyModelTransform();
    // A reflection reverses polygon winding, so front faces become clockwise
    glFrontFace(mirrorZ ? GL_CW : GL_CCW);
    drawPlatform();
    drawMonument(minar);
    if (shadows && !wireframe) drawShadows();
    if (showPigeons) drawPigeons(pigeonTime);
    glFrontFace(GL_CCW);
    glPopMatrix();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (showAxes) drawAxes();
}

// -------------------------------------------------------------------- HUD --

static const int CHAR_W = 8, LINE_H = 15;

static void begin2D(int w, int h)
{
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void end2D()
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draws a translucent box of text. (x, y) is its top-left corner in pixels,
// measured from the top-left of the window; right-aligns when x < 0.
static void drawPanel(int x, int y, char lines[][100], int n)
{
    int maxLen = 0;
    for (int i = 0; i < n; i++) {
        int len = (int)strlen(lines[i]);
        if (len > maxLen) maxLen = len;
    }
    int w = maxLen * CHAR_W + 16, h = n * LINE_H + 10;
    if (x < 0) x = winW + x - w;
    int top = winH - y;

    glColor4f(0, 0, 0, 0.55f);
    glBegin(GL_QUADS);
    glVertex2i(x, top - h); glVertex2i(x + w, top - h);
    glVertex2i(x + w, top); glVertex2i(x, top);
    glEnd();
    for (int i = 0; i < n; i++) {
        glColor3f(i == 0 ? 1.0f : 0.92f, i == 0 ? 0.82f : 0.92f, i == 0 ? 0.35f : 0.92f);
        glRasterPos2i(x + 8, top - 5 - (i + 1) * LINE_H + 4);
        platformDrawString(lines[i]);
    }
}

static void drawHUD()
{
    begin2D(winW, winH);

    char s[12][100];
    int n = 0;
    sprintf(s[n++], "SHAHEED MINAR - OpenGL 3D model");
    sprintf(s[n++], "Projection : %s", PROJ_NAMES[projMode]);
    if (projMode == PROJ_PERSPECTIVE) sprintf(s[n++], "Field of view: %.0f deg", fovY);
    sprintf(s[n++], "Camera     : yaw %.0f  pitch %.0f  dist %.1f  [%s]", camYaw, camPitch, camDist, presetName);
    sprintf(s[n++], "Translate  : (%.1f, %.1f, %.1f)", tx, ty, tz);
    sprintf(s[n++], "Rotate     : X %.0f  Y %.0f  Z %.0f deg%s", rx, ry, rz, autoRotate ? "  (auto)" : "");
    sprintf(s[n++], "Scale      : %.2f   Shear x+=k*y: %.2f   Mirror(z): %s", scaleF, shearXY, mirrorZ ? "on" : "off");
    sprintf(s[n++], "Bend       : %.1f deg   Sun: %.0f%%   Explode: %.0f%%",
            minar.bendAngle, minar.sunRise * 100, minar.explode * 100);
    sprintf(s[n++], "Light %s  Wire %s  Shadow %s  Sun %s  Pigeons %s  Grid %s  Axes %s",
            lighting ? "on" : "off", wireframe ? "on" : "off", shadows ? "on" : "off",
            minar.showSun ? "on" : "off", showPigeons ? "on" : "off",
            showGrid ? "on" : "off", showAxes ? "on" : "off");
    drawPanel(10, 10, s, n);

    if (showHelp) {
        char h[24][100];
        int m = 0;
        sprintf(h[m++], "KEYS  (I hides this panel)");
        sprintf(h[m++], "Transform  arrows: move X/Z   PgUp/PgDn: move Y");
        sprintf(h[m++], "           x/X y/Y z/Z: rotate   s/S: scale");
        sprintf(h[m++], "           h/H: shear   M: mirror   R: reset");
        sprintf(h[m++], "Projection P: cycle persp/ortho/cabinet/cavalier");
        sprintf(h[m++], "           V: 4 viewports   [ ]: field of view");
        sprintf(h[m++], "Camera     1-6: presets   0: reset camera");
        sprintf(h[m++], "           L-drag: orbit  R-drag/wheel/+ -: zoom");
        sprintf(h[m++], "Animation  Space: auto-rotate   B: bow/raise head");
        sprintf(h[m++], "           , .: bend angle   U: sunrise   E: explode");
        sprintf(h[m++], "Display    L: lighting   W: wireframe   T: shadow");
        sprintf(h[m++], "           C: sun disc   F: pigeons   G: grid");
        sprintf(h[m++], "           O: axes");
        sprintf(h[m++], "           Q / Esc: quit");
        drawPanel(-10, 10, h, m);
    }
    end2D();
}

static void drawViewLabel(const char* label, int w, int h)
{
    begin2D(w, h);
    int len = (int)strlen(label);
    glColor4f(0, 0, 0, 0.5f);
    glBegin(GL_QUADS);
    glVertex2i(6, 6); glVertex2i(18 + len * CHAR_W, 6);
    glVertex2i(18 + len * CHAR_W, 26); glVertex2i(6, 26);
    glEnd();
    glColor3f(1, 1, 1);
    glRasterPos2i(12, 12);
    platformDrawString(label);
    end2D();
}

// ---------------------------------------------------------------- display --

static void renderViewport(int x, int y, int w, int h, int view, const char* label)
{
    glViewport(x, y, w, h);
    glScissor(x, y, w, h);
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    drawBackground();

    float aspect = (float)w / h;
    if (view == VIEW_CAMERA) {
        setProjection(aspect);
        setOrbitView();
    } else {
        setFixedView(view, aspect);
    }
    renderScene();

    if (label) drawViewLabel(label, w, h);
    glDisable(GL_SCISSOR_TEST);
}

void appDisplay()
{
    glViewport(0, 0, winW, winH);
    glClearColor(0.15f, 0.15f, 0.18f, 1);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (!quadView) {
        renderViewport(0, 0, winW, winH, VIEW_CAMERA, 0);
    } else {
        int hw = winW / 2, hh = winH / 2;
        renderViewport(0,  hh, hw - 1, winH - hh, VIEW_FRONT, "Front (orthographic)");
        renderViewport(hw, hh, winW - hw, winH - hh, VIEW_SIDE, "Right side (orthographic)");
        renderViewport(0,  0,  hw - 1, hh - 1, VIEW_TOP, "Top / plan (orthographic)");
        renderViewport(hw, 0,  winW - hw, hh - 1, VIEW_CAMERA, PROJ_NAMES[projMode]);
    }

    glViewport(0, 0, winW, winH);
    if (!quadView) drawHUD();
}

// -------------------------------------------------------------- animation --

static bool approach(float& v, float target, float step)
{
    if (v == target) return false;
    if (fabsf(target - v) <= step) v = target;
    else v += target > v ? step : -step;
    return true;
}

bool appUpdate(float dt)
{
    if (dt > 0.05f) dt = 0.05f;
    bool moving = false;
    moving |= approach(minar.bendAngle, bendTarget, 18 * dt);
    moving |= approach(minar.sunRise, sunTarget, 0.25f * dt);
    moving |= approach(minar.explode, explodeTarget, 0.7f * dt);
    if (autoRotate) {
        ry = fmodf(ry + 25 * dt, 360);
        moving = true;
    }
    if (showPigeons) {
        pigeonTime += dt;
        moving = true;
    }
    return moving;
}

// ------------------------------------------------------------------ input --

void appKey(unsigned char key)
{
    switch (key) {
    // Model transformations
    case 'x': rx -= 5; break;
    case 'X': rx += 5; break;
    case 'y': ry -= 5; break;
    case 'Y': ry += 5; break;
    case 'z': rz -= 5; break;
    case 'Z': rz += 5; break;
    case 's': scaleF = clampf(scaleF / 1.05f, 0.2f, 4); break;
    case 'S': scaleF = clampf(scaleF * 1.05f, 0.2f, 4); break;
    case 'h': shearXY = clampf(shearXY - 0.05f, -1, 1); break;
    case 'H': shearXY = clampf(shearXY + 0.05f, -1, 1); break;
    case 'm': case 'M': mirrorZ = !mirrorZ; break;
    case 'r': case 'R': resetModel(); break;

    // Projection and camera
    case 'p': case 'P': projMode = (projMode + 1) % PROJ_COUNT; break;
    case 'v': case 'V': quadView = !quadView; break;
    case '[': fovY = clampf(fovY - 5, 15, 100); break;
    case ']': fovY = clampf(fovY + 5, 15, 100); break;
    case '1': case '2': case '3': case '4': case '5': case '6': applyPreset(key - '1'); break;
    case '0': applyPreset(5); fovY = 45; break;
    case '+': case '=': camDist = clampf(camDist * 0.9f, 5, 200); break;
    case '-': case '_': camDist = clampf(camDist / 0.9f, 5, 200); break;

    // Animation
    case ' ': autoRotate = !autoRotate; break;
    case 'b': case 'B': bendTarget = bendTarget > 1 ? 0 : DEFAULT_BEND; break;
    case ',': bendTarget = clampf(bendTarget - 2, -30, 45); break;
    case '.': bendTarget = clampf(bendTarget + 2, -30, 45); break;
    case 'u': case 'U':
        minar.showSun = true;
        minar.sunRise = 0;
        sunTarget = 1;
        break;
    case 'e': case 'E': explodeTarget = explodeTarget > 0.5f ? 0.0f : 1.0f; break;

    // Display
    case 'l': case 'L': lighting = !lighting; break;
    case 'w': case 'W': wireframe = !wireframe; break;
    case 't': case 'T': shadows = !shadows; break;
    case 'c': case 'C': minar.showSun = !minar.showSun; break;
    case 'f': case 'F': showPigeons = !showPigeons; break;
    case 'g': case 'G': showGrid = !showGrid; break;
    case 'o': case 'O': showAxes = !showAxes; break;
    case 'i': case 'I': showHelp = !showHelp; break;
    case 'q': case 'Q': case 27: quitRequested = true; break;
    }
}

void appSpecialKey(int key)
{
    const float step = 0.5f;
    switch (key) {
    case KEY_LEFT:      tx -= step; break;
    case KEY_RIGHT:     tx += step; break;
    case KEY_UP:        tz -= step; break;
    case KEY_DOWN:      tz += step; break;
    case KEY_PAGE_UP:   ty += step; break;
    case KEY_PAGE_DOWN: ty -= step; break;
    }
}

void appMouseButton(int button, bool down, int x, int y)
{
    dragButton = down ? button : -1;
    lastX = x;
    lastY = y;
}

void appMouseMotion(int x, int y)
{
    int dx = x - lastX, dy = y - lastY;
    lastX = x;
    lastY = y;
    if (dragButton == 0) {
        camYaw = fmodf(camYaw - dx * 0.4f, 360);
        camPitch = clampf(camPitch + dy * 0.4f, -30, 89);
        presetName = "custom";
    } else if (dragButton == 2) {
        camDist = clampf(camDist * (1 + dy * 0.01f), 5, 200);
    }
}

void appMouseWheel(int dir)
{
    camDist = clampf(dir > 0 ? camDist * 0.9f : camDist / 0.9f, 5, 200);
}
