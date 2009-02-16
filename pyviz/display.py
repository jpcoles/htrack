from math import log10, sqrt, pi, cos, sin, radians, fabs
from color_ramp import color_ramp_wrbb, color_ramp_tipsy, color_ramp_astro, color_ramp_grey
from environment import env, WHITE, BLACK, MODE_NONE, MODE_TRACK, MODE_HALOTRACKS, MODE_HALOBODIES
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

FAC = .025
#FAC =.01
ANG = 10
##define ANG 1.3

#define Ex env.eye.x
#define Ey env.eye.y
#define Ez env.eye.z

#define Ux env.eye.ux
#define Uy env.eye.uy
#define Uz env.eye.uz

#define Tx env.eye.tx
#define Ty env.eye.ty
#define Tz env.eye.tz

def eye(*args):
    if args:
        env.eye.x,  env.eye.y,  env.eye.z,  \
        env.eye.ux, env.eye.uy, env.eye.uz, \
        env.eye.tx, env.eye.ty, env.eye.tz = args
    else:
        return env.eye.x,  env.eye.y,  env.eye.z,  \
               env.eye.ux, env.eye.uy, env.eye.uz, \
               env.eye.tx, env.eye.ty, env.eye.tz


ENV_RADIUS = 2
starColor = [0.6, 0.52, 0.0, 1.0]

sphere = []
display_list=0

TIMEOUT = 2

def display_start(argv):
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)

    glutInitWindowSize(env.screenWidth,env.screenHeight)

    new_argv = glutInit(argv)
    glutCreateWindow("Halo Visualizer")
    glutSetCursor(GLUT_CURSOR_NONE)
    viz_init()
    glutMainLoop()

def viz_init():
    if env.fullscreen: glutFullScreen()
    #CGAssociateMouseAndMouseCursorPosition(false)
    #glutSetCursor(GLUT_CURSOR_NONE)
    glClearDepth(1.0)

    #glutEstablishOverlay()
    #glutOverlayDisplayFunc(onUpdateInfo)
    #glutShowOverlay()

    glutDisplayFunc(onUpdate)
    glutReshapeFunc(onReshape)
    glutTimerFunc(TIMEOUT,onTimer,0)
    #glutTimerFunc(30,onTimer,0)
    #glutIdleFunc(onIdle)

    #glutMotionFunc(on2DDrag)
    glutPassiveMotionFunc(on2DDrag)
    glutMouseFunc(onClick)
    glutKeyboardFunc(onKeyboard)
    glutSpecialFunc(onKeyboardSpecial)

    glutSpaceballMotionFunc(on3DDrag)
    glutSpaceballRotateFunc(on3DRotate)

    #glClearColor(1, 1, 1, 0)

    if env.background == WHITE:
        glClearColor(1, 1, 1, 0)
    elif env.background == BLACK:
        glClearColor(0, 0, 0, 0)

    glPointSize(2.0)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1)

    #========================================================================
    # Lighting
    #========================================================================
    #glShadeModel(GL_FLAT)
#if 1
    glEnable(GL_DEPTH_TEST)
    glDepthFunc(GL_LEQUAL)
    #glDepthMask(GL_TRUE)
    #glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

#if 0
#   glEnable(GL_LIGHTING)
#   glEnable(GL_LIGHT0)
#   glShadeModel(GL_SMOOTH)

#   GLfloat global_light_ambient[] = {0.2,0.2,0.2,1.0}
#   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_light_ambient)

#   GLfloat light_ambient[]  = {.30, .30, .30, 1.0}
#   GLfloat light_diffuse[]  = {1.0, 1.0, 1.0, 1.0}
#   GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0}

#   glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient)
#   glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse)
#   glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular)

#   glEnable(GL_COLOR_MATERIAL)
#   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)

#   GLfloat mat_shininess[]= {30.0}                   # Define shininess of surface
#   #glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular)   // Set material properties
#   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, starColor)
#   glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess) # Set material properties

#   glEnable(GL_CULL_FACE)
#endif
#endif
    #========================================================================
    #========================================================================

    global display_list
    display_list = glGenLists(1)

    start = glGenLists(3)
    for i,r in [[0,4], [1,8], [2,16]]:
    #for i,r in [[0,8], [1,16], [2,32]]:
        sphere.append(start + i)
        glNewList(sphere[i], GL_COMPILE)
        glutWireSphere(1., r, r)
        glEndList()

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
    glPixelStorei(GL_PACK_ALIGNMENT, 1)

def toggleSpinning():
    env.spinning = not env.spinning

def pitch(ang):
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()
    t = radians(ang)

    Ix = (Uy * Tz  -  Uz * Ty)
    Iy = (Uz * Tx  -  Ux * Tz)
    Iz = (Ux * Ty  -  Uy * Tx)

    r = sqrt(Ix**2 + Iy**2 + Iz**2)

    Ix /= r
    Iy /= r
    Iz /= r

    UdotI = Ux*Tx + Uy*Ty + Uz*Tz

    ax = Ux * cos(t) + (Uy * Iz  -  Uz * Iy) * sin(t) + UdotI*Ux * (1-cos(t))
    ay = Uy * cos(t) + (Uz * Ix  -  Ux * Iz) * sin(t) + UdotI*Uy * (1-cos(t))
    az = Uz * cos(t) + (Ux * Iy  -  Uy * Ix) * sin(t) + UdotI*Uz * (1-cos(t))

    r = sqrt(ax**2 + ay**2 + az**2)

    Ux = ax/r
    Uy = ay/r
    Uz = az/r

    Tx = (Uy * -Iz  -  Uz * -Iy)
    Ty = (Uz * -Ix  -  Ux * -Iz)
    Tz = (Ux * -Iy  -  Uy * -Ix)

    r = sqrt(Tx**2 + Ty**2 + Tz**2)

    Tx /= r
    Ty /= r
    Tz /= r
    eye(Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz)

def roll(ang):
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()

    t = -radians(ang)
    
    r = sqrt(Tx**2 + Ty**2 + Tz**2)

    Tx /= r
    Ty /= r
    Tz /= r

    UdotT = Ux*Tx + Uy*Ty + Uz*Tz

    ax = Ux * cos(t) + (Uy * Tz  -  Uz * Ty) * sin(t) + UdotT*Ux * (1-cos(t))
    ay = Uy * cos(t) + (Uz * Tx  -  Ux * Tz) * sin(t) + UdotT*Uy * (1-cos(t))
    az = Uz * cos(t) + (Ux * Ty  -  Uy * Tx) * sin(t) + UdotT*Uz * (1-cos(t))

    r = sqrt(ax**2 + ay**2 + az**2)

    Ux = ax/r
    Uy = ay/r
    Uz = az/r

def yaw(ang):
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()
    t = radians(ang)
    
    #float r = sqrt(pow(Ux,2) + pow(Uy,2) + pow(Uz,2))

    #Ux /= r
    #Uy /= r
    #Uz /= r

    TdotU = Ux*Tx + Uy*Ty + Uz*Tz

    ax = Tx * cos(t) + (Ty * Uz  -  Tz * Uy) * sin(t) + TdotU*Tx * (1-cos(t))
    ay = Ty * cos(t) + (Tz * Ux  -  Tx * Uz) * sin(t) + TdotU*Ty * (1-cos(t))
    az = Tz * cos(t) + (Tx * Uy  -  Ty * Ux) * sin(t) + TdotU*Tz * (1-cos(t))

    r = sqrt(Tx**2 + Ty**2 + Tz**2)

    Tx = ax/r
    Ty = ay/r
    Tz = az/r

    eye(Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz)

def strafe(dist):
    dist *= -1

    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()

    i = (Uy * Tz  -  Uz * Ty)
    j = (Uz * Tx  -  Ux * Tz)
    k = (Ux * Ty  -  Uy * Tx)
    r = sqrt(i**2 + j**2 + k**2)

    Ex += (i/r) * dist
    Ey += (j/r) * dist
    Ez += (k/r) * dist

    eye(Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz)

def walk(dist):
    dist *= -1

    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()

    Ex += Tx * dist
    Ey += Ty * dist
    Ez += Tz * dist


    eye(Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz)

def fly(dist):
    dist *= -1
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()

    Ex += Ux * dist
    Ey += Uy * dist
    Ez += Uz * dist

    eye(Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz)

def on2DDrag(x0, y0):

    oldx = -10000
    oldy = -10000

    x = x0 - oldx
    y = y0 - oldy

    #if (oldx != -10000 && x) yaw((x > 0) * 2 - 1)
    #if (oldy != -10000 && y) pitch((y < 0) * 2 - 1)

    oldx = x0
    oldy = y0

    glutPostRedisplay()

def on3DDrag(x0, y0, z0):
#   if (x0) strafe((x0 > 0) * 2 - 1)
#   if (y0)   walk((y0 > 0) * 2 - 1)
#   if (z0)    fly((z0 > 0) * 2 - 1)
    if x0: strafe(x0/1000. * FAC)
    if y0:   walk(y0/1000. * FAC)
    if z0:    fly(z0/1000. * FAC)
    glutPostRedisplay()

def on3DRotate(rx0, ry0, rz0):
#   if (rx0) pitch((rx0 > 0) * 2 - 1)
#   if (ry0)   yaw((ry0 < 0) * 2 - 1)
#   if (rz0)  roll((rz0 > 0) * 2 - 1)
    if rx0: pitch(rx0/1000. * ANG)
    if ry0:   yaw(ry0/1000. * ANG)
    if rz0:  roll(rz0/1000. * ANG)
    glutPostRedisplay()

def ray_halo_intersect(cx, cy, cz, r):
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()
    vx = cx - Ex  # this is the vector from p to c
    vy = cy - Ey  # this is the vector from p to c
    vz = cz - Ez  # this is the vector from p to c
    
    vdotT = vx*Tx + vy*Ty + vz*Tz
    if vdotT < 0: # when the sphere is behind the origin p
        # note that this case may be dismissed if it is considered
        # that p is outside the sphere
        
        print >>stderr, "Behind!"
        return 0
    else: # center of sphere projects on the ray
        p = vdotT / (Ex*Ex + Ey*Ey + Ez*Ez)
        pcx = vx * p
        pcy = vy * p
        pcz = vz * p
        
        #fprintf(stderr, "X  %f, %f\n", (sqrt(pow(pcx,2) + pow(pcy,2) + pow(pcz,2))), r)
        return sqrt(pcx**2 + pcy**2 + pcz**2) < r

def onClick(button, state, x, y):
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()
    #env.mouse_down = (state == GLUT_DOWN) * (button+1)
    #glutTimerFunc(TIMEOUT, onTimer,0)

    print >>sys.stderr, "HERE!"

#   nearest = -1
#   for i in range(env.mt.n_halos):
#       ii = env.mt.h[i][env.t].v[0]

#       if ii == 0: continue

#       hl = env.hl[env.t]
#       halo = hl.halo[ ii ]

#       hx = halo.w.Xc
#       hy = halo.w.Yc
#       hz = halo.w.Zc

#       r = sqrt((Ex - hx)**2 + (Ey - hy)**2 + (Ez - hz)**2) 
#       hr = halo.w.Radius

#       if r <= hr: continue

#       if ray_halo_intersect(hx, hy, hz, r):
#           if nearest == -1 or r < nearest:
#               nearest = r
#               print >>sys.stderr, "Intersection with %i" % ii
#               env.selected_halo = i

#   if nearest == -1:
#       env.selected_halo = 0

def onKeyboard(key, x, y):
    if key == '\x7f': # backspace
        if env.t > 1: env.t -= 1
        env.dirty = 1
    elif key == ' ':
        if env.t < len(env.halos)-1: env.t += 1
        env.dirty = 1

    elif key in ['a', 'A']: roll(-1)
    elif key in ['d', 'D']: roll(1)
    elif key in ['w', 'W']: pitch(ANG)
    elif key in ['s', 'S']: pitch(-ANG)

    elif key in ['t', 'T']:
        env.mode ^= MODE_HALOTRACKS
        env.dirty = 1

    elif key in ['b', 'B']:
        env.mode ^= MODE_HALOBODIES
        env.dirty = 1
    elif key == '<':
        env.level += 1
        if env.level > env.max_level: env.level = env.max_level
        print >>sys.stderr, "level=%i\n" % env.level
        env.dirty = 1
    elif key == '>':
        env.level -= 1
        if env.level < 0: env.level = 0
        print >>sys.stderr, "level=%i\n" % env.level
        env.dirty = 1

    glutPostRedisplay()

def onKeyboardSpecial(key, x, y):
    if   key == GLUT_KEY_RIGHT: strafe(FAC)
    elif key == GLUT_KEY_LEFT:  strafe(-FAC)
    elif key == GLUT_KEY_UP:    walk(-FAC)
    elif key == GLUT_KEY_DOWN:  walk(FAC)
    glutPostRedisplay()

def onReshape(width, height):
    env.screenWidth  = width
    env.screenHeight = height
    glViewport(0, 0, width, height)

    glMatrixMode(GL_PROJECTION)                # Select The Projection Matrix
    glLoadIdentity()                   # Reset The Projection Matrix

    # Calculate The Aspect Ratio Of The Window
    gluPerspective(45.0,float(width)/float(height), 0.001, 101.0) 

    glMatrixMode(GL_MODELVIEW)             # Select The Modelview Matrix
    glLoadIdentity()                   # Reset The Modelview Matrix

def onUpdateInfo():
    glutStrokeCharacter(GLUT_STROKE_ROMAN, 'H')

def draw_halo(halo, ramp, i):
    hx = halo.X
    hy = halo.Y
    hz = halo.Z

    #r2 = (Ex - hx)**2 + (Ey - hy)**2 + (Ez - hz)**2

    #float R = halo.w.Mass
    #float R = halo.w.Rmax
    #float R = halo.w.Radius * 0.001
    R = halo.Radius
    #float R = halo.w.Radius * 0.001
    #float R = 0.001

    #hr2 = R**2

    #if (r2 <= hr2) continue


    #r = float(i) / len(env.mt)
    #g = float(i) / len(env.mt)
    #b = float(i) / len(env.mt)
    #a = fabs(1/(i+1)) 

#           m = [
#               halo.Eax, halo.Ebx, halo.Ecx, 0,
#               halo.Eay, halo.Eby, halo.Ecy, 0,
#               halo.Eaz, halo.Ebz, halo.Ecz, 0,
#                         0,           0,           0, 1
#           ]
    

    glTranslatef(hx, hy, hz)

    if False:
        glBegin(GL_LINES)
        glColor4f(1,0,0,1)
        glVertex3f(0,0,0)
        glVertex3f(halo.Eax*R*halo.a, halo.Eay*R*halo.a, halo.Eaz*R*halo.a)
#            glColor4f(0,1,0,1)
#            glVertex3f(0,0,0)
#            glVertex3f(halo.Eax*R*halo.a/2, halo.Eay*R*halo.a/2, halo.Eaz*R*halo.a/2)
        glColor4f(0,1,0,1)
        glVertex3f(0,0,0)
        glVertex3f(halo.Ebx*R*halo.b, halo.Eby*R*halo.b, halo.Ebz*R*halo.b)
        glColor4f(0,0,1,1)
        glVertex3f(0,0,0)
        glVertex3f(halo.Ecx*R*halo.c, halo.Ecy*R*halo.c, halo.Ecz*R*halo.c)
        glEnd()

    if False:
        glBegin(GL_LINES)
        glColor4f(1,0,0,1)
        glVertex3f(0,0,0)
        glVertex3f(halo.VX, halo.VY, halo.VZ)
        glEnd()

#endif

    glScalef(halo.a * R,
             halo.b * R,
             halo.c * R)

    alpha = 0.1 #exp(-pow((int)t-(int)env.t,2) / env.t_max) / 2
    #alpha = exp(-pow((int)t-(int)env.t,2) / env.t_max) / 2
    #fprintf(stderr, "%f\n", alpha)

    #color_ramp_tipsy((float)(n)/7., &r, &g, &b)
    if alpha > .01:
        #color_ramp_wrbb(&r, &g, &b)
        #color_ramp_grey(&r, &g, &b)
        #color_ramp_hot2cold(&r, &g, &b)
        #color_ramp_astro(&r, &g, &b)
        #color_ramp_tipsy((float)(i+1) / 10., &r, &g, &b)
        #r,g,b = color_ramp_tipsy(float(i) / nhalos)
        r,g,b = color_ramp_tipsy(ramp)
#if 0
#               if (t == env.t)
#                   glColor4f(1, 0, 0, 1)
#               else
#endif

        glColor4f(r, g, b, .8)
#        if i in [32, 414]:
#            glColor4f(r,g,b, 1)



        mi = log10(R)
        #print mi
        if mi > -3:
            if mi > -1: mi = 2
            elif mi > -2: mi = 1
            elif mi > -3: mi = 0
            glCallList(sphere[int(mi)])
        else:
            glPointSize(R * 1e4)
            glBegin(GL_POINTS)
            glVertex3f(0,0,0)
            glEnd()



def draw_halos_old():
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()

    active = 0
    for track in env.mt:
        if track and track[env.t]: active += 1

    nhalos = len(env.mt[1:])
    n=0
    for i,track in enumerate(env.mt[1:]):
        i += 1
        for tinfo in track[1:]:
            #print t
            #if (i != 1) continue
            t = tinfo['snap_id']
            if t != env.t: continue
            #if (i > 0 and t != env.t) continue

            #if (!(i == 0)
            #&& t != env.t) continue

            #int ii = env.mt.h[i][t].v[0]

#           ii = 0
#           if env.level >= track[t].len-1:
#               ii = track[t]['gid']
#           else:
#               continue
#               #ii = env.mt.h[i][t].v[ env.mt.h[i][t].len-1 - env.level ]

            ii = tinfo['gid']
            if not ii: continue


            n += 1

            glPushMatrix()
            draw_halo(env.halos[t][ii], float(i) / nhalos, i)
            glPopMatrix()

def draw_halos():
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()

    nhalos = len(env.halos[env.t])-1
    for i,halo in enumerate(env.halos[env.t][1:]):
        glPushMatrix()
        draw_halo(halo, float(i) / nhalos, i+1)
        glPopMatrix()

def draw_tracks():
    nhalos = len(env.mt[1:])

    for i,track in enumerate(env.mt[1:]):
#    for i in range(env.mt.n_halos):
##if 1
        #if i+1 not in [414,32]: continue
##              i != 1 
##           &&  i != 2
##          &&  i != 3
##          &&  i != 4
##          &&  i != 5
##           &&  i != 31
##           i != 412
##          &&  i >= 64
##          &&  i != 1738
##endif
#
        r,g,b = color_ramp_tipsy(float(i) / nhalos)
        #glColor4f(r, g, b, 1)
        glColor4f(1, 1, 1, .1)

        lx=0
        ly=0
        lz=0

        if len(track[1:]):
            glBegin(GL_LINE_STRIP)

            for tinfo in track[1:]:
                t  = tinfo['snap_id']
                ii = tinfo['gid']
                if not ii: continue

                halo = env.halos[t][ii]

                M = halo.Mass

                hx = halo.X
                hy = halo.Y
                hz = halo.Z

                glVertex3f(hx, hy, hz)

                lx = hx
                ly = hy
                lz = hz

            glEnd()

def onUpdate():
    Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz = eye()

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    glLoadIdentity()                   # Reset The Modelview Matrix

    if env.track_id and env.mt and (env.mode & MODE_TRACK):
        ii = env.mt[env.track_id][env.t]
        if ii and ii['gid']:
            ii = ii['gid']
            Tx = env.halos[env.t][ ii ].X - Ex
            Ty = env.halos[env.t][ ii ].Y - Ey
            Tz = env.halos[env.t][ ii ].Z - Ez

            r = sqrt(Tx**2 + Ty**2 + Tz**2)

            Tx /= r
            Ty /= r
            Tz /= r

            i  = Uy * -Tz  -  Uz * -Ty
            j  = Uz * -Tx  -  Ux * -Tz
            k  = Ux * -Ty  -  Uy * -Tx

            i2 = -Ty * k  -  -Tz * j
            j2 = -Tz * i  -  -Tx * k
            k2 = -Tx * j  -  -Ty * i

            r = sqrt(i2**2 + j2**2 + k2**2)

            Ux = i2/r
            Uy = j2/r
            Uz = k2/r
            eye(Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz)
    else:
        Tx = -Ex
        Ty = -Ey
        Tz = -Ez

        r = sqrt(Tx**2 + Ty**2 + Tz**2)

        Tx /= r
        Ty /= r
        Tz /= r

        i  = Uy * -Tz  -  Uz * -Ty
        j  = Uz * -Tx  -  Ux * -Tz
        k  = Ux * -Ty  -  Uy * -Tx

        i2 = -Ty * k  -  -Tz * j
        j2 = -Tz * i  -  -Tx * k
        k2 = -Tx * j  -  -Ty * i

        r = sqrt(i2**2 + j2**2 + k2**2)

        Ux = i2/r
        Uy = j2/r
        Uz = k2/r
        eye(Ex,Ey,Ez,Ux,Uy,Uz,Tx,Ty,Tz)


    gluLookAt(
        Ex,    Ey,    Ez,  
        Ex+Tx, Ey+Ty, Ez+Tz,
        Ux,    Uy,    Uz)

    cx = env.pointer.x
    cy = env.pointer.y
    cz = env.pointer.z

    #glEnable(GL_LIGHTING)
    #glEnable(GL_LIGHT0)                 // Enable light source

    #GLfloat light_pos[] = {env.eye.x, env.eye.y, env.eye.z, 1}

    #GLfloat light_dir[] = {env.eye.tx, env.eye.ty, env.eye.tz}
    #GLfloat light_dir[] = {env.eye.x-env.pointer.x, env.eye.y-env.pointer.y, env.eye.z-env.pointer.z}

    #glLightfv(GL_LIGHT0, GL_POSITION,       light_pos)
    #glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir)

    if env.dirty:
        env.dirty = 0
        glNewList(display_list, GL_COMPILE_AND_EXECUTE)
        if env.mode & MODE_HALOBODIES: draw_halos()
        if env.mode & MODE_HALOTRACKS: draw_tracks()
        glEndList()
    else:
        glCallList(display_list)

    if env.track_id or env.pointto_id:
        for halo in env.halos[env.t][1:]:
            if halo.gid == env.track_id or halo.gid in env.pointto_id:
                glColor4f(1,1,1,.5)
                glBegin(GL_LINES)
                glVertex3f(Ex-Ux, Ey-Uy, Ez-Uz)
                glVertex3f(halo.X, halo.Y, halo.Z)
                glEnd()

#if  0
#   glBegin(GL_LINES)
#   glColor4f(1,0,0,1)
#   glVertex3f(-.5, 0, 0)
#   glVertex3f( .5, 0, 0)
#   glColor4f(0,1,0,1)
#   glVertex3f(0, -.5, 0)
#   glVertex3f(0,  .5, 0)
#   glColor4f(0,0,1,1)
#   glVertex3f(0, 0, -.5)
#   glVertex3f(0, 0,  .5)

#   glColor4f(1,1,1,1)
#   glVertex3f(0, 0, 0)
#   glVertex3f(env.eye.x-env.eye.tx, env.eye.y-env.eye.ty, env.eye.z-env.eye.tz)
#   glEnd()

#   glColor4f(1,0,1,1)
#   glVertex3f(env.eye.x, env.eye.y, env.eye.z)
#   glVertex3f(env.eye.tx*10, env.eye.ty*10, env.eye.tz*10)
#   glEnd()
#endif


    #glFlush()
    glutSwapBuffers()

def onTimer(value):
#    if env.mouse_down:
#        walk(env.mouse_down-1 == GLUT_LEFT_BUTTON ? -1 : 1)
#        glutTimerFunc(TIMEOUT, onTimer,0)

#if 0
#       env.eye.angle += 0.009
#       if (env.eye.angle > 2*pi) env.eye.angle -= 2*pi

#       env.eye.x = env.eye.ox*cos(env.eye.angle) - env.eye.oz*sin(env.eye.angle)
#       env.eye.z = env.eye.ox*sin(env.eye.angle) + env.eye.oz*cos(env.eye.angle)
#       env.eye.y = env.eye.oy
#       
#       glutPostRedisplay()
#endif
    glutPostRedisplay()

#endif

