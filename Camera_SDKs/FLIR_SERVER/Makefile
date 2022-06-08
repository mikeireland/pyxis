################################################################################
# FLIR API Makefile
################################################################################

################################################################################
# Key paths and settings
################################################################################
CFLAGS += -std=c++17 -Wall
CXX = g++

OBJDIR  = build
SRCDIR  = src
INCDIR = include

SRCEXT = cpp
MKDIR = mkdir -p

PLATFORM = $(shell uname)
ifeq ($(PLATFORM),Darwin)
OS = mac
endif

OUTDIR = bin

################################################################################
# Dependencies
################################################################################
# Spinnaker deps
SPINNAKER_LIB = -L/opt/spinnaker/lib -lSpinnaker${D} ${SPIN_DEPS}

################################################################################
# Master inc/lib/obj/dep settings
################################################################################

INC = -I${INCDIR}
ifneq ($(OS),mac)
INC += -I/opt/spinnaker/include
LIB += -Wl,-Bdynamic ${SPINNAKER_LIB}
LIB += -lcfitsio
LIB += -lpthread
#LIB += -lfftw3
#LIB += -lzml
else
INC += -I/usr/local/include/spinnaker
LIB += ${SPINNAKER_LIB}

endif


################################################################################
# Rules/recipes
################################################################################
# Binaries

_OBJ =  main.o mainCam.o FLIRCamera.o globals.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))


all: main

main: $(OBJ)
	${CXX} -o ${OUTDIR}/$@ $^ ${LIB}


# Intermediate object files
${OBJDIR}/%.o : ${SRCDIR}/%.${SRCEXT}
	@${MKDIR} ${OBJDIR}
	${CXX} ${CFLAGS} ${INC} -c $< -o $@


# Clean up intermediate objects
clean_obj:
	rm -f ${OBJDIR}/*
	@echo "intermediate objects cleaned up!"

# Clean up everything.
clean: clean_obj
	rm -f ${OUTDIR}/*
	@echo "all cleaned up!"
