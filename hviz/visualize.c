#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "color_ramp.h"
#include "hviz.h"
#include "visualize.h"
#include "io.h"
#include "keys.h"

#define WITH_POINTS 0

extern Environment env;

void toggleSpinning();

const float ENV_RADIUS = 2;
float starColor[] = {0.6f, 0.52f, 0.0f, 1.0f};

#if 0
void onTimer(int value) {}
void onUpdate()  {}
void onUpdateInfo()  {}
void onReshape(int width, int height)  {}
void onKeyboard(unsigned char key, int x, int y) {}
void onClick(int button, int state, int x, int y)  {}
void on3DDrag(int x0, int y0, int z0) {}
void on2DDrag(int x0, int y0) {}
void toggleSpinning() {}
void viz_init() {}

#else

GLuint sphere;

void viz_init()
{
    if (env.fullscreen) glutFullScreen();
    //CGAssociateMouseAndMouseCursorPosition(false);
    //glutSetCursor(GLUT_CURSOR_NONE);
    glClearDepth(1.0f);

    //glutEstablishOverlay();
    //glutOverlayDisplayFunc(onUpdateInfo);
    //glutShowOverlay();

    glutDisplayFunc(onUpdate);
    glutReshapeFunc(onReshape);
    glutTimerFunc(100,onTimer,0);
    //glutTimerFunc(30,onTimer,0);
    //glutIdleFunc(onIdle);

    glutMotionFunc(on2DDrag);
    //glutMouseFunc(onClick);
    glutKeyboardFunc(onKeyboard);
    glutSpecialFunc(onKeyboardSpecial);

    glutSpaceballMotionFunc(on3DDrag);
    //glutSpaceballRotateFunc(on3DRotate);

    //glClearColor(1, 1, 1, 0);
#if 1
    switch (env.background)
    {
        case WHITE:
            glClearColor(1, 1, 1, 0);
            break;
        case BLACK:
            glClearColor(0, 0, 0, 0);
            break;
    }
#endif

    glPointSize(2.0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    //========================================================================
    // Lighting
    //========================================================================
    //glShadeModel(GL_FLAT);
#if 1
    glEnable(GL_DEPTH_TEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_BLEND);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glShadeModel(GL_SMOOTH);

    GLfloat global_light_ambient[] = {0.2,0.2,0.2,1.0};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_light_ambient);

    GLfloat light_ambient[]  = {.30, .30, .30, 1.0};
    GLfloat light_diffuse[]  = {1.0, 1.0, 1.0, 1.0};
    GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat mat_shininess[]= {30.0};                   // Define shininess of surface
    //glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);   // Set material properties
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, starColor);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); // Set material properties

    //glEnable(GL_CULL_FACE);
#endif
    //========================================================================
    //========================================================================

    sphere = glGenLists(1);
    glNewList(sphere, GL_COMPILE);
    glutSolidSphere(0.02, 10, 10);
    glEndList();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
}

void toggleSpinning()
{
    env.spinning = !env.spinning;
}

void on2DDrag(int x0, int y0)
{
    static float oldx = -10;
    static float oldz = -10;

    float x = 2.0*x0 / env.screenWidth  - 1.0;
    float z = 2.0*y0 / env.screenHeight - 1.0;

    if (oldx != -10)
        on3DDrag((int)((x - oldx)*1000), 0, (int)((z - oldz)*1000));

    oldx = x;
    oldz = z;

    glutPostRedisplay();
}

void on3DDrag(int x0, int y0, int z0)
{
    float x1, y1, z1;
    float xd, yd, zd;
    float x, y, z;
    float r;

    x1 =  x0 / 1000.0;
    y1 = -z0 / 1000.0;
    z1 = -y0 / 1000.0;


    //double r = env.eye.x*env.eye.x + env.eye.y*env.eye.y + env.eye.z*env.eye.z;
    //double dot = (x1*env.eye.x + y1*env.eye.y + z1*env.eye.z)/r;
    //xd = env.eye.x * dot;
    //yd = env.eye.y * dot;
    //zd = env.eye.z * dot;

    //fprintf(stderr, "%f %f %f\n", xd, yd, zd);

    xd = x1*cos(-env.eye.angle) - z1*sin(-env.eye.angle);
    yd = y1;
    zd = x1*sin(-env.eye.angle) + z1*cos(-env.eye.angle);

    if (1) //env.simStatus == SIM_STOPPED)
    {
        x = env.pointer.x + xd;
        y = env.pointer.y + yd;
        z = env.pointer.z - zd;

        r = x*x + y*y + z*z;

        if (r > ENV_RADIUS)
        {
            x /= r/ENV_RADIUS;
            y /= r/ENV_RADIUS;
            z /= r/ENV_RADIUS;
        }

        env.pointer.x = x; //fminf(fmaxf(-1, env.pointer.x + x0 / 1000.0), 1);
        env.pointer.y = y; //fminf(fmaxf(-1, env.pointer.y - z0 / 1000.0), 1);
        env.pointer.z = z; //fminf(fmaxf(-1, env.pointer.z + y0 / 1000.0), 1);
    }
    else
    {
        env.eye.x += xd;
        env.eye.y += yd;
        env.eye.z += zd;
    }

    glutPostRedisplay();
}

#if 0
void on3DRotate(int rx0, int ry0, int rz0)
{
    float x1 = rx0 / 180.0;
    float y1 = ry0 / 180.0;
    float z1 = rz0 / 180.0;

    xd = x1*cos(-env.eye.angle) - z1*sin(-env.eye.angle);
    yd = y1;
    zd = x1*sin(-env.eye.angle) + z1*cos(-env.eye.angle);

    env.eye.pitch   -= xd / 180.0;
    env.eye.heading -= yd / 180.0;
    env.eye.roll    += zd / 180.0;
    //fprintf(stderr, "rotate %i %i %i\n", rx0, ry0, rz0);
}
#endif

void onClick(int button, int state, int x, int y) 
{
    //fprintf(stderr, "click\n");
}

void onKeyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
#ifdef KEY_SPIN
        KEY_SPIN
            toggleSpinning();
            break;
#endif

        default: 
            break;
    }
}

void onKeyboardSpecial(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_RIGHT:
            if (! (env.t >= env.t_max) ) env.t++;
            break;
        case GLUT_KEY_LEFT:
            if (! (env.t == 0) ) env.t--;
            break;
        default:
            break;
    }

    glutPostRedisplay();
}

void onReshape(int width, int height) 
{
    env.screenWidth = width;
    env.screenHeight = height;
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);                // Select The Projection Matrix
    glLoadIdentity();                   // Reset The Projection Matrix

    // Calculate The Aspect Ratio Of The Window
    gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.01f, 101.0f); 

    glMatrixMode(GL_MODELVIEW);             // Select The Modelview Matrix
    glLoadIdentity();                   // Reset The Modelview Matrix
}

void onUpdateInfo() 
{
    glutStrokeCharacter(GLUT_STROKE_ROMAN, 'H');
}

void onUpdate() 
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();                   // Reset The Modelview Matrix

    gluLookAt(env.eye.x, env.eye.y, env.eye.z,  0.0, 0.0, 0.0,  0.0, 1.0, 0.); 

    float cx = env.pointer.x;
    float cy = env.pointer.y;
    float cz = env.pointer.z;

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);                 // Enable light source

    GLfloat light_pos[] = {env.eye.x, env.eye.y, env.eye.z, 1};

    GLfloat light_dir[] = {env.pointer.x-env.eye.x, env.pointer.y-env.eye.y, env.pointer.z-env.eye.z};
    //GLfloat light_dir[] = {env.eye.x-env.pointer.x, env.eye.y-env.pointer.y, env.eye.z-env.pointer.z};

    glLightfv(GL_LIGHT0, GL_POSITION,       light_pos);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir);

    halo_list_t *hl = &env.hl[env.t];

    unsigned int i;
    for (i=0; i < env.mt.n_halos; i++)
    {
        halo_t *halo = &hl->halo[ env.mt.h[env.t][i] ];

        glPushMatrix();
        glTranslatef(halo->Xc / env.max_x, 
                     halo->Yc / env.max_x, 
                     halo->Zc / env.max_x);

        float r = fabsf(halo->Mvir); 
        float g = fabsf(halo->Mvir); 
        float b = fabsf(halo->Mvir); 

        //color_ramp_wrbb(&r, &g, &b);
        //color_ramp_grey(&r, &g, &b);
        //color_ramp_hot2cold(&r, &g, &b);
        color_ramp_astro(&r, &g, &b);
        glColor3f(r, g, b);

        //glCallList(sphere);
        glutSolidSphere(0.02, 10, 10);
        glPopMatrix();
    }

    if (env.make_movie)
    {
        glFinish();
        glReadPixels(0,0,env.screenWidth, env.screenHeight, GL_RGB, GL_UNSIGNED_BYTE, env.frame_buffer);
        save_frame_buffer();
    }

    //glFlush();
    glutSwapBuffers();
}

void onTimer(int value)
{
    if (env.spinning)
    {
        glutTimerFunc(20, onTimer,0);

        env.eye.angle += 0.009;
        if (env.eye.angle > 2*M_PI) env.eye.angle -= 2*M_PI;

        env.eye.x = env.eye.ox*cos(env.eye.angle) - env.eye.oz*sin(env.eye.angle);
        env.eye.z = env.eye.ox*sin(env.eye.angle) + env.eye.oz*cos(env.eye.angle);
        env.eye.y = env.eye.oy;
        
        glutPostRedisplay();
    }
    //glutPostOverlayRedisplay();
}

#endif

