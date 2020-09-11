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
SPINNAKER_LIB = -lSpinnaker${D} ${SPIN_DEPS}

################################################################################
# Master inc/lib/obj/dep settings
################################################################################

INC = -Ilib/
ifneq ($(OS),mac)
INC += -I/usr/include/spinnaker
LIB += -Wl,-Bdynamic ${SPINNAKER_LIB}
LIB += -lcfitsio
LIB += -lfftw3
else
INC += -I/usr/local/include/spinnaker
LIB += ${SPINNAKER_LIB}

endif

################################################################################
# Rules/recipes
################################################################################
# Binaries

all: runCam testFPS testFlats testLock

testLock: ${OBJDIR}/testLock.o ${OBJDIR}/FLIRCamera.o ${OBJDIR}/fringeLock.o
	${CXX} -o ${OUTDIR}/testLock $^ ${LIB}

runCam: ${OBJDIR}/runCam.o ${OBJDIR}/FLIRCamera.o
	${CXX} -o ${OUTDIR}/runCam $^ ${LIB}

testFPS: ${OBJDIR}/testFPS.o ${OBJDIR}/FLIRCamera.o
	${CXX} -o ${OUTDIR}/testFPS $^ ${LIB}

testFlats: ${OBJDIR}/testFlats.o ${OBJDIR}/FLIRCamera.o
	${CXX} -o ${OUTDIR}/testFlats $^ ${LIB}

# Intermediate script object files
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
