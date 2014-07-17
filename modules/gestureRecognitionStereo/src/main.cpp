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

#include <yarp/os/Network.h>
#include <gestureRecognition.h>

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

YARP_DECLARE_DEVICES(icubmod)

int main(int argc, char *argv[])
{
    yarp::os::Network yarp;

    YARP_REGISTER_DEVICES(icubmod)

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

