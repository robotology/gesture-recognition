# Copyright: (C) 2011 EFAA Consortium
# Authors: Ilaria Gori
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

set(PROJECTNAME gestureRecognition)
project(${PROJECTNAME})

find_package(OpenCV REQUIRED)
find_package(kinectWrapper REQUIRED)

file(GLOB sources src/*.cpp)
file(GLOB headers include/*.h)

source_group("Source Files" FILES ${sources})
source_group("Header Files" FILES ${headers})

include_directories(${YARP_INCLUDE_DIRS} ${ICUB_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${kinectWrapper_INCLUDE_DIRS} )
include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(${PROJECTNAME} ${sources} ${headers})
target_link_libraries(${PROJECTNAME} ${YARP_LIBRARIES} ${OpenCV_LIBRARIES} ctrlLib kinectWrapper)
install(TARGETS ${PROJECTNAME} DESTINATION bin)

