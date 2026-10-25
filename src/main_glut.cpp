// main_glut.cpp - GLUT / freeglut front end (the usual course setup).
//
// Build (MinGW + freeglut):
//   g++ -O2 -o shaheed_minar_glut.exe src/main_glut.cpp src/app.cpp
//       src/minar_model.cpp src/geometry.cpp src/pigeons.cpp
//       -lfreeglut -lopengl32 -lglu32
// Linux:  g++ ... -lglut -lGLU -lGL

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <stdlib.h>
#include "app.h"

static int lastTime;

void platformDrawString(const char* s)
{
    for (; *s; s++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s);
}

static void display()
{
    appDisplay();
    glutSwapBuffers();
}

static void keyboard(unsigned char key, int, int)
{
    appKey(key);
    if (appWantsQuit()) exit(0);
    glutPostRedisplay();
}

static void special(int key, int, int)
{
    switch (key) {
    case GLUT_KEY_LEFT:      appSpecialKey(KEY_LEFT); break;
    case GLUT_KEY_RIGHT:     appSpecialKey(KEY_RIGHT); break;
    case GLUT_KEY_UP:        appSpecialKey(KEY_UP); break;
    case GLUT_KEY_DOWN:      appSpecialKey(KEY_DOWN); break;
    case GLUT_KEY_PAGE_UP:   appSpecialKey(KEY_PAGE_UP); break;
    case GLUT_KEY_PAGE_DOWN: appSpecialKey(KEY_PAGE_DOWN); break;
    }
    glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
    if (button == 3 || button == 4) {          // freeglut reports the wheel as buttons 3/4
        if (state == GLUT_DOWN) appMouseWheel(button == 3 ? 1 : -1);
    } else {
        appMouseButton(button, state == GLUT_DOWN, x, y);
    }
    glutPostRedisplay();
}

static void motion(int x, int y)
{
    appMouseMotion(x, y);
    glutPostRedisplay();
}

static void timer(int)
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (appUpdate((now - lastTime) / 1000.0f)) glutPostRedisplay();
    lastTime = now;
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);
    glutInitWindowSize(1280, 800);
    glutCreateWindow("Shaheed Minar - OpenGL 3D Model");

    appInit();
    glutDisplayFunc(display);
    glutReshapeFunc(appResize);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
