CC=g++

PKG_CONFIG_PATH ?= /usr/lib/x86_64-linux-gnu/pkgconfig
CFLAGS  = -std=c++17 -Wall -Wextra -ggdb -O1 -I../include -I../../../commander/include
LDFLAGS = -L../../../lib -L/usr/local/lib -lcommander -lm -lzmq -lboost_program_options -lfmt
EXEC    = TargetServer
OBJECTS = main.o

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
