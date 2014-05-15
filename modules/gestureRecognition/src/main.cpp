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

#include <yarp/os/Network.h>
#include <gestureRecognition.h>

int main(int argc, char *argv[])
{
    yarp::os::Network yarp;

    if (!yarp.checkNetwork())
        return -1;

    yarp::os::ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultContext("gestureRecognition");
    rf.setDefaultConfigFile("gestureRecognition.ini");
    rf.configure(argc,argv);

    GestRecognition mod;
    return mod.runModule(rf);

    return 1;
}

