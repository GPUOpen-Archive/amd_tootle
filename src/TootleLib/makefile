CC 		= g++ -fpermissive -msse -D_SOFTWARE_ONLY_VERSION -D_LINUX

TOOTLETARGET    = libTootle.a
OPTIMIZE        = -O3 -DNDEBUG

TOP		= ../..
TARGET		= ${TOP}/lib/${TOOTLETARGET}
RAYTRACER	= RayTracer
RTJRT		= ${RAYTRACER}/JRT
RTMATH		= ${RAYTRACER}/Math

CFLAGS 		= ${OPTIMIZE} -I. -Iinclude -I${RAYTRACER} -I${RTJRT} -I${RTMATH}

OBJECTS		= aligned_malloc.o clustering.o feedback.o fit.o overdraw.o soup.o souptomesh.o Stripifier.o Timer.o tootlelib.o triorder.o error.o heap.o ${RAYTRACER}/TootleRaytracer.o ${RTJRT}/JRTBoundingBox.o ${RTJRT}/JRTCamera.o ${RTJRT}/JRTCore.o ${RTJRT}/JRTCoreUtils.o ${RTJRT}/JRTH2KDTreeBuilder.o ${RTJRT}/JRTHeuristicKDTreeBuilder.o ${RTJRT}/JRTKDTree.o ${RTJRT}/JRTKDTreeBuilder.o ${RTJRT}/JRTMesh.o ${RTJRT}/JRTOrthoCamera.o ${RTJRT}/JRTPPMImage.o ${RTJRT}/JRTTriangleIntersection.o ${RTMATH}/JMLFuncs.o 

CLEAN		= ${OBJECTS} *.o

tootlelib: ${OBJECTS}
	ar -rs ${TARGET} *.o

debug:
	${MAKE} "OPTIMIZE=-g" "TOOTLETARGET=libTootle_d.a"

.cpp.o:
	${CC} ${CFLAGS} -c $<

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	/bin/rm -f ${CLEAN}
