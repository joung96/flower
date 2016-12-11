# INC_DIRS = -I/usr/openwin/include -I/usr/local/glut-3.7/include -I/usr/local/include
# LIB_DIRS = -L/usr/local/glut-3.7/lib/glut -L/usr/local/lib
# LDLIBS = -lglut -lGL -lGLU -lm -lXmu -lX11
# CFLAGS = -g  

# #LDLIBS= -lGL -lGLU -lm -lX11 
# #INC_DIRS = -I/usr/local/include -I/usr/X11R6/include -I/usr/local/include
# #LIB_DIRS = -L/usr/X11R6/lib -L/usr/local/lib

# OBJECTS= particles.o

# EXEC=particles

# all: $(EXEC)

# $(EXEC): $(OBJECTS)
# 	c++ $(OBJECTS) ${LIB_DIRS} ${LDLIBS} -R/usr/local/lib/glut -o $(EXEC)


# particles.o: particles.cc
# 	c++ ${CFLAGS} -c particles.cc  ${INC_DIRS}

# clean:
# 	-rm *.o


BASE = particle

all: $(BASE)

OS := $(shell uname -s)

ifeq ($(OS), Linux) # Science Center Linux Boxes
  CPPFLAGS = -I/home/l/i/lib175/usr/glew/include
  LDFLAGS += -L/home/l/i/lib175/usr/glew/lib -L/usr/X11R6/lib
  LIBS += -lGL -lGLU -lglut -lGLEW 
endif

ifeq ($(OS), Darwin) # Assume OS X
  CPPFLAGS += -D__MAC__ -stdlib=libstdc++
  LDFLAGS += -framework GLUT -framework OpenGL
endif

ifdef OPT 
  #turn on optimization
  CXXFLAGS += -O2
else 
  #turn on debugging
  CXXFLAGS += -g -w
endif

CXX = g++ 

OBJ = $(BASE).o 

$(BASE): $(OBJ)
	$(LINK.cpp) -o $@ $^ $(LIBS) 

clean:
	rm -f $(OBJ) $(BASE)
