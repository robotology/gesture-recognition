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

#include "sceneFlowThread.h"

SceneFlowThread::SceneFlowThread(yarp::os::ResourceFinder &rf, DisparityThread* d, OpticalFlowThread* o) : RateThread(10) 
{
    success=true;
    init=true;
    done=false;
    work=false;

    dimension=rf.check("dimension",Value(5)).asInt();
    useDictionary=rf.check("dictionary");
    minimum=-1.00001;
    maximum=1.00001;

    computeEdges();
    
    if (useDictionary)
    {
        string dictionary_path=rf.findFile("dictionary_flow").c_str();
        string dictionary_group=rf.check("dictionary_group",Value("DICTIONARY")).asString().c_str();
        sparse_coder=new DictionaryLearning(dictionary_path,dictionary_group);
    }
    disp=d;
    opt=o;
}

bool SceneFlowThread::isOpen()
{
    return success;
}

yarp::sig::Vector SceneFlowThread::getCode()
{
    return this->code;
}

bool SceneFlowThread::checkDone()
{
    return done;
}

void SceneFlowThread::threadRelease()
{
    if(useDictionary && sparse_coder!=NULL)
        delete sparse_coder;
}

void SceneFlowThread::run()
{
    if(!success)
        return;

    if (work)
    {
        points3Dold=points3D;
        get3DPoints(points3D);
        opt->getOptFlow(optFlow);
        vector<Point3f> flow3D;
        if (points3D.rows>0 && points3Dold.rows>0)
            getSceneFlow(flow3D);
        computeFlowHistogram(flow3D, flowHist);
        if (useDictionary)
            computeFlowCode(flowHist,code);        
    }
    
    done=true;
    this->suspend();
}

void SceneFlowThread::computeFlowCode(yarp::sig::Vector& flowHist, yarp::sig::Vector& code)
{
    sparse_coder->computeCode(flowHist,code);
}

void SceneFlowThread::get3DPoints(Mat &points)
{
    points.create(optFlow.rows,optFlow.cols,CV_32FC3);
    for (int v=0; v<optFlow.rows; v++)
    {
        for(int u=0; u<optFlow.cols; u++)
        {
            if (disparity.at<double>(v,u)==0.0)
                continue;

            Point2f point2D;
            point2D.x=(float)u;
            point2D.y=(float)v;
            Point3f point3D;
            triangulate(point2D,point3D);
            
            points.at<Vec3f>(v,u)[0]=point3D.x;
            points.at<Vec3f>(v,u)[1]=point3D.y;
            points.at<Vec3f>(v,u)[2]=point3D.z;
        }
    }
}

void SceneFlowThread::triangulate(Point2f &pixel,Point3f &point) 
{
    disp->triangulate(pixel,point);
    if (point.x<-4.0)
    {
        point.x=0.0;
        point.y=0.0;
        point.z=0.0;
    }
}

void SceneFlowThread::setImage(Mat &disparity) 
{
    this->disparity=disparity;
    work=true;
    done=false;
}

Point3f SceneFlowThread::getSceneFlowPixel(int u, int v)
{
    Point3f flowPoint;
    flowPoint.x=0.0;
    flowPoint.y=0.0;
    flowPoint.z=0.0;

    if(v>=optFlow.rows || u>=optFlow.cols || optFlow.empty())
    {
        return flowPoint;
    }

    Point3f point3Dold;
    point3Dold.x=points3Dold.at<Vec3f>(v,u)[0];
    point3Dold.y=points3Dold.at<Vec3f>(v,u)[1];
    point3Dold.z=points3Dold.at<Vec3f>(v,u)[2];

    if (point3Dold.z==0.0)
    {
        return flowPoint;
    }

    Point2f point2Dnew;
    point2Dnew.x=u+optFlow.ptr<float>(v)[2*u];
    point2Dnew.y=v+optFlow.ptr<float>(v)[2*u+1];
    Point3f point3Dnew;
    triangulate(point2Dnew,point3Dnew);
    if (point3Dnew.z==0.0)
    {
        return flowPoint;
    }

    flowPoint.x=point3Dnew.x-point3Dold.x;
    flowPoint.y=point3Dnew.y-point3Dold.y;
    flowPoint.z=point3Dnew.z-point3Dold.z;

    return flowPoint;
}


void SceneFlowThread::getSceneFlow(vector<Point3f> &flow3D)
{
    flow3D.clear();
    Point3f flowPoint;
    flowPoint.x=0.0;
    flowPoint.y=0.0;
    flowPoint.z=0.0;

    if(optFlow.empty())
        return;
    
    for (int v=0; v<optFlow.rows; v++)
    {
        for(int u=0; u<optFlow.cols; u++)
        {
            Point3f point3Dold;
            point3Dold.x=points3Dold.at<Vec3f>(v,u)[0];
            point3Dold.y=points3Dold.at<Vec3f>(v,u)[1];
            point3Dold.z=points3Dold.at<Vec3f>(v,u)[2];

            if (point3Dold.x>=0.0)
                continue;

            Point2f point2Dnew;
            point2Dnew.x=u+optFlow.ptr<float>(v)[2*u];
            point2Dnew.y=v+optFlow.ptr<float>(v)[2*u+1];

            /*printf("method 1 %g %g\n", point2Dnew.x, point2Dnew.y);

            Point2f tmp;
            tmp.x=u+optFlow.ptr<float>(v,u)[0];
            tmp.y=v+optFlow.ptr<float>(v,u)[1];

            printf("method 2 %g %g\n", tmp.x, tmp.y);*/

            if(point2Dnew.x<0 || point2Dnew.y<0 || point2Dnew.x>=optFlow.cols || point2Dnew.y>=optFlow.rows)
                continue;

            if((int)point2Dnew.x==u && (int)point2Dnew.y==v)
                continue;

            Point3f point3Dnew;

            triangulate(point2Dnew,point3Dnew);
            if (point3Dnew.x>=0.0)
                continue;
            
            //fprintf(stdout, "2dFlow: %i %i %i %i 3dPoints %f %f %f %f %f %f\n",u,v, (int)point2Dnew.x, (int)point2Dnew.y,point3Dold.x,point3Dold.y,point3Dold.z,point3Dnew.x,point3Dnew.y,point3Dnew.z);
            flowPoint.x=point3Dnew.x-point3Dold.x;
            flowPoint.y=point3Dnew.y-point3Dold.y;
            flowPoint.z=point3Dnew.z-point3Dold.z;

            if (abs(flowPoint.x)>3.0 || abs(flowPoint.y)>3.0 || abs(flowPoint.z)>3.0)
                continue;

            flow3D.push_back(flowPoint);
        }
    }
}


void SceneFlowThread::drawFlowModule(IplImage* imgMotion)
{
    IplImage * module =cvCreateImage(cvSize(imgMotion->width,imgMotion->height),32,1);
    IplImage * moduleU =cvCreateImage(cvSize(imgMotion->width,imgMotion->height),8,1);
    Mat vel[2];
    split(optFlow,vel);
    IplImage tx=(Mat)vel[0];
    IplImage ty=(Mat)vel[1];

    IplImage* velxpow=cvCloneImage(&tx);
    IplImage* velypow=cvCloneImage(&ty);

    cvPow(&tx, velxpow, 2);
    cvPow(&ty, velypow, 2);

    cvAdd(velxpow, velypow, module, NULL);
    cvPow(module, module, 0.5);
    cvNormalize(module, module, 0.0, 1.0, CV_MINMAX, NULL);
    cvZero(imgMotion);
    cvConvertScale(module,moduleU,255,0);
    cvMerge(moduleU,moduleU,moduleU,NULL,imgMotion);
    cvReleaseImage(&module);
    cvReleaseImage(&moduleU);
}


bool SceneFlowThread::threadInit()
{
    return true;
}

void SceneFlowThread::printMatrix(Mat &matrix) 
{
    int row=matrix.rows;
    int col =matrix.cols;
        cout << endl;
    for(int i = 0; i < matrix.rows; i++)
    {
        const double* Mi = matrix.ptr<double>(i);
        for(int j = 0; j < matrix.cols; j++)
            cout << Mi[j] << " ";
        cout << endl;
    }
        cout << endl;
}


void SceneFlowThread::generateMatIndex(double num, int &index)
{
    index=-1;
    unsigned int i;
    for (i=0; i<edges.size()-1; i++)
        if((num>=edges.at(i)) && (num<edges.at(i+1)))
            break;
    index=i;
}

void SceneFlowThread::computeEdges()
{
    edges.push_back(minimum);
    double div=(maximum-minimum)/dimension;
    double temp=minimum;
    for (int i=0; i<dimension-1; i++)
    {
        temp+=div;
        edges.push_back(temp);
    }
    edges.push_back(maximum);
}

void SceneFlowThread::computeFlowHistogram(const vector<Point3f>& flow3D,  yarp::sig::Vector& flowHist)
{
    flowHist.clear();

    if (flow3D.size()==0)
    {
        for (int i=0; i<dimension*dimension*dimension; i++)
            flowHist.push_back(0);
        return;
    }

    vector<Mat> matHistogram;
    for (int i=0; i<dimension; i++)
    {
        Mat matrix=Mat::zeros(dimension,dimension,CV_32F);
        matHistogram.push_back(matrix);
    }

    for (unsigned int i=0; i<flow3D.size(); i++)
    {
        int x,y,z;
        double n=sqrt((flow3D.at(i).x*flow3D.at(i).x)+(flow3D.at(i).y*flow3D.at(i).y)+(flow3D.at(i).z*flow3D.at(i).z));
        generateMatIndex((flow3D.at(i).x/n),x);
        generateMatIndex((flow3D.at(i).y/n),y);
        generateMatIndex((flow3D.at(i).z/n),z);
        if(x<0 || y<0 || z<0 || x>=dimension || y>=dimension || z>=dimension)
            continue;
        matHistogram.at(x).ptr<float>(y)[z]+=1;
    }
    
    double normalizer=0;
    for (unsigned int i=0; i<matHistogram.size(); i++)
    {
        for (int j=0; j<dimension; j++)
        {
            for (int k=0; k<dimension; k++)
            {
                flowHist.push_back(matHistogram.at(i).ptr<float>(j)[k]);
                normalizer=normalizer+matHistogram.at(i).ptr<float>(j)[k];
            }
        }
    }

    for (unsigned int i=0; i<flowHist.size(); i++)
    {
        if(normalizer>0)
            flowHist[i]=flowHist[i]/normalizer;
    }

    return;
}

yarp::sig::Vector SceneFlowThread::getHist()
{
    return this->flowHist;
}

