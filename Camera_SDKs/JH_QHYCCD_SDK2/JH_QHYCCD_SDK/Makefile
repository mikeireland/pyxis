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

OUTDIR = .

################################################################################
# Dependencies
################################################################################
QHY_SDK = qhy_sdk_linux64_21.08.05


################################################################################
# Master inc/lib/obj/dep settings
################################################################################

INC = -Ilibs/
INC += -I${SRCINCDIR}


QHY_LIB  = /usr/local/lib/libqhyccd.so
LIB += -lusb-1.0 -pthread -Wl,${QHY_LIB} -Llib


SRCINC = $(wildcard ${SRCINCDIR}/*.${SRCEXT})

OBJ = $(addprefix ${OBJDIR}/,$(notdir $(SRCINC:.${SRCEXT}=.o)))

################################################################################
# Rules/recipes
################################################################################
# Binaries

binaries = testCam SingleFrameMode LiveFrameSample

all: ${binaries}

#Install QHYCCD
install:
	unzip ${QHY_SDK}.zip
	cd ${QHY_SDK}; sudo bash install.sh
	rm -rf ${QHY_SDK}

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

#Uninstall QHYCCD
uninstall:
	unzip ${QHY_SDK}.zip
	cd ${QHY_SDK}; sudo bash uninstall.sh
	rm -rf ${QHY_SDK}

# Clean up intermediate objects
clean_obj:
	rm -f ${OBJDIR}/*
	@echo "intermediate objects cleaned up!"

# Clean up everything.
clean: clean_obj
	rm -f ${OUTDIR}/*
	@echo "all cleaned up!"
