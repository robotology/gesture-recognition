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

#include <cv.h>
#include <highgui.h>

#include <yarp/os/RateThread.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/sig/Vector.h>
#include <yarp/math/Math.h>

#include "DictionaryLearning.h"

#define MAXPOOLING      0
#define CONCATENATE     1

class HOGThread : public yarp::os::RateThread
{
private:

    cv::Mat disparity;
    DictionaryLearning *sparse_coder;
    yarp::sig::Vector code;
    int levels;
    int nbins;
    int pool;
    bool done;
    bool work;
    bool useDictionary;
    std::vector<yarp::sig::Vector> pyramidHOG;

    IplImage* initPosition;
    IplImage* frameDiff;

    void computeHOG(IplImage* image, yarp::sig::Vector &currHist, double Th=0., int aperture=-1, bool show=false);
    void computeHOGcode(yarp::sig::Vector &currHist, yarp::sig::Vector &currCode);
    void computeBoundaries(std::vector<yarp::sig::Vector> &boundaries);
    void extractBoundariesFromLevel(int i, std::vector<yarp::sig::Vector> &boundaries);

public:

    HOGThread(yarp::os::ResourceFinder &rf);
    ~HOGThread() {};

    bool checkDone();
    bool threadInit();     
    void setImage(cv::Mat &disparity);
    void setInitialPositionFrame(IplImage* initPosition);
    void threadRelease();
    void run(); 
    yarp::sig::Vector getCode();
    std::vector<yarp::sig::Vector> getHist();
     
};
