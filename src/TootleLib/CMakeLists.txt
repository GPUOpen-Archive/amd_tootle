PROJECT(TootleLib)

SET(SOURCES
    aligned_malloc.cpp
    clustering.cpp
    error.c
    feedback.cpp
    fit.cpp
    heap.c
    overdraw.cpp
    soup.cpp
    souptomesh.cpp
    Stripifier.cpp
    Timer.cpp
    tootlelib.cpp
    triorder.cpp)

SET(HEADERS
    aligned_malloc.h
    array.h
    bbox.h
    cloud.h
    clustering.h
    color.h
    error.h
    feedback.h
    fit.h
    heap.h
    matrix.h
    mesh.h
    option.h
    overdraw.h
    quaternion.h
    rcarray.h
    scalar.h
    soup.h
    souptomesh.h
    Stripifier.h
    Timer.h
    TootlePCH.h
    triorder.h
    vector.h
    viewpoints.h
    window.h)

ADD_LIBRARY(TootleLib STATIC ${SOURCES} ${HEADERS})
TARGET_INCLUDE_DIRECTORIES(TootleLib
    PUBLIC include)
TARGET_LINK_LIBRARIES(TootleLib RayTracer)

IF(UNIX)
    TARGET_COMPILE_DEFINITIONS(TootleLib
        PUBLIC _LINUX)
ENDIF()
