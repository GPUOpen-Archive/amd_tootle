PROJECT(TootleRaytracer)

SET(SOURCES
    TootleRaytracer.cpp
    JRT/JRTBoundingBox.cpp
    JRT/JRTCamera.cpp
    JRT/JRTCore.cpp
    JRT/JRTCoreUtils.cpp
    JRT/JRTH2KDTreeBuilder.cpp
    JRT/JRTHeuristicKDTreeBuilder.cpp
    JRT/JRTKDTree.cpp
    JRT/JRTKDTreeBuilder.cpp
    JRT/JRTMesh.cpp
    JRT/JRTOrthoCamera.cpp
    JRT/JRTPPMImage.cpp
    JRT/JRTTriangleIntersection.cpp
    Math/JMLFuncs.cpp)

SET(HEADERS
    TootleRaytracer.h
    JRT/JRTBoundingBox.h
    JRT/JRTCamera.h
    JRT/JRTCommon.h
    JRT/JRTCore.h
    JRT/JRTCoreUtils.h
    JRT/JRTH2KDTreeBuilder.h
    JRT/JRTHeuristicKDTreeBuilder.h
    JRT/JRTKDTree.h
    JRT/JRTKDTreeBuilder.h
    JRT/JRTMesh.h
    JRT/JRTOrthoCamera.h
    JRT/JRTPPMImage.h
    JRT/JRTTriangleIntersection.h
    Math/JML.h
    Math/JMLFuncs.h
    Math/JMLMatrix.h
    Math/JMLScalar.h
    Math/JMLSSEVec.h
    Math/JMLVec2.h
    Math/JMLVec3.h)

ADD_LIBRARY(RayTracer STATIC ${SOURCES} ${HEADERS})
TARGET_INCLUDE_DIRECTORIES(RayTracer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        JRT
        Math
    PRIVATE ..) # For TootlePCH.h
    
IF(UNIX)
    TARGET_COMPILE_DEFINITIONS(TootleLib
        PUBLIC _LINUX)
ENDIF()
