# Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ilaria Gori
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

option(BUILD_AGAINST_KINECTWRAPPER "Use kinectWrapper to build gestureRecognition" ON)
option(BUILD_AGAINST_STEREO "Use stereoVision to build gestureRecognitionStereo" ON)

find_package(kinectWrapper)
find_package(stereoVision)

if(kinectWrapper_FOUND)
  if(BUILD_AGAINST_KINECTWRAPPER)  
    add_subdirectory(gestureRecognition)
  endif()
else()
	message("kinectWrapper has not been found, gestureRecognition will not be built")
endif()

if(stereoVision_FOUND)
  if(BUILD_AGAINST_STEREO)  
    add_subdirectory(gestureRecognitionStereo)
  endif()
else()
	message("stereoVision has not been found, gestureRecognitionStereo will not be built")
endif()

add_subdirectory(game/actionPerformer)
add_subdirectory(game/gameManager)
