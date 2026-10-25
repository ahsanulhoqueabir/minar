// geometry.h - small immediate-mode primitives shared by the models.

#ifndef GEOMETRY_H
#define GEOMETRY_H

struct V3 { float x, y, z; };

inline V3 v3(float x, float y, float z) { V3 v = { x, y, z }; return v; }

// Vertices 0-3 form one face and 4-7 the opposite face, with vertex i joined
// to vertex i+4. Any winding may be passed in; normals always point outward.
void drawHex(const V3 v[8]);

void drawBox(float x0, float x1, float y0, float y1, float z0, float z1);

// A post whose x-extent changes from [xb0,xb1] at y0 to [xt0,xt1] at y1.
void drawSlantedPost(float xb0, float xb1, float xt0, float xt1,
                     float y0, float y1, float z0, float z1);

// Round rod (capped cylinder) from a to b with smooth normals.
void drawRod(V3 a, V3 b, float radius, int segments = 14);

void drawEllipsoid(V3 c, float rx, float ry, float rz, int slices = 16, int stacks = 10);

#endif
