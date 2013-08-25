CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS "-Wall")

PROJECT(demuxer)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/../include core io)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/../lib)
ADD_EXECUTABLE(demuxer Main.cpp Mp4.cpp Rtp.cpp Mp4Player.cpp
	core/Log.cpp core/Thread.cpp core/Socket.cpp core/Event.cpp core/Common.cpp
	io/IO.cpp io/Disk.cpp)
TARGET_LINK_LIBRARIES(demuxer log4cplus pthread aio rt)
INSTALL(TARGETS demuxer RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/../bin)
