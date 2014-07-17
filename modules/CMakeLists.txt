# Copyright: (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ilaria Gori
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

find_package(kinectWrapper)
find_package(stereoVision)

if (kinectWrapper_FOUND)
	add_subdirectory(gestureRecognition)
else()
	message("kinectWrapper has not been found, gestureRecognition will not be compiled")
endif()

if (stereoVision_FOUND)
	add_subdirectory(gestureRecognitionStereo)
else()
	message("stereoVision has not been found, gestureRecognitionStereo will not be compiled")
endif()

add_subdirectory(game/actionPerformer)
add_subdirectory(game/gameManager)
