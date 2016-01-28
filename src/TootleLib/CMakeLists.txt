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
    triorder.cpp

    RayTracer/TootleRaytracer.cpp
    RayTracer/JRT/JRTBoundingBox.cpp
    RayTracer/JRT/JRTCamera.cpp
    RayTracer/JRT/JRTCore.cpp
    RayTracer/JRT/JRTCoreUtils.cpp
    RayTracer/JRT/JRTH2KDTreeBuilder.cpp
    RayTracer/JRT/JRTHeuristicKDTreeBuilder.cpp
    RayTracer/JRT/JRTKDTree.cpp
    RayTracer/JRT/JRTKDTreeBuilder.cpp
    RayTracer/JRT/JRTMesh.cpp
    RayTracer/JRT/JRTOrthoCamera.cpp
    RayTracer/JRT/JRTPPMImage.cpp
    RayTracer/JRT/JRTTriangleIntersection.cpp
    RayTracer/Math/JMLFuncs.cpp)

SET(HEADERS
    aligned_malloc.h
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
    scalar.h
    soup.h
    souptomesh.h
    Stripifier.h
    Timer.h
    TootlePCH.h
    triorder.h
    vector.h
    viewpoints.h
    window.h

    include/tootlelib.h

    RayTracer/TootleRaytracer.h
    RayTracer/JRT/JRTBoundingBox.h
    RayTracer/JRT/JRTCamera.h
    RayTracer/JRT/JRTCommon.h
    RayTracer/JRT/JRTCore.h
    RayTracer/JRT/JRTCoreUtils.h
    RayTracer/JRT/JRTH2KDTreeBuilder.h
    RayTracer/JRT/JRTHeuristicKDTreeBuilder.h
    RayTracer/JRT/JRTKDTree.h
    RayTracer/JRT/JRTKDTreeBuilder.h
    RayTracer/JRT/JRTMesh.h
    RayTracer/JRT/JRTOrthoCamera.h
    RayTracer/JRT/JRTPPMImage.h
    RayTracer/JRT/JRTTriangleIntersection.h
    RayTracer/Math/JML.h
    RayTracer/Math/JMLFuncs.h
    RayTracer/Math/JMLMatrix.h
    RayTracer/Math/JMLScalar.h
    RayTracer/Math/JMLSSEVec.h
    RayTracer/Math/JMLVec2.h
    RayTracer/Math/JMLVec3.h)

ADD_LIBRARY(TootleLib STATIC ${SOURCES} ${HEADERS})
TARGET_INCLUDE_DIRECTORIES(TootleLib
    PUBLIC include
    PRIVATE
        RayTracer/JRT
        RayTracer/Math
        RayTracer
        ${CMAKE_CURRENT_SOURCE_DIR})

TARGET_COMPILE_DEFINITIONS(TootleLib
    PUBLIC _SOFTWARE_ONLY_VERSION)

IF(UNIX)
    TARGET_COMPILE_DEFINITIONS(TootleLib
        PUBLIC _LINUX)

    SET_PROPERTY(TARGET TootleLib PROPERTY CXX_STANDARD 11)
    SET_PROPERTY(TARGET TootleLib PROPERTY CXX_STANDARD_REQUIRED ON)
ENDIF()
