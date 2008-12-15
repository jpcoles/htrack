#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <GLUT/glut.h>

float starColor[] = {0.6f, 0.52f, 0.0f, 1.0f};

void onUpdate() 
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();                   // Reset The Modelview Matrix

    glTranslated(0, 0, -1);
    glutSolidTeapot(.3);
    glutSwapBuffers();
}

void onReshape(int width, int height) 
{
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);                // Select The Projection Matrix
    glLoadIdentity();                   // Reset The Projection Matrix

    // Calculate The Aspect Ratio Of The Window
    gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.01f, 101.0f); 

    glMatrixMode(GL_MODELVIEW);             // Select The Modelview Matrix
    glLoadIdentity();                   // Reset The Modelview Matrix

}

int main(int argc, char **argv)
{
    glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

    //glutInitWindowPosition(100,100);
    //glutInitWindowSize(env.screenWidth,env.screenHeight);

    glutInit(&argc,argv);

    glutCreateWindow("TestGL");

    glClearColor(0, 0, 0, 0);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
#if 1

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat global_light_ambient[] = {.20,.20,.20,1.0};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_light_ambient);

    glShadeModel(GL_SMOOTH);

    GLfloat light_ambient[]  = {.2, .2, .2, 1.0};
    GLfloat light_diffuse[]  = {.2, .8, .2, 1.0};
    GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, starColor);

    GLfloat light_pos[] = {0,0,1, 1};
    GLfloat light_dir[] = {0,1,-10};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir);
#endif

    glutDisplayFunc(onUpdate);
    glutReshapeFunc(onReshape);

    glutMainLoop();

    return 0;
}
