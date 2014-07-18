/*
 * Copyright (C) 2012 EFAA Consortium, European Commission FP7 Project IST-270490
 * Authors: Ilaria Gori, Sean Ryan Fanello
 * email:   ilaria.gori@iit.it, sean.fanello@iit.it
 * website: http://efaa.upf.edu/
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * $EFAA_ROOT/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

/** 
@ingroup robotology
\defgroup gestureRecognition gestureRecognition

A module that recognizes in real-time gestures belonging to a predefined set using 3DHOF
and HOG descriptors and linear SVMs as classifiers.

\section intro_sec Description 
This module is able to recognize different gestures that belong to a predefined
pool. It is possible to save features and train new Support Vector Machines using libsvm. 
Right now we provide a file to recognize 6 gestures depicted in 
app/conf/supported_actions_easy.jpg. 3DHOF and HOG are used as descriptors. 
For further information:

"S. R. Fanello*, I. Gori*, G. Metta, F. Odone
Keep It Simple And Sparse: Real-Time Action Recognition.
Journal of Machine Learning Research (JMLR), 2013."
 
\section rpc_port Commands:

The commands sent as bottles to the module port /<modName>/rpc
are described in the following:

<b>REC</b> \n
format: [rec] \n
action: the module starts to recognize any action that is being performed.

<b>SAVE</b> \n
format: [save] \n
action: the module starts saving features on a .txt file located in the directory
specified as outDir in the ResourceFinder.

<b>STOP</b> \n
format: [stop] \n
action: the module stops recognizing or saving.

\section lib_sec Libraries 
- YARP libraries. 
- \ref kinect-wrapper library.
- \ref OpenCV library.

\section portsc_sec Ports Created 

- \e /<modName>/rpc remote procedure call. It always replies something.
- \e /<modName>/scores this is the port where the module outputs the 
    result of the recognition procedure.
- \e /<modName>/images this port outputs an image containing the player,
    his skeleton joints and his hands segmented with two different colors.

\section parameters_sec Parameters 
The following are the options that are usually contained 
in the configuration file, which is gestureRecognition.ini:

--name \e name
- specify the module name, which is \e gestureRecognition by 
  default.

--dimension \e dimension
- number of bins for the 3D histogram of flow. The final histogram will
  be dimension*dimension*dimension.

-- outDir \e outDir
- folder where the features will be saved, if the save command is
  sent to the rpc port.

-- showImages \e showImages
- if this is true, depth and skeleton will be visualized.

There is another configuration file that should be provided, called SVMModels.ini. 
It is the file that contains information on the SVMs that have been trained on 
the gestures it is wanted to recognize. One example file is already provided, and 
it allows recognizing 6 gestures specified in the file app/conf/supported_actions_easy.jpg.
Those gestures can also be used to play the All Gestures You Can game. For
further information: 

I. Gori, S. R. Fanello, G. Metta, F. Odone
All Gestures You Can: A Memory Game Against A Humanoid Robot.
In proceedings of IEEE-RAS International Conference on Humanoid Robots, 2012.

The parameters that should be present in the SVMModels.ini file are:

-- numActions \e numActions
- in the group GENERAL, number of trained actions.

-- nFeatures \e nFeatures
- in the group GENERAL. It is 2 if 3DHOF and HOG on the whole body are used. If also
  HOGs on the two hands are used, this parameter should be equal to 4.

-- dictionarySize \e dictionarySize
- in the group GENERAL. Not used for now. It is needed if a dictionary learning step
  has to be applied.

-- bufferSize \e bufferSize
- in the group GENERAL. Number of frames to be evaluated at each step.

-- frameFeatureSize \e frameFeatureSize
- in the group GENERAL. Size of the final frame descriptor.

For each action there is a group called ACTIONX, where X is a number going from 1 to
the number of trained actions. In each ACTIONX group there should be the parameter w, 
which is a list of the coefficients of the trained SVM.

\section tested_os_sec Tested OS
Windows, Linux

\author Ilaria Gori, Sean Ryan Fanello
**/

#include <yarp/os/Network.h>
#include <gestureRecognition.h>

int main(int argc, char *argv[])
{
    yarp::os::Network yarp;

    if (!yarp.checkNetwork())
        return -1;

    yarp::os::ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultContext("gesture-recognition");
    rf.setDefaultConfigFile("gestureRecognition.ini");
    rf.configure(argc,argv);

    GestRecognition mod;
    return mod.runModule(rf);

    return 1;
}

