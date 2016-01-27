PROJECT(TootleSample)

SET(SOURCES
    MaterialSort.cpp
    ObjLoader.cpp
    Timer.cpp
    Tootle.cpp)

SET(HEADERS
    ObjLoader.h
    option.h
    Timer.h)

ADD_EXECUTABLE(TootleSample ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(TootleSample TootleLib)
