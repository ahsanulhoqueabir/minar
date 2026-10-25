// geometry.cpp - small immediate-mode primitives shared by the models.

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <math.h>
#include "geometry.h"

static const float PI = 3.14159265f;

// Face normals are computed with Newell's method and turned to point away
// from the centre, and each face is emitted counter-clockwise from outside.
void drawHex(const V3 v[8])
{
    static const int F[6][4] = {
        {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4},
        {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}
    };
    V3 c = v3(0, 0, 0);
    for (int i = 0; i < 8; i++) { c.x += v[i].x; c.y += v[i].y; c.z += v[i].z; }
    c.x /= 8; c.y /= 8; c.z /= 8;

    glBegin(GL_QUADS);
    for (int f = 0; f < 6; f++) {
        V3 n = v3(0, 0, 0), fc = v3(0, 0, 0);
        for (int k = 0; k < 4; k++) {
            const V3& a = v[F[f][k]];
            const V3& b = v[F[f][(k + 1) % 4]];
            n.x += (a.y - b.y) * (a.z + b.z);
            n.y += (a.z - b.z) * (a.x + b.x);
            n.z += (a.x - b.x) * (a.y + b.y);
            fc.x += a.x / 4; fc.y += a.y / 4; fc.z += a.z / 4;
        }
        float len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len < 1e-9f) continue;   // degenerate face
        bool flip = n.x * (fc.x - c.x) + n.y * (fc.y - c.y) + n.z * (fc.z - c.z) < 0;
        if (flip) len = -len;
        glNormal3f(n.x / len, n.y / len, n.z / len);
        for (int k = 0; k < 4; k++) {
            const V3& q = v[F[f][flip ? 3 - k : k]];
            glVertex3f(q.x, q.y, q.z);
        }
    }
    glEnd();
}

void drawBox(float x0, float x1, float y0, float y1, float z0, float z1)
{
    V3 v[8] = {
        v3(x0, y0, z0), v3(x1, y0, z0), v3(x1, y0, z1), v3(x0, y0, z1),
        v3(x0, y1, z0), v3(x1, y1, z0), v3(x1, y1, z1), v3(x0, y1, z1)
    };
    drawHex(v);
}

void drawSlantedPost(float xb0, float xb1, float xt0, float xt1,
                     float y0, float y1, float z0, float z1)
{
    V3 v[8] = {
        v3(xb0, y0, z0), v3(xb1, y0, z0), v3(xb1, y0, z1), v3(xb0, y0, z1),
        v3(xt0, y1, z0), v3(xt1, y1, z0), v3(xt1, y1, z1), v3(xt0, y1, z1)
    };
    drawHex(v);
}

static V3 cross(V3 a, V3 b)
{
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

static V3 normalize(V3 a)
{
    float l = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    return l > 0 ? v3(a.x / l, a.y / l, a.z / l) : a;
}

void drawRod(V3 a, V3 b, float radius, int segments)
{
    // Orthonormal frame (u, w) perpendicular to the rod axis d
    V3 d = normalize(v3(b.x - a.x, b.y - a.y, b.z - a.z));
    V3 helper = fabsf(d.y) < 0.9f ? v3(0, 1, 0) : v3(1, 0, 0);
    V3 u = normalize(cross(d, helper));
    V3 w = cross(d, u);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++) {
        float t = 2 * PI * i / segments, c = cosf(t), s = sinf(t);
        V3 n = v3(u.x * c + w.x * s, u.y * c + w.y * s, u.z * c + w.z * s);
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(a.x + n.x * radius, a.y + n.y * radius, a.z + n.z * radius);
        glVertex3f(b.x + n.x * radius, b.y + n.y * radius, b.z + n.z * radius);
    }
    glEnd();

    for (int end = 0; end < 2; end++) {
        V3 p = end ? b : a;
        float sgn = end ? 1.0f : -1.0f;
        glNormal3f(d.x * sgn, d.y * sgn, d.z * sgn);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(p.x, p.y, p.z);
        for (int i = 0; i <= segments; i++) {
            float t = sgn * 2 * PI * i / segments, c = cosf(t), s = sinf(t);
            glVertex3f(p.x + (u.x * c + w.x * s) * radius,
                       p.y + (u.y * c + w.y * s) * radius,
                       p.z + (u.z * c + w.z * s) * radius);
        }
        glEnd();
    }
}

void drawEllipsoid(V3 c, float rx, float ry, float rz, int slices, int stacks)
{
    for (int i = 0; i < stacks; i++) {
        float p0 = PI * i / stacks - PI / 2, p1 = PI * (i + 1) / stacks - PI / 2;
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float t = 2 * PI * j / slices;
            for (int k = 1; k >= 0; k--) {
                float p = k ? p1 : p0;
                float x = cosf(p) * sinf(t), y = sinf(p), z = cosf(p) * cosf(t);
                // Normal of an ellipsoid is the unit-sphere normal divided by the radii
                V3 n = normalize(v3(x / rx, y / ry, z / rz));
                glNormal3f(n.x, n.y, n.z);
                glVertex3f(c.x + x * rx, c.y + y * ry, c.z + z * rz);
            }
        }
        glEnd();
    }
}
