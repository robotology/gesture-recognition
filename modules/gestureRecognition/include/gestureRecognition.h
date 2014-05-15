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

#include <string>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <opencv2/legacy/legacy.hpp>
#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/BufferedPort.h>

#include <yarp/sig/Vector.h>
#include <yarp/sig/Matrix.h>
#include <yarp/sig/Image.h>

#include <yarp/math/Math.h>

// C RunTime Header Files
#include <stdlib.h>
#if !defined(__APPLE__)
    #include <malloc.h>
    //#include <tchar.h>
#endif
#include <memory.h>
#include <cv.h>
#include <highgui.h>
#include <cxcore.h>
#include <iCub/ctrl/math.h>
#include <kinectWrapper/kinectTags.h>
#include <kinectWrapper/kinectWrapper_client.h>

#include "Recognizer.h"
#include "ModelSVM.h"

#define VOCAB_CMD_ACK           VOCAB3('a','c','k')
#define VOCAB_CMD_NACK          VOCAB4('n','a','c','k')
#define VOCAB_CMD_REC           VOCAB3('r','e','c')
#define VOCAB_CMD_SET           VOCAB3('s','e','t')
#define VOCAB_CMD_SAVE          VOCAB4('s','a','v','e')
#define VOCAB_CMD_STOP          VOCAB4('s','t','o','p')
#define NUMBER_OF_JOINTS        20

#define KINECT_CAMERA_DEPTH_NOMINAL_INVERSE_FOCAL_LENGTH_IN_PIXELS (3.501e-3f) // (1/KINECT_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS)
// Assuming a pixel resolution of 320x240
// x_meters = (x_pixelcoord - 160) * KINECT_CAMERA_DEPTH_IMAGE_TO_SKELETON_MULTIPLIER_320x240 * z_meters;
// y_meters = (y_pixelcoord - 120) * KINECT_CAMERA_DEPTH_IMAGE_TO_SKELETON_MULTIPLIER_320x240 * z_meters;
#define KINECT_CAMERA_DEPTH_IMAGE_TO_SKELETON_MULTIPLIER_320x240 (KINECT_CAMERA_DEPTH_NOMINAL_INVERSE_FOCAL_LENGTH_IN_PIXELS)

class GestRecognition: public yarp::os::RFModule
{
    kinectWrapper::KinectWrapperClient client;
    kinectWrapper::Player joint;
    std::deque<kinectWrapper::Player> joints;
    yarp::sig::Matrix players;
    yarp::sig::ImageOf<yarp::sig::PixelRgb> rgb;
    yarp::sig::ImageOf<yarp::sig::PixelFloat> depth;
    yarp::sig::ImageOf<yarp::sig::PixelBgr> playersImage;
    yarp::sig::ImageOf<yarp::sig::PixelBgr> skeletonImage;
    std::ofstream featuresFile;
    
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelBgr> > imagePort;

    Recognizer recognizer;
    yarp::os::Port rpc;
    yarp::os::Port out;

    yarp::os::Semaphore* mutex;

    IplImage* depthCurrentF;
    IplImage* depthPrevious;
    IplImage* RGBCurrent;
    IplImage* RGBsmallCurrent;
    IplImage* RGBsmallPrevious;
    IplImage* skeleton;
    IplImage* foo;
    IplImage* r;
    IplImage* g;
    IplImage* b;
    IplImage* rightHand;
    IplImage* leftHand;
    IplImage* visualizeFlow;

    int nbins;
    int frameNumber;
    double minimum;
    double maximum;
    std::vector<double> edges;
    int dimension;
    float thModFlow;
    int tol;
    int player;

    int rgb_width;
    int rgb_height;
    int depth_width;
    int depth_height;

    std::vector<cv::Point3f> flow3D;
    std::vector<yarp::sig::Vector> flow2D;
    std::vector<cv::Point3f> handPositions3D;
    std::vector<cv::Point2f> handPositions2D;

    cv::Mat optFlow;
    std::vector<double> bodyHist;
    std::vector<double> leftHist;
    std::vector<double> rightHist;
    std::vector<double> flowHist;
    yarp::sig::Vector scores;
    ModelSVM *Classifiers;

    yarp::sig::Vector frameFeatures; 
    int featuresSize;
    int nFeatures;
    int frameFeatureSize;
    std::deque<yarp::sig::Vector> featuresBuffer;

    std::string context;
    std::string outDir;
    int bufferCount;
    int countFile;
    int SVMBuffer;

    bool showflow;
    bool rec;
    bool initC;
    bool initD;
    bool init;
    bool rightInImage;
    bool leftInImage;
    bool seatedMode;
    bool showImages;
    bool save;
    
    double frameC;
    double frameD;

private:
    void computeContrastandOrientation(IplImage* img, IplImage* arctan, IplImage* contrast, double contrastTh=0.0, int aperture=-1);
    void computeEdges();
    void findNearestPlayer(int id, IplImage* depth);
    void computeFlowDense(IplImage* previous, IplImage* current, cv::Mat &optFlow);
    void computeFlowHistogram();
    void computeHOG(IplImage* image, std::vector<double> &currHist, double Th=0., int aperture=-1, bool show=false);
    void computeSceneFlow(IplImage* previousMask,IplImage* currentMask, cv::Mat &optFlow);
    void drawMotionField(IplImage* imgMotion, int xSpace, int ySpace, float cutoff, float multiplier, CvScalar color);
    std::string findNearest(yarp::sig::Vector &currPoint,kinectWrapper::Skeleton& jointsMap);
    void generateMatIndex(double num, int &index);
    void retrieve3DPoint(IplImage* depth, int u,int v, yarp::sig::Vector &point3D);
    void segmentHand(IplImage * depth, IplImage* resR, IplImage* resL, kinectWrapper::Player & joints, IplImage* skeleton);
    void thresholdBW(IplImage *img, IplImage* res, float distance, float metersTh);
    int trackSkeleton(IplImage* skeleton);
    void recognize();
    bool checkHand(kinectWrapper::Joint &wrist, kinectWrapper::Joint &elbow, kinectWrapper::Joint &shoulder);
    void saveFeatures(yarp::sig::Vector &Features);

public:

    GestRecognition();
    bool configure(yarp::os::ResourceFinder &rf);
    bool close();
    bool updateModule();
    double getPeriod();
    bool respond(const yarp::os::Bottle& cmd, yarp::os::Bottle& reply);
};

