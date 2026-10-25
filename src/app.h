// app.h - window-system independent part of the Shaheed Minar demo.
//
// A front end (GLUT or plain Win32) creates the window and GL context and
// forwards events here. It must request a depth and a stencil buffer.

#ifndef APP_H
#define APP_H

enum SpecialKey {
    KEY_LEFT = 1, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_PAGE_UP, KEY_PAGE_DOWN
};

void appInit();
void appResize(int w, int h);
void appDisplay();                 // draws one frame; the front end swaps buffers
bool appUpdate(float dt);          // advances animations; true while anything moves
void appKey(unsigned char key);
void appSpecialKey(int key);       // one of SpecialKey
void appMouseButton(int button, bool down, int x, int y);   // 0 left, 1 middle, 2 right
void appMouseMotion(int x, int y);
void appMouseWheel(int dir);       // +1 away from the user, -1 toward
bool appWantsQuit();

// Implemented by the front end: draws ASCII text at the current raster position.
void platformDrawString(const char* s);

#endif
