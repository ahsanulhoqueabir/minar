// minar_model.h - geometry of the Central Shaheed Minar (Dhaka).
//
// Coordinate system: Y is up, the monument faces +Z, the ground is y = 0.
// Units are metres (approximate, proportioned from the reference images).

#ifndef MINAR_MODEL_H
#define MINAR_MODEL_H

// Height of the marble dais that all five frames stand on.
extern const float BASE_Y;

struct MinarState {
    float bendAngle;   // degrees the upper "head" of the central frame bows forward
    float sunRise;     // 0 = sun below the ground, 1 = fully risen behind the frames
    float explode;     // 0 = assembled, 1 = parts pulled apart (exploded view)
    bool  showSun;
};

void drawPlatform();                      // plinth, front steps and dais
void drawDaisTop();                       // only the dais top face (shadow receiver)
void drawMonument(const MinarState& s);   // central frame, four side frames, red sun

// While on, the model does not set colours (and skips a sun that is still
// rising), so it can be flattened into a shadow with the caller's colour.
void setShadowPass(bool on);

#endif
