
#include "mesh.h"

void GLInit()
{
    float mat_ambient[]   = {0.3, 0.3, 0.3, 1.0};
    float mat_shininess[] = {40.0};
    float mat_specular[]  = {1.0, 1.0, 1.0, 0.0};
    float mat_diffuse[]   = {1.0, 1.0, 1.0, 1.0};

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // "-near" and "-far" define clipping planes
    glOrtho(-1.1, 1.1, -1.1, 1.1, -10000, 10000);

    //gluPerspective(45, 1.0, 1.0, 100);


    glMatrixMode(GL_MODELVIEW);

    static float light0_ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float light0_specular[] = {0.0, 0.0, 0.0, 0.0};
    static float light0_diffuse[] = {0.98, 0.73, 0.0, 0.0};
    static float light0_position[] = {-1000.0, 500.0, 0.0, 0.0};
    static float light1_diffuse[] = {0.66, 0.91, 0.0, 0.0};
    static float light1_position[] = {0.0, 0.0, 1000.0, 0.0};
    static float light2_diffuse[] = {0.0, 0.4, 0.8, 0.0};
    static float light2_position[] = {1000.0, 500.0, 0.0, 0.0};
    static float light_white_diffuse[] = {1.0, 1.0, 1.0, 0.0};
    static float light_white_position[] = {0.0, 00.0, 500.0, 0.0};

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT1, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
    glEnable(GL_LIGHT1);
    /*
    glLightfv(GL_LIGHT2, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_diffuse);
    glLightfv(GL_LIGHT2, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
    glEnable(GL_LIGHT2);
    */
    glLightfv(GL_LIGHT3, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, light_white_diffuse);
    glLightfv(GL_LIGHT3, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT3, GL_POSITION, light_white_position);

    glEnable(GL_LIGHTING);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse); 

    glEnable(GL_NORMALIZE);
    glFrontFace(GL_CCW);    // CW: clock wise if we render teapot. Otherwise, use "GL_CCW"
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
}

