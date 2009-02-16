CC			 =	g++
CXX			 =	g++
NVCC			 =	nvcc 
#NVCC			+=	-deviceemu 

PLATFORM		 =	Darwin
#PLATFORM		 =	Linux
#PLATFORM		 =	$(shell uname)
USERSRC			 =	$(HOME)

USE_GPU			 =	False
USE_NVIDIA 		 =	False

OBJS			 = 	main.o \
	 			display.o visualize.o \
		 		io.o \
		 		color_ramp.o
HDRS			 = 	color_ramp.h \
		 		display.h io.h keys.h \
		 		simulation.h \
		 		hviz.h \
		 		visualize.h 

ifeq ($(USE_GPU), True)
endif

CFLAGS  		+=	-Wall -g 
CFLAGS  		+=	-O0

CFLAGS 			+=	-Ijpeg-6b 
CFLAGS 			+=	-I../

ifeq ($(PLATFORM), Darwin)
LDFLAGS 		+= 	-framework OpenGL -framework GLUT -framework ApplicationServices
LIBS 		 	+= 	-lpthread 
endif

ifeq ($(PLATFORM), Linux)
LIBS 			+= 	-glut 
endif

CXXFLAGS		+=	$(CFLAGS)


hviz: $(OBJS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) ../set.o $(OBJS) jpeg-6b/libjpeg.a -o hviz $(LIBS)

all: hviz 

testgl: testgl.o
	$(CC) $(CFLAGS) $(LDFLAGS) testgl.o -o testgl $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

%.o: %.cu
	$(NVCC) $(CUFLAGS) -c $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@ 

$(SVR_OBJS): %.o: %.c
	$(CC) -DSERVER $(CFLAGS) -c $< -o $@

$(SVR_CXXOBJS): %.o: %.cpp
	$(CXX) -DSERVER $(CXXFLAGS) -c $< -o $@

#$(OBJS): $(INCL)

.PHONY : clean
clean:
	rm -f $(OBJS) $(SVR_OBJS) $(SVR_CXXOBJS) hviz

dist: clean
	mkdir -p dist
	cp README Makefile *.c *.cpp *h run_ss jonathan.par pkdgrav2m_pthread dist/
	cp *.jpg dist/
	cp -R jpeg-6b dist/

bindist: all
	mkdir -p hviz-v0.0
	cp hviz hviz-v0.0
	cp -R sample hviz-v0.0/
	tar cvvf hviz-v0.0.tar hviz-v0.0
	gzip hviz-v0.0.tar


