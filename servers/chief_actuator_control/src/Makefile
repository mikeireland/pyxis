CC=g++

CFLAGS  = -std=c++17 -Wall -Wextra -ggdb -O1 -I../include -I../../../commander/include
LDFLAGS = -L../../../lib -lcommander -lm -lzmq -lboost_program_options -lfmt -lfftw3  
EXEC    = ChiefAuxServer
OBJECTS = main.o chiefAuxGlobals.o chiefAuxServerFuncs.o Decode.o SerialPort.o

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

# the makefile instructions

all: ../bin/$(EXEC)

../bin/$(EXEC): $(OBJECTS)
	$(CC) -o ../bin/$@ $^ $(LDFLAGS)

%.o: %.cpp 
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o *.so
	rm -rf *~
	rm -f ../bin/$(EXEC)

install:
	install -D ../bin/$(EXEC) $(PREFIX)/bin/
