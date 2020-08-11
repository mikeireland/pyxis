################################################################################
# FLIR API Makefile
################################################################################

################################################################################
# Key paths and settings
################################################################################
CFLAGS += -std=c++11 -Wall
CXX = g++

OBJDIR  = build
SRCDIR  = src
SRCEXT = cpp
MODDIR = ${SRCDIR}/modules
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
SRC = $(shell find ${MODDIR} -type f -name "*.${SRCEXT}")
OBJ = $(SRC:${MODDIR}/%.${SRCEXT}=${OBJDIR}/%.o)

INC = -Ilib/
ifneq ($(OS),mac)
INC += -I/usr/include/spinnaker
LIB += -Wl,-Bdynamic ${SPINNAKER_LIB} 
LIB += -lcfitsio
else
INC += -I/usr/local/include/spinnaker
LIB += ${SPINNAKER_LIB}

endif

################################################################################
# Rules/recipes
################################################################################
# Binaries

all: runCam charDet

charDet: ${OBJDIR}/charDet.o ${OBJ}
	${CXX} -o ${OUTDIR}/charDet $^ ${LIB}

runCam: ${OBJDIR}/runCam.o ${OBJ}
	${CXX} -o ${OUTDIR}/runCam $^ ${LIB}

# Intermediate module object files
${OBJ}: ${OBJDIR}/%.o : ${MODDIR}/%.cpp
	@${MKDIR} ${OBJDIR}
	${CXX} ${CFLAGS} ${INC} -c $< -o $@

# Intermediate script object files
${OBJDIR}/%.o : ${SRCDIR}/%.cpp
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
