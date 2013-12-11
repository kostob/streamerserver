#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Generic
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/InputFactory.o \
	${OBJECTDIR}/InputFfmpeg.o \
	${OBJECTDIR}/Network.o \
	${OBJECTDIR}/Streamer.o \
	${OBJECTDIR}/Threads.o \
	${OBJECTDIR}/Watermark.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/lib -L/usr/home/tobias/dev/GRAPES/src -Wl,-rpath,. -lpthread -lavcodec -lavdevice -lavfilter -lavformat -lavresample -lavutil -lgrapes

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/streamerserver

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/streamerserver: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/streamerserver ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/InputFactory.o: InputFactory.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DDEBUG -I/usr/home/tobias/dev/GRAPES/include -I/usr/local/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/InputFactory.o InputFactory.cpp

${OBJECTDIR}/InputFfmpeg.o: InputFfmpeg.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DDEBUG -I/usr/home/tobias/dev/GRAPES/include -I/usr/local/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/InputFfmpeg.o InputFfmpeg.cpp

${OBJECTDIR}/Network.o: Network.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DDEBUG -I/usr/home/tobias/dev/GRAPES/include -I/usr/local/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Network.o Network.cpp

${OBJECTDIR}/Streamer.o: Streamer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DDEBUG -I/usr/home/tobias/dev/GRAPES/include -I/usr/local/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Streamer.o Streamer.cpp

${OBJECTDIR}/Threads.o: Threads.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DDEBUG -I/usr/home/tobias/dev/GRAPES/include -I/usr/local/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Threads.o Threads.cpp

${OBJECTDIR}/Watermark.o: Watermark.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DDEBUG -I/usr/home/tobias/dev/GRAPES/include -I/usr/local/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Watermark.o Watermark.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DDEBUG -I/usr/home/tobias/dev/GRAPES/include -I/usr/local/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/streamerserver

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
