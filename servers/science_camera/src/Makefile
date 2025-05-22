CC=g++

PKG_CONFIG_PATH ?= /usr/lib/x86_64-linux-gnu/pkgconfig
CFLAGS  = -std=c++17 -Wall -Wextra -ggdb -O1 -I../include -I../../../commander/include -I../../libs/camera/include -I../../libs -I../../libs/brent -I/opt/spinnaker/include $(shell pkg-config --cflags opencv4)
LDFLAGS = -L../../../lib -L/opt/spinnaker/lib -L/usr/local/lib -lcommander -lm -lzmq -lboost_program_options -lfmt -lfftw3 -lSpinnaker -lcfitsio -lqhyccd $(shell pkg-config --libs opencv4)
EXEC    = SciCamServer
OBJECTS = main.o runQHYCam.o QHYCamera.o globals.o QHYcamServerFuncs.o brent.o SciCamServer.o setup.o group_delay.o
vpath %.cpp .:../../libs/camera/src:../../libs/brent

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
	rm -f ../bin/$(EXEC)

install:
	install -D ../bin/$(EXEC) $(PREFIX)/bin/
