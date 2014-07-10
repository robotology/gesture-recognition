# Copyright: 2010 RobotCub Consortium
# Author: Sean Ryan Fanello, Ilaria Gori
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
cmake_minimum_required(VERSION 2.6)

project(gestureRecognitionStereo)

find_package(OpenCV REQUIRED)
find_package(stereoVision REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${stereoVision_INCLUDE_DIRS})

file(GLOB source src/*.cpp)
file(GLOB header include/*.h)

source_group("Source Files" FILES ${source})
source_group("Header Files" FILES ${header})

add_executable(gestureRecognitionStereo ${source} ${header} )
target_link_libraries(gestureRecognitionStereo ${YARP_LIBRARIES} ${OpenCV_LIBRARIES} stereoVision ctrlLib)
install(TARGETS gestureRecognitionStereo DESTINATION bin)

