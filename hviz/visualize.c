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

#define FAC 0.001

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

#define TIMEOUT 2

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
    glutTimerFunc(TIMEOUT,onTimer,0);
    //glutTimerFunc(30,onTimer,0);
    //glutIdleFunc(onIdle);

    glutMotionFunc(on2DDrag);
    glutPassiveMotionFunc(on2DDrag);
    glutMouseFunc(onClick);
    glutKeyboardFunc(onKeyboard);
    glutSpecialFunc(onKeyboardSpecial);

    glutSpaceballMotionFunc(on3DDrag);
    glutSpaceballRotateFunc(on3DRotate);

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
    glDepthFunc(GL_LEQUAL);
    //glDepthMask(GL_TRUE);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if 0
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
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat mat_shininess[]= {30.0};                   // Define shininess of surface
    //glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);   // Set material properties
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, starColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess); // Set material properties

    glEnable(GL_CULL_FACE);
#endif
#endif
    //========================================================================
    //========================================================================

    sphere = glGenLists(1);
    glNewList(sphere, GL_COMPILE);
    glutWireSphere(1., 30, 30);
    //glutSolidSphere(1., 50, 50);
    glEndList();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
}

void toggleSpinning()
{
    env.spinning = !env.spinning;
}

#define Ux env.eye.ux
#define Uy env.eye.uy
#define Uz env.eye.uz

#define Tx env.eye.tx
#define Ty env.eye.ty
#define Tz env.eye.tz

#define ANG 1.3
void pitch(int dir)
{
    const float t = ANG * M_PI/180. * -dir;

    float Ix = (Uy * Tz  -  Uz * Ty);
    float Iy = (Uz * Tx  -  Ux * Tz);
    float Iz = (Ux * Ty  -  Uy * Tx);

    float r = sqrt(pow(Ix,2) + pow(Iy,2) + pow(Iz,2));

    Ix /= r;
    Iy /= r;
    Iz /= r;

    float UdotI = Ux*Tx + Uy*Ty + Uz*Tz;

    float ax = Ux * cos(t) + (Uy * Iz  -  Uz * Iy) * sin(t) + UdotI*Ux * (1-cos(t));
    float ay = Uy * cos(t) + (Uz * Ix  -  Ux * Iz) * sin(t) + UdotI*Uy * (1-cos(t));
    float az = Uz * cos(t) + (Ux * Iy  -  Uy * Ix) * sin(t) + UdotI*Uz * (1-cos(t));

    r = sqrt(pow(ax,2) + pow(ay,2) + pow(az,2));

    Ux = ax/r;
    Uy = ay/r;
    Uz = az/r;

    Tx = (Uy * -Iz  -  Uz * -Iy);
    Ty = (Uz * -Ix  -  Ux * -Iz);
    Tz = (Ux * -Iy  -  Uy * -Ix);

    r = sqrt(pow(Tx,2) + pow(Ty,2) + pow(Tz,2));

    Tx /= r;
    Ty /= r;
    Tz /= r;
}

void roll(int dir)
{
    const float t = ANG * M_PI/180. * -dir;
    
    float ax, ay, az;

    float r = sqrt(pow(Tx,2) + pow(Ty,2) + pow(Tz,2));

    Tx /= r;
    Ty /= r;
    Tz /= r;

    float UdotT = Ux*Tx + Uy*Ty + Uz*Tz;

    ax = Ux * cos(t) + (Uy * Tz  -  Uz * Ty) * sin(t) + UdotT*Ux * (1-cos(t));
    ay = Uy * cos(t) + (Uz * Tx  -  Ux * Tz) * sin(t) + UdotT*Uy * (1-cos(t));
    az = Uz * cos(t) + (Ux * Ty  -  Uy * Tx) * sin(t) + UdotT*Uz * (1-cos(t));

    r = sqrt(pow(ax,2) + pow(ay,2) + pow(az,2));

    Ux = ax/r;
    Uy = ay/r;
    Uz = az/r;
}

void yaw(int dir)
{
    const float t = ANG * M_PI/180. * dir;
    
    float ax, ay, az;

    //float r = sqrt(pow(Ux,2) + pow(Uy,2) + pow(Uz,2));

    //Ux /= r;
    //Uy /= r;
    //Uz /= r;

    float TdotU = Ux*Tx + Uy*Ty + Uz*Tz;

    ax = Tx * cos(t) + (Ty * Uz  -  Tz * Uy) * sin(t) + TdotU*Tx * (1-cos(t));
    ay = Ty * cos(t) + (Tz * Ux  -  Tx * Uz) * sin(t) + TdotU*Ty * (1-cos(t));
    az = Tz * cos(t) + (Tx * Uy  -  Ty * Ux) * sin(t) + TdotU*Tz * (1-cos(t));

    float r = sqrt(pow(Tx,2) + pow(Ty,2) + pow(Tz,2));

    Tx = ax/r;
    Ty = ay/r;
    Tz = az/r;
}

void strafe(int dir)
{
    const float dist = FAC * dir;

    float i = (Uy * Tz  -  Uz * Ty);
    float j = (Uz * Tx  -  Ux * Tz);
    float k = (Ux * Ty  -  Uy * Tx);
    float r = sqrt(pow(i,2) + pow(j,2) + pow(k,2));

    env.eye.x += (i/r) * dist;
    env.eye.y += (j/r) * dist;
    env.eye.z += (k/r) * dist;
}

void walk(int dir)
{
    const float dist = FAC * dir;

    env.eye.x += Tx * dist;
    env.eye.y += Ty * dist;
    env.eye.z += Tz * dist;
}

void fly(int dir)
{
    const float dist = FAC * -dir;

    env.eye.x += Ux * dist;
    env.eye.y += Uy * dist;
    env.eye.z += Uz * dist;
}

void on2DDrag(int x0, int y0)
{
    static int oldx = -10000;
    static int oldy = -10000;

    int x = x0 - oldx;
    int y = y0 - oldy;

    fprintf(stderr, "x=%i\n", x);
    if (oldx != -10000 && x) yaw((x > 0) * 2 - 1);
    if (oldy != -10000 && y) pitch((y < 0) * 2 - 1);

    oldx = x0;
    oldy = y0;

    glutPostRedisplay();
}

void on3DDrag(int x0, int y0, int z0)
{
    float x1, y1, z1;
    float xd, yd, zd;
    float x, y, z;
    float r;

    fprintf(stderr, "%i %i %i -- ", x0, y0, z0);
    fprintf(stderr, "%f %f %f\n", env.eye.x, env.eye.y, env.eye.z);

    if (x0) strafe((x0 > 0) * 2 - 1);
    if (y0)   walk((y0 > 0) * 2 - 1);
    if (z0)    fly((z0 > 0) * 2 - 1);

#if 0
    x1 =  x0 / 1000.0;
    y1 = -z0 / 1000.0;
    z1 = -y0 / 1000.0;


    //double r = env.eye.x*env.eye.x + env.eye.y*env.eye.y + env.eye.z*env.eye.z;
    //double dot = (x1*env.eye.x + y1*env.eye.y + z1*env.eye.z)/r;
    //xd = env.eye.x * dot;
    //yd = env.eye.y * dot;
    //zd = env.eye.z * dot;

    //fprintf(stderr, "%f %f %f\n", xd, yd, zd);

    xd = x1; //*cos(-env.eye.angle) - z1*sin(-env.eye.angle);
    yd = y1; //;
    zd = x1; //*sin(-env.eye.angle) + z1*cos(-env.eye.angle);

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
#endif

    glutPostRedisplay();
}

void on3DRotate(int rx0, int ry0, int rz0)
{
    fprintf(stderr, "%i %i %i -- ", rx0, ry0, rz0);
    if (rx0) pitch((rx0 > 0) * 2 - 1);
    if (ry0)   yaw((ry0 < 0) * 2 - 1);
    if (rz0)  roll((rz0 < 0) * 2 - 1);

    glutPostRedisplay();
}

void onClick(int button, int state, int x, int y) 
{
    env.mouse_down = (state == GLUT_DOWN) * (button+1);
    glutTimerFunc(TIMEOUT, onTimer,0);

    fprintf(stderr, "click %i %i\n", button, state);
}

void onKeyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 127: // backspace
            if (env.t > 0) env.t--;
            break;
        case ' ':
            if (env.t < env.t_max-1) env.t++;
            break;

        case 'a': case 'A':
            roll(1);
            break;
        case 'd': case 'D':
            roll(-1);
            break;

        case 'w': case 'W':
            pitch(-1);
            break;

        case 's': case 'S':
            pitch(1);
            break;

        default: 
            break;
    }

    glutPostRedisplay();
}

void onKeyboardSpecial(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_RIGHT: strafe(1);  break;
        case GLUT_KEY_LEFT:  strafe(-1); break;
        case GLUT_KEY_UP:    walk(-1);   break;
        case GLUT_KEY_DOWN:  walk(1);    break;
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
    gluPerspective(45.0f,(GLfloat)width/(GLfloat)height, 0.01f, 101.0f); 

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

#if 0
    if (env.mode == MODE_TRACK)
    {
        int ii = env.mt.h[env.track_id][env.t];
        if (ii != 0)
        {
            env.eye.tx = env.hl[env.t].halo[ ii ].Xc / env.max_x;
            env.eye.ty = env.hl[env.t].halo[ ii ].Yc / env.max_x;
            env.eye.tz = env.hl[env.t].halo[ ii ].Zc / env.max_x;

            float i, j, k;
            float i2, j2, k2;
            float r;

            i = (env.eye.uy * env.eye.tz  -  env.eye.uz * env.eye.ty);
            j = (env.eye.uz * env.eye.tx  -  env.eye.ux * env.eye.tz);
            k = (env.eye.ux * env.eye.ty  -  env.eye.uy * env.eye.tx);

            i2 = (env.eye.ty * k  -  env.eye.tz * j);
            j2 = (env.eye.tz * i  -  env.eye.tx * k);
            k2 = (env.eye.tx * j  -  env.eye.ty * i);

            r = sqrt(pow(i2,2) + pow(j2,2) + pow(k2,2));

            env.eye.ux = i2/r;
            env.eye.uy = j2/r;
            env.eye.uz = k2/r;
        }
    }
#endif

    gluLookAt(
        env.eye.x,  env.eye.y,  env.eye.z,  
        env.eye.x-env.eye.tx, env.eye.y-env.eye.ty, env.eye.z-env.eye.tz,
        env.eye.ux, env.eye.uy, env.eye.uz);

    float cx = env.pointer.x;
    float cy = env.pointer.y;
    float cz = env.pointer.z;

    //glEnable(GL_LIGHTING);
    //glEnable(GL_LIGHT0);                 // Enable light source

    GLfloat light_pos[] = {env.eye.x, env.eye.y, env.eye.z, 1};

    GLfloat light_dir[] = {env.eye.tx, env.eye.ty, env.eye.tz};
    //GLfloat light_dir[] = {env.eye.x-env.pointer.x, env.eye.y-env.pointer.y, env.eye.z-env.pointer.z};

    glLightfv(GL_LIGHT0, GL_POSITION,       light_pos);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir);

    halo_list_t *hl = &env.hl[env.t];

    unsigned int i;
    uint64_t active = 0;
    for (i=0; i < env.mt.n_halos; i++)
        if (env.mt.h[i][env.t] != 0) active++;

    uint64_t n=0;
    for (i=0; i < env.mt.n_halos; i++)
    {
        int ii = env.mt.h[i][env.t];

        if (ii == 0) continue;

        n++;

        halo_t *halo = &hl->halo[ ii ];

        glPushMatrix();

#if 0
        float r = fabsf(halo->Mvir); 
        float g = fabsf(halo->Mvir); 
        float b = fabsf(halo->Mvir); 
#else
#if 0
        float r = (float)n / active; 
        float g = (float)n / active;
        float b = (float)n / active;
        float a = fabsf(1/(i+1)); 
#else
        float r = (float)i / env.mt.n_halos; 
        float g = (float)i / env.mt.n_halos;
        float b = (float)i / env.mt.n_halos;
        float a = fabsf(1/(i+1)); 
#endif
#endif


        glTranslatef((halo->Xc / env.max_x) * 1, 
                     (halo->Yc / env.max_x) * 1, 
                     (halo->Zc / env.max_x) * 1);
        glScalef(halo->a * halo->Mvir / env.max_m,
                 halo->b * halo->Mvir / env.max_m,
                 halo->c * halo->Mvir / env.max_m);

#if 0
        glScalef(halo->Mvir * halo->a,
                 halo->Mvir * halo->b,
                 halo->Mvir * halo->c);
#endif
        //color_ramp_wrbb(&r, &g, &b);
        //color_ramp_grey(&r, &g, &b);
        //color_ramp_hot2cold(&r, &g, &b);
        //color_ramp_astro(&r, &g, &b);
        color_ramp_tipsy((float)i / env.mt.n_halos, &r, &g, &b);
        glColor4f(r, g, b, 1);
        //glColor4f(1, 0, 0, 1);
        //glColor4f(1, 1, 1, 1);
        glCallList(sphere);
        //glutSolidSphere(1., 50, 50);
        glPopMatrix();
    }

    if (env.make_movie)
    {
        glFinish();
        glReadPixels(0,0,env.screenWidth, env.screenHeight, GL_RGB, GL_UNSIGNED_BYTE, env.frame_buffer);
        save_frame_buffer();
    }

#if  1
    glBegin(GL_LINES);
    glColor4f(1,0,0,1);
    glVertex3f(-.5, 0, 0);
    glVertex3f( .5, 0, 0);
    glColor4f(0,1,0,1);
    glVertex3f(0, -.5, 0);
    glVertex3f(0,  .5, 0);
    glColor4f(0,0,1,1);
    glVertex3f(0, 0, -.5);
    glVertex3f(0, 0,  .5);

    glColor4f(1,1,1,1);
    glVertex3f(0, 0, 0);
    glVertex3f(env.eye.x-env.eye.tx, env.eye.y-env.eye.ty, env.eye.z-env.eye.tz);
    glEnd();

    glColor4f(1,0,1,1);
    glVertex3f(env.eye.x, env.eye.y, env.eye.z);
    glVertex3f(env.eye.tx*10, env.eye.ty*10, env.eye.tz*10);
    glEnd();
#endif


    //glFlush();
    glutSwapBuffers();
}

void onTimer(int value)
{
    if (env.mouse_down)
    {
        walk(env.mouse_down-1 == GLUT_LEFT_BUTTON ? -1 : 1);
        glutTimerFunc(TIMEOUT, onTimer,0);

#if 0
        env.eye.angle += 0.009;
        if (env.eye.angle > 2*M_PI) env.eye.angle -= 2*M_PI;

        env.eye.x = env.eye.ox*cos(env.eye.angle) - env.eye.oz*sin(env.eye.angle);
        env.eye.z = env.eye.ox*sin(env.eye.angle) + env.eye.oz*cos(env.eye.angle);
        env.eye.y = env.eye.oy;
        
        glutPostRedisplay();
#endif
    }
    glutPostRedisplay();
}

#endif

