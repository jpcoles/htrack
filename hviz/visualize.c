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

//#define FAC .025
#define FAC .001
#define ANG 0.6
//#define ANG 1.3

#define Ex env.eye.x
#define Ey env.eye.y
#define Ez env.eye.z

#define Ux env.eye.ux
#define Uy env.eye.uy
#define Uz env.eye.uz

#define Tx env.eye.tx
#define Ty env.eye.ty
#define Tz env.eye.tz


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

GLuint sphere[3];
GLuint display_list;

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

    //glutMotionFunc(on2DDrag);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    display_list = glGenLists(1);

    int i=0, r=8;
    GLuint start = glGenLists(3);
    for (i=0; i < 3; i++, r *= 2)
    {
        sphere[i] = start + i;
        glNewList(sphere[i], GL_COMPILE);
            glutWireSphere(1., r, r);
        glEndList();
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
}

void toggleSpinning()
{
    env.spinning = !env.spinning;
}

void pitch(int dir)
{
    const float t = ANG * M_PI/180. * dir;

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
    const float dist = FAC * -dir;

    float i = (Uy * Tz  -  Uz * Ty);
    float j = (Uz * Tx  -  Ux * Tz);
    float k = (Ux * Ty  -  Uy * Tx);
    float r = sqrt(pow(i,2) + pow(j,2) + pow(k,2));

    Ex += (i/r) * dist;
    Ey += (j/r) * dist;
    Ez += (k/r) * dist;
}

void walk(int dir)
{
    const float dist = FAC * -dir;

    Ex += Tx * dist;
    Ey += Ty * dist;
    Ez += Tz * dist;
}

void fly(int dir)
{
    const float dist = FAC * -dir;

    Ex += Ux * dist;
    Ey += Uy * dist;
    Ez += Uz * dist;
}

void on2DDrag(int x0, int y0)
{
    static int oldx = -10000;
    static int oldy = -10000;

    int x = x0 - oldx;
    int y = y0 - oldy;

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
    if (rx0) pitch((rx0 > 0) * 2 - 1);
    if (ry0)   yaw((ry0 < 0) * 2 - 1);
    if (rz0)  roll((rz0 > 0) * 2 - 1);

    glutPostRedisplay();
}

int ray_halo_intersect(float cx, float cy, float cz, float r)
{
    float vx = cx - Ex;  // this is the vector from p to c
    float vy = cy - Ey;  // this is the vector from p to c
    float vz = cz - Ez;  // this is the vector from p to c
    
    float vdotT = vx*Tx + vy*Ty + vz*Tz;
    if (vdotT < 0) // when the sphere is behind the origin p
    {
        // note that this case may be dismissed if it is considered
        // that p is outside the sphere
        
        fprintf(stderr, "Behind!\n");
        return 0;
    }        
    else // center of sphere projects on the ray
    {
        float p = vdotT / (Ex*Ex + Ey*Ey + Ez*Ez);
        float pcx = vx * p;
        float pcy = vy * p;
        float pcz = vz * p;
        
        //fprintf(stderr, "X  %f, %f\n", (sqrt(pow(pcx,2) + pow(pcy,2) + pow(pcz,2))), r);
        return (sqrt(pow(pcx,2) + pow(pcy,2) + pow(pcz,2)) < r);
    }
}

void onClick(int button, int state, int x, int y) 
{
    //env.mouse_down = (state == GLUT_DOWN) * (button+1);
    //glutTimerFunc(TIMEOUT, onTimer,0);

    fprintf(stderr, "HERE!\n");

    float nearest = -1;
    int i;
    for (i=0; i < env.mt.n_halos; i++)
    {
        int ii = env.mt.h[i][env.t];

        if (ii == 0) continue;

        halo_list_t *hl = &env.hl[env.t];
        halo_t *halo = &hl->halo[ ii ];

        float hx = halo->w.Xc;
        float hy = halo->w.Yc;
        float hz = halo->w.Zc;

        float r = 
              sqrt(
                  pow(Ex - hx, 2)
                + pow(Ey - hy, 2)
                + pow(Ez - hz, 2));

        float hr = halo->w.Rvir;

        if (r <= hr) continue;

        if (ray_halo_intersect(hx, hy, hz, r))
        {
            if (nearest == -1 || r < nearest)
            {
                nearest = r;
                fprintf(stderr, "Intersection with %i\n", ii);
                env.selected_halo = i;
            }
        }
    }

    if (nearest == -1)
        env.selected_halo = 0;

}

void onKeyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 127: // backspace
            if (env.t > 0) env.t--;
            env.dirty = 1;
            break;
        case ' ':
            if (env.t < env.t_max-1) env.t++;
            env.dirty = 1;
            break;

        case 'a': case 'A':
            roll(-1);
            break;
        case 'd': case 'D':
            roll(1);
            break;

        case 'w': case 'W':
            pitch(1);
            break;

        case 's': case 'S':
            pitch(-1);
            break;


        case 't': case 'T':
            env.mode ^= MODE_HALOTRACKS;
            env.dirty = 1;
            break;

        case 'b': case 'B':
            env.mode ^= MODE_HALOBODIES;
            env.dirty = 1;
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
    gluPerspective(45.0f,(GLfloat)width/(GLfloat)height, 0.001f, 101.0f); 

    glMatrixMode(GL_MODELVIEW);             // Select The Modelview Matrix
    glLoadIdentity();                   // Reset The Modelview Matrix
}

void onUpdateInfo() 
{
    glutStrokeCharacter(GLUT_STROKE_ROMAN, 'H');
}

void draw_halos()
{
    unsigned int i, j, t;
    uint64_t active = 0;
    for (i=0; i < env.mt.n_halos; i++)
        if (env.mt.h[i][env.t] != 0) active++;

    uint64_t n=0;
    for (i=0; i < env.mt.n_halos; i++)
    {
        for (t=0; t < env.t_max; t++)
        {
            //if (i != 1) continue;
            if (t != env.t) continue;
            //if (i > 0 && t != env.t) continue;

            //if (!(i == 0)
            //&& t != env.t) continue;

            int ii = env.mt.h[i][t];

            if (ii == 0) continue;

#if 0
            if (1
//              i != 1 
            &&  i != 2
            &&  i != 3
//          &&  i != 4
//          &&  i != 5
//          &&  i != 1738
//          &&  i != 7
            ) continue;
#endif

            n++;

            halo_list_t *hl = &env.hl[t];
            //fprintf(stderr, "%i\n", env.t_max);
            //fprintf(stderr, "%i/%ld %i/%i  %i/%ld\n", i, (long int)env.mt.n_halos, t, env.t_max, ii, (long int)hl->n_halos);
            assert(ii <= hl->n_halos);
            halo_t *halo = &hl->halo[ ii ];

            float hx = halo->w.Xc;
            float hy = halo->w.Yc;
            float hz = halo->w.Zc;

            float r2 = 
                  pow(Ex - hx, 2)
                + pow(Ey - hy, 2)
                + pow(Ez - hz, 2);

            //float R = halo->w.Mvir;
            //float R = halo->w.Rmax;
            //float R = halo->w.Rvir * 0.001;
            float R = halo->w.Rvir;
            //float R = halo->w.Rvir * 0.001;
            //float R = 0.001;

            float hr2 = pow(R, 2);

            //if (r2 <= hr2) continue;

            glPushMatrix();

            float r = (float)i / env.mt.n_halos; 
            float g = (float)i / env.mt.n_halos;
            float b = (float)i / env.mt.n_halos;
            float a = fabsf(1/(i+1)); 

            float m[16] = {
                halo->w.Eax, halo->w.Ebx, halo->w.Ecx, 0,
                halo->w.Eay, halo->w.Eby, halo->w.Ecy, 0,
                halo->w.Eaz, halo->w.Ebz, halo->w.Ecz, 0,
                          0,           0,           0, 1
            };
            
            //glLoadMatrixf(m);

            //glMultMatrixf(m);
#if 1
            glTranslatef(halo->w.Xc * 1, 
                         halo->w.Yc * 1, 
                         halo->w.Zc * 1);
#endif

#if 0
            glBegin(GL_LINES);
                glColor4f(1,0,0,1);
                glVertex3f(0,0,0);
                glVertex3f(halo->Eax*R*halo->w.a, halo->Eay*R*halo->w.a, halo->Eaz*R*halo->w.a);
                glColor4f(0,1,0,1);
                glVertex3f(0,0,0);
                glVertex3f(halo->Eax*R*halo->w.a/2, halo->Eay*R*halo->w.a/2, halo->Eaz*R*halo->w.a/2);
#if 1
                glColor4f(0,1,0,1);
                glVertex3f(0,0,0);
                glVertex3f(halo->Ebx*R*halo->w.b, halo->Eby*R*halo->w.b, halo->Ebz*R*halo->w.b);
                glColor4f(0,0,1,1);
                glVertex3f(0,0,0);
                glVertex3f(halo->Ecx*R*halo->w.c, halo->Ecy*R*halo->w.c, halo->Ecz*R*halo->w.c);
#endif
            glEnd();
#endif

#if 1
            glScalef(halo->a * R,
                     halo->b * R,
                     halo->c * R);
#endif

            float alpha = 0.1; //exp(-pow((int)t-(int)env.t,2) / env.t_max) / 2;
            //float alpha = exp(-pow((int)t-(int)env.t,2) / env.t_max) / 2;
            //fprintf(stderr, "%f\n", alpha);

            //color_ramp_tipsy((float)(n)/7., &r, &g, &b);
            if (alpha > .01)
            {
                //color_ramp_wrbb(&r, &g, &b);
                //color_ramp_grey(&r, &g, &b);
                //color_ramp_hot2cold(&r, &g, &b);
                //color_ramp_astro(&r, &g, &b);
                //color_ramp_tipsy((float)(i+1) / 10., &r, &g, &b);
                color_ramp_tipsy((float)i / env.mt.n_halos, &r, &g, &b);
#if 0
                if (t == env.t)
                    glColor4f(1, 0, 0, 1);
                else
#endif

                if (env.selected_halo == i)
                    glColor4f(0, 1, 0, 1);
                else
                    glColor4f(r, g, b, alpha);

                glColor4f(r, g, b, alpha);
                if (i == 31 || i == 413)
                    glColor4f(r,g,b, 1);


                int mi = 3 - log10(R);
                if (mi > 2) mi = 2;
                glCallList(sphere[mi]);
            }
            glPopMatrix();

#if 0
            glDisable(GL_LIGHTING);
            glPushMatrix();
            glColor4f(1,0,0,1);
            glTranslatef(halo->w.Xc * 1,
                         halo->w.Yc * 1,
                         halo->w.Zc * 1);
            glScalef(10,10,10);
            glutStrokeCharacter(GLUT_STROKE_ROMAN, '0'+ii);
            glPopMatrix();
#endif

        }
    }
}

void draw_tracks()
{
    float r,g,b;
    unsigned int i;
    uint64_t t=0;
    //for (i=0; i < 10; i++)
    for (i=0; i < env.mt.n_halos; i++)
    {
#if 1
            if (1
//              i != 1 
//          &&  i != 2
//          &&  i != 3
//          &&  i != 4
//          &&  i != 5
            &&  i != 31
            &&  i != 413
            &&  i >= 64
//          &&  i != 1738
            ) continue;
#endif

        color_ramp_tipsy((float)i / env.mt.n_halos, &r, &g, &b);
        glColor4f(r, g, b, 1);
        //glColor4f(1, 1, 1, 1);

        float lx=0, ly=0, lz=0;

        glBegin(GL_LINE_STRIP);

        int first=1;
        for (t=0; t < env.t_max; t++)
        {
            //if (! ( 3 <= env.t_max-t&&env.t_max-t <= 4) ) continue;

            //glColor4f(0, 1, 1, 1.0);
            //if (i == 31)
                //glColor4f(0, 1, 0, 1.0);
            int ii = env.mt.h[i][t];

            if (ii == 0) continue;

            halo_t *halo = &env.hl[t].halo[ ii ];

            float M = halo->w.Mvir;
            //if (M < 0.01) continue;

            float hx = halo->w.Xc;
            float hy = halo->w.Yc;
            float hz = halo->w.Zc;

            //if (halo->npart < 30)
                //continue;

            //if (!first)
            {
                //glVertex3f(lx, ly, lz);
                glVertex3f(hx, hy, hz);
            }

            lx = hx;
            ly = hy;
            lz = hz;
        }

        glEnd();
    }
}

void onUpdate() 
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();                   // Reset The Modelview Matrix

#if 1
    if (env.mode & MODE_TRACK)
    {
        int ii = env.mt.h[env.track_id][env.t];
        if (ii != 0)
        {
            float i, j, k;
            float i2, j2, k2;
            float r;

            Tx = env.hl[env.t].halo[ ii ].w.Xc - Ex;
            Ty = env.hl[env.t].halo[ ii ].w.Yc - Ey;
            Tz = env.hl[env.t].halo[ ii ].w.Zc - Ez;

            r = sqrt(pow(Tx,2) + pow(Ty,2) + pow(Tz,2));

            Tx /= r;
            Ty /= r;
            Tz /= r;

            i = (Uy * -Tz  -  Uz * -Ty);
            j = (Uz * -Tx  -  Ux * -Tz);
            k = (Ux * -Ty  -  Uy * -Tx);

            i2 = (-Ty * k  -  -Tz * j);
            j2 = (-Tz * i  -  -Tx * k);
            k2 = (-Tx * j  -  -Ty * i);

            r = sqrt(pow(i2,2) + pow(j2,2) + pow(k2,2));

            Ux = i2/r;
            Uy = j2/r;
            Uz = k2/r;
#if 0
        gluLookAt(
            env.eye.x,  env.eye.y,  env.eye.z,  
            env.hl[env.t].halo[ ii ].w.Xc,
            env.hl[env.t].halo[ ii ].w.Yc,
            env.hl[env.t].halo[ ii ].w.Zc,
            env.eye.ux, env.eye.uy, env.eye.uz);
#endif
        }
    }
#endif

    gluLookAt(
        Ex,    Ey,    Ez,  
        Ex+Tx, Ey+Ty, Ez+Tz,
        Ux,    Uy,    Uz);



    float cx = env.pointer.x;
    float cy = env.pointer.y;
    float cz = env.pointer.z;

    //glEnable(GL_LIGHTING);
    //glEnable(GL_LIGHT0);                 // Enable light source

    //GLfloat light_pos[] = {env.eye.x, env.eye.y, env.eye.z, 1};

    //GLfloat light_dir[] = {env.eye.tx, env.eye.ty, env.eye.tz};
    //GLfloat light_dir[] = {env.eye.x-env.pointer.x, env.eye.y-env.pointer.y, env.eye.z-env.pointer.z};

    //glLightfv(GL_LIGHT0, GL_POSITION,       light_pos);
    //glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir);

    if (env.dirty)
    {
        env.dirty = 0;
        glNewList(display_list, GL_COMPILE_AND_EXECUTE);
        if (env.mode & MODE_HALOBODIES) draw_halos();
        if (env.mode & MODE_HALOTRACKS) draw_tracks();
        glEndList();
    }
    else
    {
        glCallList(display_list);
    }

    if (env.make_movie)
    {
        glFinish();
        glReadPixels(0,0,env.screenWidth, env.screenHeight, GL_RGB, GL_UNSIGNED_BYTE, env.frame_buffer);
        save_frame_buffer();
    }

#if  0
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

