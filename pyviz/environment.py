MODE_NONE             = 0x00
MODE_TRACK            = 0x01
MODE_HALOTRACKS       = 0x02
MODE_HALOBODIES       = 0x04

BLACK = 0
WHITE = 255

class Coord:
    def __init__(self):
        self.x = 0
        self.y = 0
        self.z = 0

class Eye:
    def __init__(self):
        self.ox = 0
        self.oy = 0
        self.oz = 0 # Original position
        self.x  = 0 
        self.y  = 0 
        self.z  = 0

        self.tx = 0
        self.ty = 0
        self.tz = 0
        self.ux = 0
        self.uy = 0
        self.uz = 0

class World:
    def __init__(self):
        self.Xc             = 0
        self.Yc             = 0
        self.Zc             = 0
        self.VXc            = 0
        self.VYc            = 0
        self.VZc            = 0
        self.Mvir           = 0
        self.Vmax           = 0
        self.Rmax           = 0
        self.sigV           = 0
        self.Lambda         = 0
        self.Lx             = 0
        self.Ly             = 0
        self.Lz             = 0
        self.a              = 0
        self.Eax            = 0
        self.Eay            = 0
        self.Eaz            = 0
        self.b              = 0
        self.Ebx            = 0
        self.Eby            = 0
        self.Ebz            = 0
        self.c              = 0
        self.Ecx            = 0
        self.Ecy            = 0
        self.Ecz            = 0
        self.ovdens         = 0
        self.Redge          = 0
        self.nbins          = 0
        self.Ekin           = 0
        self.Epot           = 0
        self.mbp_offset     = 0
        self.com_offset     = 0
        self.r2             = 0
        self.lambdaE        = 0

class Halo(object):
    def __init__(self, row):
#       self.npart          = 0
#       self.nvpart         = 0
#       self.Xc             = 0
#       self.Yc             = 0
#       self.Zc             = 0
#       self.VXc            = 0
#       self.VYc            = 0
#       self.VZc            = 0
#       self.Mvir           = 0
#       self.Vmax           = 0
#       self.Rmax           = 0
#       self.sigV           = 0
#       self.Lambda         = 0
#       self.Lx             = 0
#       self.Ly             = 0
#       self.Lz             = 0
#       self.a              = 0
#       self.Eax            = 0
#       self.Eay            = 0
#       self.Eaz            = 0
#       self.b              = 0
#       self.Ebx            = 0
#       self.Eby            = 0
#       self.Ebz            = 0
#       self.c              = 0
#       self.Ecx            = 0
#       self.Ecy            = 0
#       self.Ecz            = 0
#       self.ovdens         = 0
#       self.Redge          = 0
#       self.nbins          = 0
#       self.Ekin           = 0
#       self.Epot           = 0
#       self.mbp_offset     = 0
#       self.com_offset     = 0
#       self.r2             = 0
#       self.lambdaE        = 0
#       w                   = World()
        self.row = row

    def __getattr__(self, name):
        return object.__getattribute__(self, "row")[name]

class Environment:
    def __init__(self):
        self.maxParticles       = 0
        self.fullscreen         = 0

        self.screenWidth        = 0
        self.screenHeight       = 0

        self.pointer            = Coord()
        self.eye                = Eye()

        self.t_max              = 0
        self.t                  = 0
        self.halos              = []
        self.mt                      = []

        self.max_x              = 0
        self.max_v              = 0
        self.max_m              = 0

        self.spinning           = 0

        self.movie_prefix       = 0
        self.make_movie         = 0
        self.print_frame        = 0
        self.current_movie_frame= 0

        self.background         = 0

        self.mode               = 0

        self.track_id           = 0

        self.mouse_down         = 0

        self.dirty              = 0

        self.selected_halo      = 0

        self.max_level          = 0
        self.level              = 0


env = Environment()

