// pigeons.cpp - pigeons circling the Shaheed Minar.
//
// Each bird is a small hierarchical model: body -> wing (shoulder rotation)
// -> outer wing (elbow rotation), so one flap angle drives a two-joint wing.
// Birds follow closed parametric paths; their heading, climb and bank are
// derived from the path's direction and how fast it turns.

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <math.h>
#include "geometry.h"
#include "pigeons.h"

static const float PI = 3.14159265f;
static const float PIGEON_SCALE = 2.2f;   // enlarged so they read at monument scale

// ------------------------------------------------------------------ model --
// Local frame: forward +Z, up +Y, left wing on +X. Units are metres.

static void drawWing(float side, float flapDeg)
{
    glPushMatrix();
    glTranslatef(side * 0.05f, 0.03f, 0.02f);       // shoulder
    glRotatef(side * flapDeg, 0, 0, 1);

    // Inner wing (arm)
    glColor3f(0.62f, 0.65f, 0.72f);
    V3 inner[8] = {
        v3(0, -0.006f, 0.07f),            v3(0, -0.006f, -0.09f),
        v3(side * 0.2f, -0.005f, -0.08f), v3(side * 0.2f, -0.005f, 0.06f),
        v3(0, 0.006f, 0.07f),             v3(0, 0.006f, -0.09f),
        v3(side * 0.2f, 0.005f, -0.08f),  v3(side * 0.2f, 0.005f, 0.06f)
    };
    drawHex(inner);

    // The two dark bars typical of a rock pigeon
    glColor3f(0.16f, 0.16f, 0.2f);
    for (int b = 0; b < 2; b++) {
        float z0 = -0.03f - b * 0.035f;
        float x0 = side * 0.04f, x1 = side * 0.18f;
        drawBox(x0 < x1 ? x0 : x1, x0 < x1 ? x1 : x0, 0.005f, 0.009f, z0 - 0.014f, z0);
    }

    // Outer wing (hand) bends further at the elbow
    glTranslatef(side * 0.2f, 0, 0);
    glRotatef(side * flapDeg * 0.7f, 0, 0, 1);
    glColor3f(0.40f, 0.42f, 0.49f);
    V3 outer[8] = {
        v3(0, -0.005f, 0.06f),             v3(0, -0.005f, -0.08f),
        v3(side * 0.24f, -0.003f, -0.03f), v3(side * 0.24f, -0.003f, 0.02f),
        v3(0, 0.005f, 0.06f),              v3(0, 0.005f, -0.08f),
        v3(side * 0.24f, 0.003f, -0.03f),  v3(side * 0.24f, 0.003f, 0.02f)
    };
    drawHex(outer);
    glPopMatrix();
}

static void drawTailSegment(float z0, float z1, float hw0, float hw1, float y0, float y1)
{
    const float t = 0.006f;
    V3 v[8] = {
        v3(-hw0, y0 - t, z0), v3(hw0, y0 - t, z0), v3(hw1, y1 - t, z1), v3(-hw1, y1 - t, z1),
        v3(-hw0, y0 + t, z0), v3(hw0, y0 + t, z0), v3(hw1, y1 + t, z1), v3(-hw1, y1 + t, z1)
    };
    drawHex(v);
}

static void drawPigeon(float flapDeg)
{
    // Body, iridescent neck, head
    glColor3f(0.55f, 0.58f, 0.66f);
    drawEllipsoid(v3(0, 0, 0), 0.075f, 0.07f, 0.16f);
    glColor3f(0.36f, 0.50f, 0.46f);
    drawEllipsoid(v3(0, 0.035f, 0.10f), 0.062f, 0.066f, 0.07f);
    glColor3f(0.48f, 0.51f, 0.60f);
    drawEllipsoid(v3(0, 0.075f, 0.17f), 0.045f, 0.045f, 0.05f);

    // Beak and orange eyes
    glColor3f(0.25f, 0.22f, 0.22f);
    V3 beak[8] = {
        v3(-0.012f, 0.058f, 0.21f),  v3(0.012f, 0.058f, 0.21f),
        v3(0.012f, 0.078f, 0.21f),   v3(-0.012f, 0.078f, 0.21f),
        v3(-0.003f, 0.062f, 0.255f), v3(0.003f, 0.062f, 0.255f),
        v3(0.003f, 0.067f, 0.255f),  v3(-0.003f, 0.067f, 0.255f)
    };
    drawHex(beak);
    glColor3f(0.9f, 0.5f, 0.1f);
    drawEllipsoid(v3(0.034f, 0.085f, 0.19f), 0.009f, 0.009f, 0.009f, 8, 6);
    drawEllipsoid(v3(-0.034f, 0.085f, 0.19f), 0.009f, 0.009f, 0.009f, 8, 6);

    // Fanned tail with a dark terminal band
    glColor3f(0.50f, 0.53f, 0.60f);
    drawTailSegment(-0.12f, -0.26f, 0.04f, 0.068f, 0.0f, -0.016f);
    glColor3f(0.18f, 0.18f, 0.22f);
    drawTailSegment(-0.26f, -0.30f, 0.068f, 0.076f, -0.016f, -0.02f);

    drawWing(1, flapDeg);
    drawWing(-1, flapDeg);
}

// ---------------------------------------------------------------- flights --

enum PathShape { LOOP, FIGURE_EIGHT };

struct FlightPath {
    PathShape shape;
    float radius, height, speed;   // speed in radians of path parameter per second
    float wobble, bob;             // radial and vertical waviness
    int lobes;
};

// Paths stay clear of the frames: loops orbit outside them and the figure
// eight crosses in front of the central frame's bowed head.
static const FlightPath PATHS[3] = {
    { LOOP,         17, 13.0f,  0.32f, 3.0f, 2.0f, 3 },
    { LOOP,         21, 16.0f, -0.24f, 2.5f, 1.5f, 2 },
    { FIGURE_EIGHT, 15, 12.5f,  0.30f, 0.0f, 1.5f, 0 }
};

struct Pigeon {
    int path;
    float lag;          // how far behind the flock leader (path parameter)
    float side, up;     // formation offset in the bird's own frame
    float flapOffset;
};

static const Pigeon BIRDS[8] = {
    { 0, 0.00f,  0.0f,  0.0f, 0.0f },
    { 0, 0.07f,  0.9f,  0.5f, 1.3f },
    { 0, 0.11f, -0.8f, -0.4f, 2.1f },
    { 0, 0.17f,  0.3f,  0.9f, 0.6f },
    { 1, 0.00f,  0.0f,  0.0f, 3.0f },
    { 1, 0.06f, -1.0f,  0.4f, 4.2f },
    { 1, 0.10f,  0.8f, -0.5f, 5.1f },
    { 2, 0.00f,  0.0f,  0.0f, 2.7f }
};

static void pathPoint(const FlightPath& f, float th, float p[3])
{
    if (f.shape == LOOP) {
        float r = f.radius + f.wobble * sinf(f.lobes * th);
        p[0] = r * cosf(th);
        p[1] = f.height + f.bob * sinf(2 * th);
        p[2] = r * sinf(th);
    } else {
        p[0] = f.radius * sinf(th);
        p[1] = f.height + f.bob * cosf(th);
        p[2] = 9 + 6 * sinf(2 * th);   // >= 5 m in front of the head where it crosses
    }
}

static float headingDeg(const FlightPath& f, float th, float dir)
{
    float a[3], b[3];
    pathPoint(f, th, a);
    pathPoint(f, th + dir * 0.01f, b);
    return atan2f(b[0] - a[0], b[2] - a[2]) * 180 / PI;
}

void drawPigeons(float t)
{
    for (int i = 0; i < 8; i++) {
        const Pigeon& b = BIRDS[i];
        const FlightPath& f = PATHS[b.path];
        float dir = f.speed > 0 ? 1.0f : -1.0f;
        float th = f.speed * t - dir * b.lag;

        float p[3], q[3];
        pathPoint(f, th, p);
        pathPoint(f, th + dir * 0.01f, q);
        float dx = q[0] - p[0], dy = q[1] - p[1], dz = q[2] - p[2];
        float yaw = atan2f(dx, dz) * 180 / PI;
        float pitch = -atan2f(dy, sqrtf(dx * dx + dz * dz)) * 180 / PI;

        // Bank into the turn in proportion to the turn rate
        float dt = 0.2f;
        float dYaw = headingDeg(f, th + dir * fabsf(f.speed) * dt, dir) - yaw;
        if (dYaw > 180) dYaw -= 360;
        if (dYaw < -180) dYaw += 360;
        float roll = -0.6f * dYaw / dt;
        if (roll > 35) roll = 35;
        if (roll < -35) roll = -35;

        // Flap at ~4.5 Hz, with occasional glides on held-up wings
        float g = 0.5f + 0.5f * sinf(0.7f * t + b.flapOffset * 3);
        float k = g < 0.2f ? 0.0f : g > 0.6f ? 1.0f : (g - 0.2f) / 0.4f;
        float flap = 8 + (10 + 28 * k) * sinf(t * 2 * PI * 4.5f + b.flapOffset);

        glPushMatrix();
        glTranslatef(p[0], p[1], p[2]);
        glRotatef(yaw, 0, 1, 0);
        glTranslatef(b.side, b.up, 0);
        glRotatef(pitch, 1, 0, 0);
        glRotatef(roll, 0, 0, 1);
        glScalef(PIGEON_SCALE, PIGEON_SCALE, PIGEON_SCALE);
        drawPigeon(flap);
        glPopMatrix();
    }
}
