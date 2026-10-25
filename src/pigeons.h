// pigeons.h - a few pigeons circling the monument forever.

#ifndef PIGEONS_H
#define PIGEONS_H

// Draws every pigeon at animation time t (seconds). The flight paths are
// periodic, so increasing t loops the flight indefinitely.
void drawPigeons(float t);

#endif
