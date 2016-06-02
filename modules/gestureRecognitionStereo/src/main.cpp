/*
 * Copyright (C) 2013 EFAA Consortium, European Commission FP7 Project IST-270490
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
\defgroup gestureRecognitionStereo gestureRecognitionStereo

A module that trains and recognizes gestures in real-time using 3DHOF
and HOG descriptors and linear SVMs as classifiers.

\section intro_sec Description 
This module is able to train and recognize in real-time different gestures.
3DHOF and pyramid-HOG are used as descriptors. 
 
\section rpc_port Commands:

The commands sent as bottles to the module port /<modName>/rpc
are described in the following:

<b>INITIAL_POSITION</b> \n
format: [pos] \n
action: since we don't have skeleton information in this case, we need to work
using the difference between an initial status and the subsequent frames to retrieve
information on where the gesture is happening. To this end, this command save 
the current frame as the initial position, to which the successive frames will 
be compared.

<b>SET</b> \n
format: [set param] \n
action: param can be value or threshold. For an initial background removal, 
it is possible to set a depth value and a threshold that allows setting to 0 all 
the pixels that are closer than value+threshold and that are further than 
value-threshold.

<b>SAVE</b> \n
format: [save param] \n
action: this command allows saving in the linearClassifierModule all the features
from this moment until STOP is commanded. The descriptors are directly saved 
in a folder called actionX, where X=param is an integer.

<b>TRAIN</b> \n
format: [train] \n
action: all the descriptors saved so far are fed into the linearClassifierModule and
support vector machines are trained accordingly.

<b>REC</b> \n
format: [rec] \n
action: the module starts to recognize any action that is being performed.

<b>WRITE</b> \n
format: [write] \n
action: the module starts saving features on a .txt file located in the directory
specified as outDir in the ResourceFinder.

<b>STOP</b> \n
format: [stop] \n
action: the module stops recognizing or saving.

\section lib_sec Libraries 
- YARP libraries. 
- \ref stereo-vision library.
- \ref OpenCV library.

\section portsc_sec Ports Created 

- \e /<modName>/rpc remote procedure call. It always replies something.
- \e /<modName>/scores:i this is the port where the linearClassifierModule
    replies the gesture that has been recognized at every frame.
- \e /<modName>/features:o this port outputs the features to be sent to
    the linearClassifierModule.
- \e /<modName>/classifier:rpc this port sends rpc commands like train or 
    save to the linearClassifierModule.
- \e /<modName>/scores:o this port sends the gameManager the gestures that
    have been recognized.
- \e /<modName>/ispeak this port sends sentences to iSpeak.
- \e /<modName>/depth this port sends the segmented depth image.

\section parameters_sec Parameters 
The following are the options that are usually contained 
in the configuration file, which is gestureRecognition.ini:

--name \e name
- specify the module name, which is \e gestureRecognitionStereo by 
  default.

--robot \e robot
- specify the robot to be used, \icub by default.

--HOFnbins \e HOFnbins
- number of bins for the 3D histogram of flow. The final histogram will
  be HOFnbins*HOFnbins*HOFnbins. HOFnbins is equal to 5 by default. 

--HOGnbins \e HOGnbins
- number of bins for the HOG. It is 128 by default.

--HOGlevels \e HOGlevels
- number of levels of the pyramid HOG. 3 by default.

--pool \e pool
- pooling strategy. can be "concatenate" or "max". "concatenate is
  usually used, especially if a dictionary is not specified.

-- outDir \e outDir
- folder where the features will be saved, if the write command is
  sent to the rpc port.

-- dictionary \e dictionary
- if this is true, a dictionary will be used. In this case the dictionary
    is specified in another config file.

-- threshold \e threshold
- threshold on the value of the depth that is given.

-- value \e value
- depth value at which the person should be moving.

-- config_disparity \e config_disparity
- file where the disparity information is stored. It is icubEyes.ini by default.

If a dictionary is specified, there exist two other config files: dictionary_hog.ini
and dictionary_flow.ini. Both of them have a GENERAL group that contains:

-- dictionarySize \e dictionarySize
- file where the disparity information is stored. It is icubEyes.ini by default.

\section tested_os_sec Tested OS
Windows, Linux

\author Ilaria Gori, Sean Ryan Fanello
**/

#include <yarp/os/Network.h>
#include "gestureRecognition.h"

void printMatrixYarp(yarp::sig::Matrix &A) 
{
    cout << endl;
    for (int i=0; i<A.rows(); i++) 
    {
        for (int j=0; j<A.cols(); j++) 
        {
            cout<<A(i,j)<<" ";
        }
        cout<<endl;
    }
    cout << endl;

}

int main(int argc, char *argv[])
{
    yarp::os::Network yarp;

    if (!yarp.checkNetwork())
        return -1;

    yarp::os::ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultContext("allGesturesYouCan2.0");
    rf.setDefaultConfigFile("config.ini");
    rf.setDefault("dictionary_hog","dictionary_hog.ini");
    rf.setDefault("dictionary_flow","dictionary_flow.ini");
    rf.configure(argc,argv);

    GestRecognition mod;
    mod.runModule(rf);

    return 1;
}

