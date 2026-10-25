// minar_model.cpp - geometry of the Central Shaheed Minar (Dhaka).
//
// The monument is a central "mother" frame whose upper part bows forward,
// flanked by four shorter "son" frames (two each side) arranged on an arc,
// all standing on a raised marble dais. Each frame is a concrete rectangle
// filled with thin round iron rods. A red sun disc is mounted on the back of
// the central frame.
//
// The concrete parts are hexahedra (8 vertices, possibly tapered or skewed).
// The bowed head of the central frame is a child of the lower frame in the
// transformation hierarchy: translate to the bend line, rotate about X, draw.

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <math.h>
#include "geometry.h"
#include "minar_model.h"

const float BASE_Y = 1.2f;

static const float PI = 3.14159265f;

// ---------------------------------------------------------------- colours --

static bool g_shadowPass = false;

void setShadowPass(bool on) { g_shadowPass = on; }

static void color(float r, float g, float b) { if (!g_shadowPass) glColor3f(r, g, b); }

static void concrete() { color(0.93f, 0.93f, 0.94f); }
static void iron()     { color(0.17f, 0.17f, 0.19f); }

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

// --------------------------------------------------------------- platform --

void drawPlatform()
{
    // Ground-level plinth
    color(0.80f, 0.78f, 0.75f);
    drawBox(-19, 19, 0, 0.9f, -12, 4);

    // Steps up to the plinth
    color(0.86f, 0.84f, 0.81f);
    drawBox(-9, 9, 0, 0.6f, 4, 5.2f);
    drawBox(-9, 9, 0, 0.3f, 5.2f, 6.4f);

    // White marble dais carrying the frames, with one step in front
    color(0.96f, 0.96f, 0.95f);
    drawBox(-16, 16, 0.9f, BASE_Y, -10, 1.8f);
    drawBox(-6, 6, 0.9f, 1.05f, 1.8f, 2.6f);
}

void drawDaisTop()
{
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-16, BASE_Y, 1.8f);
    glVertex3f( 16, BASE_Y, 1.8f);
    glVertex3f( 16, BASE_Y, -10);
    glVertex3f(-16, BASE_Y, -10);
    glEnd();
}

// ---------------------------------------------------------- central frame --

static const float C_POST_X[3]  = { -3.6f, 0.0f, 3.6f };  // post centre lines
static const float C_FLARE[3]   = { -0.45f, 0.0f, 0.45f }; // sideways splay of the head
static const float C_POST_W     = 0.9f;
static const float C_DEPTH      = 0.75f;
static const float C_LOWER_H    = 8.6f;   // height of the bend above the dais
static const float C_UPPER_L    = 5.4f;   // length of the bowed head
static const float C_SILL_H     = 0.6f;
static const float C_TOPBAR_H   = 0.9f;
static const float C_TOPBAR_LIP = 0.2f;   // overhang of the top beam
static const int   C_RODS       = 5;      // iron rods per opening
static const float ROD_R        = 0.05f;  // iron rod radius

// Fills the wedge that opens on the outer side of the bend between the lower
// post (ends at y = C_LOWER_H) and the rotated head (starts there), giving a
// sharp mitred corner. Drawn in the lower frame's coordinates.
static void drawBendJoint(float x0, float x1, float bendDeg)
{
    float a = fabsf(bendDeg) * PI / 180;
    if (a < 0.005f) return;
    float zs = bendDeg > 0 ? -1.0f : 1.0f;   // the gap is on the side away from the lean
    float h = C_DEPTH / 2, y = C_LOWER_H;

    V3 c = v3(0, y, 0);                                  // pivot on the centre line
    V3 p = v3(0, y, zs * h);                             // corner of the lower post
    V3 m = v3(0, y + h * tanf(a / 2), zs * h);           // mitre point
    V3 q = v3(0, y + h * sinf(a), zs * h * cosf(a));     // corner of the head
    V3 v[8] = { c, p, m, q, c, p, m, q };
    for (int i = 0; i < 4; i++) { v[i].x = x0; v[i + 4].x = x1; }
    drawHex(v);
}

static void drawCentralFrame(float bendDeg, float explode)
{
    const float hw = C_POST_W / 2, hd = C_DEPTH / 2;
    const float outer = C_POST_X[2] + hw;
    const float rodZ = explode * 2.0f;   // rods slide forward in the exploded view

    // --- lower part: three posts, bottom sill, mitred joints
    concrete();
    for (int i = 0; i < 3; i++)
        drawBox(C_POST_X[i] - hw, C_POST_X[i] + hw, 0, C_LOWER_H, -hd, hd);
    drawBox(-outer, outer, 0, C_SILL_H, -hd, hd);
    for (int i = 0; i < 3; i++)
        drawBendJoint(C_POST_X[i] - hw, C_POST_X[i] + hw, bendDeg);

    // Vertical rods plus a horizontal tie rod along the bend line
    iron();
    for (int o = 0; o < 2; o++) {
        float a = C_POST_X[o] + hw, b = C_POST_X[o + 1] - hw;
        for (int k = 1; k <= C_RODS; k++) {
            float x = lerp(a, b, (float)k / (C_RODS + 1));
            drawRod(v3(x, C_SILL_H, rodZ), v3(x, C_LOWER_H, rodZ), ROD_R);
        }
        drawRod(v3(a, C_LOWER_H, rodZ), v3(b, C_LOWER_H, rodZ), ROD_R);
    }

    // --- upper part ("bowed head"): child transform of the lower part
    glPushMatrix();
    glTranslatef(0, C_LOWER_H, 0);
    glRotatef(bendDeg, 1, 0, 0);

    concrete();
    for (int i = 0; i < 3; i++) {
        float x = C_POST_X[i], f = C_FLARE[i];
        drawSlantedPost(x - hw, x + hw, x - hw + f, x + hw + f, 0, C_UPPER_L, -hd, hd);
    }

    // Rods fan out with the splayed posts and stop under the top beam
    iron();
    const float rodTop = C_UPPER_L - C_TOPBAR_H;
    const float s = rodTop / C_UPPER_L;
    for (int o = 0; o < 2; o++) {
        float a = C_POST_X[o] + hw, b = C_POST_X[o + 1] - hw;
        float at = a + C_FLARE[o] * s, bt = b + C_FLARE[o + 1] * s;
        for (int k = 1; k <= C_RODS; k++) {
            float t = (float)k / (C_RODS + 1);
            drawRod(v3(lerp(a, b, t), 0, rodZ), v3(lerp(at, bt, t), rodTop, rodZ), ROD_R);
        }
    }

    // Top beam across the head
    concrete();
    float lift = explode * 2.5f;
    float topOuter = outer + C_FLARE[2] + C_TOPBAR_LIP;
    drawBox(-topOuter, topOuter, rodTop + lift, C_UPPER_L + lift, -hd - 0.08f, hd + 0.08f);

    glPopMatrix();
}

// ------------------------------------------------------------ side frames --

struct SideFrame { float x, z, yaw, width, height; };

// Inner pair is taller; outer pair is shorter and turned further toward the
// centre, so the five frames stand on a shallow arc.
static const SideFrame SIDES[4] = {
    { -7.7f,  -0.9f,  14, 4.4f, 9.6f },
    {  7.7f,  -0.9f, -14, 4.4f, 9.6f },
    { -12.6f, -2.8f,  28, 4.0f, 7.6f },
    {  12.6f, -2.8f, -28, 4.0f, 7.6f }
};

static void drawSideFrame(float W, float H)
{
    const float pw = 0.65f, hd = 0.3f, topH = 0.7f, sillH = 0.5f, lip = 0.1f;
    const int rods = 4;
    float hw = W / 2;

    concrete();
    drawBox(-hw, -hw + pw, 0, H, -hd, hd);
    drawBox(hw - pw, hw, 0, H, -hd, hd);
    drawBox(-hw - lip, hw + lip, H - topH, H, -hd - 0.05f, hd + 0.05f);
    drawBox(-hw, hw, 0, sillH, -hd, hd);

    iron();
    float a = -hw + pw, b = hw - pw;
    for (int k = 1; k <= rods; k++) {
        float x = lerp(a, b, (float)k / (rods + 1));
        drawRod(v3(x, sillH, 0), v3(x, H - topH, 0), ROD_R);
    }
}

// ---------------------------------------------------------------- red sun --

static void sunColor(float k)   // k: 0 at the bottom of the disc, 1 at the top
{
    color(lerp(0.72f, 0.93f, k), lerp(0.11f, 0.10f, k), lerp(0.20f, 0.30f, k));
}

// The disc is mounted flush against the back faces of the three central
// posts and carried by a concrete column standing on the dais behind it.
// While rising it slides up behind the posts from inside the platform.
static void drawSun(float rise)
{
    const int N = 96;
    const float R = 5.2f, T = 0.25f;
    const float Z = -C_DEPTH / 2 - 0.005f - T / 2;          // disc centre plane
    float e = rise * rise * (3 - 2 * rise);                // smoothstep easing
    float cy = BASE_Y + 8.2f - (1 - e) * 15.0f;

    if (cy > BASE_Y) {
        concrete();
        drawBox(-0.35f, 0.35f, BASE_Y, cy, Z - T / 2 - 0.7f, Z - T / 2);
    }

    for (int side = 0; side < 2; side++) {
        float nz = side == 0 ? 1.0f : -1.0f;
        glNormal3f(0, 0, nz);
        glBegin(GL_TRIANGLE_FAN);
        sunColor(0.5f);
        glVertex3f(0, cy, Z + nz * T / 2);
        for (int i = 0; i <= N; i++) {
            float ang = (side == 0 ? 1 : -1) * 2 * PI * i / N;
            float x = R * cosf(ang), y = R * sinf(ang);
            sunColor((y / R + 1) / 2);
            glVertex3f(x, cy + y, Z + nz * T / 2);
        }
        glEnd();
    }

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= N; i++) {
        float ang = 2 * PI * i / N, c = cosf(ang), s = sinf(ang);
        sunColor((s + 1) / 2);
        glNormal3f(c, s, 0);
        glVertex3f(R * c, cy + R * s, Z + T / 2);
        glVertex3f(R * c, cy + R * s, Z - T / 2);
    }
    glEnd();
}

// --------------------------------------------------------------- assembly --

void drawMonument(const MinarState& s)
{
    glPushMatrix();
    glTranslatef(0, BASE_Y, 0);
    drawCentralFrame(s.bendAngle, s.explode);
    glPopMatrix();

    for (int i = 0; i < 4; i++) {
        const SideFrame& f = SIDES[i];
        float dir = f.x < 0 ? -1.0f : 1.0f;
        float push = s.explode * (i < 2 ? 3.5f : 6.5f);
        glPushMatrix();
        glTranslatef(f.x + dir * push, BASE_Y, f.z - s.explode * 1.5f);
        glRotatef(f.yaw, 0, 1, 0);
        drawSideFrame(f.width, f.height);
        glPopMatrix();
    }

    // A part-risen disc is still inside the platform, so it casts no shadow
    if (s.showSun && (!g_shadowPass || s.sunRise >= 1)) {
        glPushMatrix();
        glTranslatef(0, 0, -s.explode * 5.0f);
        drawSun(s.sunRise);
        glPopMatrix();
    }
}
