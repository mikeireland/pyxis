CC=g++

PKG_CONFIG_PATH ?= /usr/lib/x86_64-linux-gnu/pkgconfig
CFLAGS  = -std=c++17 -Wall -Wextra -ggdb -O1 -I../include -I../../../commander/include -I../../libs/camera/include -I../../libs/imageproc/include -I/opt/spinnaker/include $(shell pkg-config --cflags opencv4)
LDFLAGS = -L../../../lib -L/opt/spinnaker/lib -L/usr/local/lib -lcommander -lm -lzmq -lboost_program_options -lfmt -lfftw3 -lSpinnaker -lcfitsio $(shell pkg-config --libs opencv4)
EXEC    = CoarseMetServer
OBJECTS = main.o CoarseMetrologyServer.o image.o centroid.o FLIRCamera.o FLIRcamServerFuncs.o globals.o runFLIRCam.o
vpath %.cpp .:../../libs/camera/src:../../libs/imageproc/src

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

# the makefile instructions

all: ../bin/$(EXEC)

../bin/$(EXEC): $(OBJECTS) $(IMG_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp 
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o *.so
	rm -rf *~
	rm ../bin/$(EXEC)

install:
	install -D ../bin/$(EXEC) $(PREFIX)/bin/
