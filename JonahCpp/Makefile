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
SRCINCDIR = src/include

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
INC += -I${SRCINCDIR}
ifneq ($(OS),mac)
INC += -I/usr/include/spinnaker
LIB += -Wl,-Bdynamic ${SPINNAKER_LIB}
LIB += -lcfitsio
LIB += -lfftw3
LIB += -lzml
else
INC += -I/usr/local/include/spinnaker
LIB += ${SPINNAKER_LIB}

endif

SRCINC = $(wildcard ${SRCINCDIR}/*.${SRCEXT})

OBJ = $(addprefix ${OBJDIR}/,$(notdir $(SRCINC:.${SRCEXT}=.o)))

################################################################################
# Rules/recipes
################################################################################
# Binaries

binaries = testCam testFPS testFlats testLock testZaber

all: ${binaries}

${binaries}: % : ${OBJDIR}/%.o ${OBJ}
	${CXX} -o ${OUTDIR}/$@ $^ ${LIB}

# Intermediate script object files
${OBJDIR}/%.o : ${SRCDIR}/%.${SRCEXT}
	@${MKDIR} ${OBJDIR}
	${CXX} ${CFLAGS} ${INC} -c $< -o $@

# Intermediate module object files
${OBJ} : ${OBJDIR}/%.o : ${SRCINCDIR}/%.${SRCEXT}
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
