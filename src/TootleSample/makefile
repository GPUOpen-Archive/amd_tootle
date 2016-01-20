CC 		= g++ -D_SOFTWARE_ONLY_VERSION -D_LINUX 

OPTIMIZE        = -O3 -DNDEBUG

TOOTLELIB       = -lTootle

TOOTLETARGET    = TootleSample

TOP		= ../..

TARGET		= ${TOP}/bin/${TOOTLETARGET}

CFLAGS 		= ${OPTIMIZE} -I. -I${TOP}/src/TootleLib/include 

LDFLAGS 	= -L${TOP}/lib ${TOOTLELIB} -lm

OBJECTS		= Tootle.o ObjLoader.o MaterialSort.o Timer.o

CLEAN		= ${TARGET} ${OBJECTS} *.o

Tootle: ${OBJECTS}
	${CC} ${CFLAGS} -o ${TARGET} ${OBJECTS} ${LDFLAGS}

debug:
	${MAKE} "OPTIMIZE=-g" "TOOTLELIB=-lTootle_d" "TOOTLETARGET=Tootle_d"

.cpp.o:
	${CC} ${CFLAGS} -c $<

clean:
	/bin/rm -f ${CLEAN}
