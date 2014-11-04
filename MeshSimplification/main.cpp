
#include "mesh.h"
#include "simplification.h"
#include <windows.h> 


Mesh mesh;
Simplification simplification;

GLfloat startx, starty;
GLfloat model_angle1 = 0.0, model_angle2 = 0.0, scale = 1.0, eye[3] = { 0.0, 0.0, 10.0 };
GLfloat window_width = 800, window_height = 800;

int Key;
int toggle = 0;
bool left_click = 0, right_click = 0;

bool doEdgeCollapse = false, doVertexSplit = false, doLOD = false;
int  step = 0;


void mouse(int button, int state, int x, int y)
{
    if ( button == GLUT_LEFT_BUTTON ) {
        if ( state == GLUT_DOWN ) {
            left_click = true;
            startx   = x;
            starty   = y;
        }else if (state == GLUT_UP) {
            left_click = false;
        }
    }else{ // button == GLUT_RIGHT_BUTTON
        if ( state == GLUT_DOWN ) {  
            right_click = true;
            startx   = x;
            starty   = y;
        }else if (state == GLUT_UP) {
            right_click = false;
        }
    }

}

void motion( int x, int y )
{
    if ( left_click && !right_click ) {       // rotation
        model_angle1 += (x - startx);
        model_angle2 += (y - starty);
    }else if( !left_click && right_click ){   // translating
        eye[0] -= (x - startx) / (window_width *0.25);
        eye[1] += (y - starty) / (window_height*0.25);
    }else{ // if( left_click && right_click ) // scaling
        scale -= (y - starty) * 0.01;
    }

    startx = x;
    starty = y;

    glutPostRedisplay();
}


void keyboard( unsigned char c, int x, int y )
{
    switch(c){
    case 'q':
        exit(0);
    case 'c':
        doEdgeCollapse = true;  break;
    case 's':        
        doVertexSplit  = true;  break;
    case 'z':
        if(step < 200){
            step++; 
            doLOD = true;
        }
            break;
    case 'x':  
        if(step > 0){
            step--;
            doLOD = true;
        }
        break;
    case 't':
        if(toggle) toggle = 0;
        else       toggle = 1;
        break;
    }

    glutPostRedisplay();
}


void display()
{
    glClearColor( 1.0, 1.0, 1.0, 0.0 );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glScalef(scale, scale, scale);
    gluLookAt(eye[0], eye[1], 10.0,
              eye[0], eye[1],  0.0,
                 0.0,    1.0,  0.0 );
    glRotatef(model_angle1, 0, 1, 0);
    glRotatef(model_angle2, 1, 0, 0);

    if(doEdgeCollapse){
        simplification.EdgeCollapse();
        doEdgeCollapse = false;
    }

    if(doVertexSplit){
        simplification.VertexSplit();
        doVertexSplit = false;
    }

    if(doLOD){
        simplification.ControlLevelOfDetail(step);
        doLOD = false;
    }

    mesh.Display(toggle);

    glutSwapBuffers();
}


int main(int argc, char *argv[])
{
    if( argc != 2 || mesh.ConstructMeshDataStructure(argv[1]) == false ){
        cerr << "usage: meshSimplification.exe *.off\n";
        exit(0);
    }

    simplification.InitSimplification(&mesh);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowPosition(20, 20);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Mesh Simplification");
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    GLInit();
    glutMainLoop();

    return 1;
}

