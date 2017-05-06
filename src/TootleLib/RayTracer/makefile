CC 		= g++ -O3 -fpermissive -msse -D_SOFTWARE_ONLY_VERSION -D_LINUX
#CC 		= g++ -g -Wall -fpermissive -msse -D_SOFTWARE_ONLY_VERSION -D_LINUX

TOP		= ../../..
TARGET		= ${TOP}/lib/libRayTracer.a
RTJRT		= JRT
RTMATH		= Math

CFLAGS 		= -I. -I../ -I../include -I${RTJRT} -I${RTMATH} 

LDFlags 	= -lm 

OBJECTS		= TootleRaytracer.o ${RTJRT}/JRTBoundingBox.o ${RTJRT}/JRTCamera.o ${RTJRT}/JRTCore.o ${RTJRT}/JRTCoreUtils.o ${RTJRT}/JRTH2KDTreeBuilder.o ${RTJRT}/JRTHeuristicKDTreeBuilder.o ${RTJRT}/JRTKDTree.o ${RTJRT}/JRTKDTreeBuilder.o ${RTJRT}/JRTMesh.o ${RTJRT}/JRTOrthoCamera.o ${RTJRT}/JRTPPMImage.o ${RTJRT}/JRTTriangleIntersection.o ${RTMATH}/JMLFuncs.o 

CLEAN		= ${TARGET} ${OBJECTS} *.o

RayTracer: ${OBJECTS}
	ar -rs ${TARGET} *.o

.cpp.o:
	${CC} ${CFLAGS} -c $<

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	/bin/rm -f ${CLEAN}
