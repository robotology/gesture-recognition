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

#include "HOGThread.h"

using namespace cv;
using namespace yarp::os;
using namespace yarp::math;

HOGThread::HOGThread(ResourceFinder &rf) : RateThread(20) 
{
    initPosition=NULL;
    frameDiff=NULL;
    this->levels=(rf.check("HOGlevels",Value(3)).asInt())-1;
    this->nbins=rf.check("HOGnbins",Value(128)).asInt();
    this->useDictionary=rf.check("dictionary");
    string pooling=rf.check("pool",Value("concatenate")).asString().c_str();
    if (pooling=="max")
        this->pool=MAXPOOLING;
    else
        this->pool=CONCATENATE;

    //string contextPath=rf.getHomeContextPath().c_str();
    //string dictionary_name=rf.check("dictionary_file",Value("dictionary_hog.ini")).asString().c_str();

    if (useDictionary)
    {
        string dictionary_path=rf.findFile("dictionary_hog").c_str();
        string dictionary_group=rf.check("dictionary_group",Value("DICTIONARY")).asString().c_str();
        sparse_coder=new DictionaryLearning(dictionary_path,dictionary_group);
    }

    work=false;
    done=true;
}

void HOGThread::run() 
{
    if(work)
    {
        code.clear();
        if (initPosition==NULL)
        {
            initPosition=cvCreateImage(cvSize(disparity.cols,disparity.rows),8,1);
            cvZero(initPosition);
        }
        if (frameDiff==NULL)
        {
            frameDiff=cvCreateImage(cvSize(disparity.cols,disparity.rows),8,1);
            cvZero(frameDiff);
        }
        IplImage disp=disparity;
        cvAbsDiff(initPosition,&disp,frameDiff);
        cvThreshold(frameDiff,frameDiff,10,1,CV_THRESH_BINARY);
        cvMul(&disp,frameDiff,&disp);
        yarp::sig::Vector currHist;
        yarp::sig::Vector currCode;

        std::vector<yarp::sig::Vector> boundaries;
        computeBoundaries(boundaries);
        pyramidHOG.clear();

        for (unsigned int i=0; i<boundaries.size(); i++)
        {
            currHist.clear();
            currCode.clear();
            cv::Rect roi=cv::Rect((int)boundaries[i][0],(int)boundaries[i][1],(int)boundaries[i][2],(int)boundaries[i][3]);
            cv::Mat roiImg;
            roiImg=disparity(roi);
            IplImage dispImage=roiImg;
            computeHOG(&dispImage,currHist);
            pyramidHOG.push_back(currHist);

            if (useDictionary)
            {
                computeHOGcode(currHist,currCode);

                if (pool==MAXPOOLING)
                {
                    if (i==0)
                        code=currCode;
                    else
                    {
                        for (unsigned int i=0; i<currCode.size(); i++)
                        {
                            if (currCode[i]>code[i])
                                code[i]=currCode[i];
                        }
                    }
                }
                else
                {
                    double n=norm(currCode);
                    if(n>0)
                        currCode/=n;
                    for (unsigned int i=0; i<currCode.size(); i++)
                        code.push_back(currCode[i]);
                }
            }
        }

        done=true;
        work=false;
    }
    this->suspend();
}

void HOGThread::setImage(Mat &disparity) 
{
    this->disparity=disparity;
    done=false;
    work=true;
}

bool HOGThread::threadInit() 
{
    return true;
}

void HOGThread::threadRelease() 
{
    if(useDictionary && sparse_coder!=NULL)
        delete sparse_coder;
    if (initPosition!=NULL)
        cvReleaseImage(&initPosition);
    if (frameDiff!=NULL)
        cvReleaseImage(&frameDiff);
}

bool HOGThread::checkDone() 
{
    return done;
}

void HOGThread::computeHOG(IplImage* img, yarp::sig::Vector &currHist, double Th, int aperture, bool show)
{
    IplImage * contrast= cvCreateImage(cvGetSize(img), IPL_DEPTH_32F, 1);
    IplImage * arctan= cvCreateImage(cvGetSize(img), IPL_DEPTH_32F, 1);
    IplImage * mask= cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);
    IplImage * edges= cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);

    double max =0;
    double min =0;
    IplImage *img_t = cvCreateImage(cvGetSize(img),img->depth,1);
    IplImage *img_f = cvCreateImage(cvGetSize(img),32,1);

    IplImage * derivativeX= cvCreateImage(cvGetSize(img),32,1);
    IplImage * derivativeY= cvCreateImage(cvGetSize(img),32,1);

    if(img->nChannels>1)
        cvCvtColor(img,img_t,CV_RGB2GRAY);
    else
        cvCopy(img,img_t,0);

    cvMinMaxLoc(img_t,&min,&max,NULL,NULL,NULL);
    cvConvertScale(img_t,img_f,1.0/max,0);

    cvSobel(img_f,derivativeX,1,0,aperture);
    cvSobel(img_f,derivativeY,0,1,aperture);

    cvZero(arctan);
    cvZero(contrast);
    cvCartToPolar(derivativeX, derivativeY, contrast, arctan, 0);

    cvThreshold(contrast,contrast,Th,1,CV_THRESH_BINARY);
    
    cvConvertScale(contrast,mask,255,0);

    int n_bins[]={nbins};
    float range[]={0,2*CV_PI };
    float * ranges[] = {range};
    CvHistogram* histTemp=cvCreateHist(1, n_bins, CV_HIST_ARRAY, ranges,1);
    cvCalcHist(&arctan, histTemp, 0, mask);
    cvNormalizeHist(histTemp,1);

    for(int i=0; i<nbins; i++)
    {   
        float bin_value=cvQueryHistValue_1D(histTemp,i);
        currHist.push_back(bin_value);
    }

    cvReleaseImage(&img_f);
    cvReleaseImage(&derivativeX);
    cvReleaseImage(&derivativeY);
    cvReleaseImage(&img_t);
    cvReleaseImage(&contrast);
    cvReleaseImage(&arctan);
    cvReleaseImage(&mask);
    cvReleaseImage(&edges);
    cvReleaseHist(&histTemp);
}

void HOGThread::computeBoundaries(std::vector<yarp::sig::Vector> &boundaries)
{
    yarp::sig::Vector dim(4);
    dim[0]=0;
    dim[1]=0;
    dim[2]=disparity.cols;
    dim[3]=disparity.rows;
    boundaries.push_back(dim);
    if (levels>0)
        for (int i=1; i<levels+1; i++)
            extractBoundariesFromLevel(i,boundaries);
}

void HOGThread::extractBoundariesFromLevel(int i, std::vector<yarp::sig::Vector> &boundaries)
{
    int cellsPerDimension=(int)std::pow(2.0,(double)i);
    int displacementX=disparity.cols/cellsPerDimension;
    int displacementY=disparity.rows/cellsPerDimension;
    int currX=0;
    int currY=0;
    for (int i=0; i<cellsPerDimension; i++)
    {
        for (int j=0; j<cellsPerDimension; j++)
        {
            yarp::sig::Vector boundary(4);
            boundary[0]=currX;
            boundary[1]=currY;
            boundary[2]=displacementX;
            boundary[3]=displacementY;
            boundaries.push_back(boundary);
            if (currX+displacementX==disparity.cols)
                currX=0;
            else
                currX+=displacementX;
        }
        currY+=displacementY;
    }
}

std::vector<yarp::sig::Vector> HOGThread::getHist()
{
    return this->pyramidHOG;
}

yarp::sig::Vector HOGThread::getCode()
{
    return this->code;
}

void HOGThread::computeHOGcode(yarp::sig::Vector &currHist, yarp::sig::Vector &currCode)
{
    sparse_coder->computeCode(currHist,currCode);
}

void HOGThread::setInitialPositionFrame(IplImage* initPosition)
{
    if (this->initPosition!=NULL)
        cvReleaseImage(&this->initPosition);
    this->initPosition=(IplImage*)cvClone(initPosition);
}

