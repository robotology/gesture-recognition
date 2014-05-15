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

#include "gestureRecognition.h"

using namespace std;
using namespace cv;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;
using namespace kinectWrapper;

GestRecognition::GestRecognition()
{
    nbins=64;
    minimum=-1.00001;
    maximum=1.00001;
    thModFlow=(float)0.5;
    tol=50;
    bufferCount=1;
    countFile=1;
    showflow=false;
    rec=true;
    seatedMode=false;
    save=false;
    bodyHist.resize(nbins);
    rightHist.resize(nbins);
    leftHist.resize(nbins);
    frameNumber=1;

    mutex=new Semaphore(1);

    visualizeFlow=NULL;

    init=true;

    initC=false;
    initD=false;

    frameC=0;
    frameD=0;
}

void GestRecognition::saveFeatures(yarp::sig::Vector &Features) 
{
    featuresFile << frameNumber << " ";
    for(unsigned int i=0; i<Features.size(); i++) 
        featuresFile << " " << Features[i];

    featuresFile << "\n";
    frameNumber++;
}

bool GestRecognition::configure(ResourceFinder &rf)
{
    context=rf.getHomeContextPath();
    string name=rf.check("name",Value("gestureRecognition")).asString().c_str();
    dimension=rf.check("dimension",Value(5)).asInt();
    outDir=rf.check("outDir",Value("C:/Users/Utente/Desktop/dataKinect/")).asString().c_str();
    string showIm=rf.check("showImages",Value("true")).asString().c_str();
    
    showImages=(showIm=="true")?true:false;
    
    if (showImages)
    {
        cvNamedWindow("depth image",CV_WINDOW_AUTOSIZE);
        cvMoveWindow("depth image", 510, 100);
        cvNamedWindow("skeleton image", CV_WINDOW_AUTOSIZE);
        cvMoveWindow("skeleton image", 860, 100);
    }
    
    string rpcName=("/"+name+"/rpc");
    string outName=("/"+name+"/scores");
    string imagePortName=("/"+name+"/images");
    imagePort.open(imagePortName.c_str());
    out.open(outName.c_str());

    player=-1;

    computeEdges();

    rpc.open(rpcName.c_str());
    attach(rpc);

    string clientName=rf.check("clientName",Value("gestureRecKinectClient")).asString().c_str();
    Property options;
    options.put("carrier","tcp");
    options.put("remote","kinectServer");
    options.put("local",clientName.c_str());

    if (!client.open(options))
        return false;

    Property opt;
    client.getInfo(opt);
    rgb_width=opt.find("img_width").asInt();
    rgb_height=opt.find("img_height").asInt();
    depth_width=opt.find("depth_width").asInt();
    depth_height=opt.find("depth_height").asInt();

    depthCurrentF = cvCreateImageHeader(cvSize(depth_width,depth_height),IPL_DEPTH_32F,1);
    depthPrevious = NULL;
    RGBsmallPrevious= NULL;
    rightHand=cvCreateImage(cvSize(rgb_width,rgb_height),IPL_DEPTH_32F,1);
    leftHand=cvCreateImage(cvSize(rgb_width,rgb_height),IPL_DEPTH_32F,1);
    optFlow.create(rgb_height,rgb_width,CV_32FC2);

    Classifiers=new ModelSVM(context);
    ResourceFinder ActionModelsFile;
    ActionModelsFile.setDefaultContext("gestureRecognition");
    ActionModelsFile.setDefaultConfigFile("SVMModels.ini");
    int argc=0; char *argv[1];
    ActionModelsFile.configure(argc,argv);

    Bottle& bgeneral=ActionModelsFile.findGroup("GENERAL");
    SVMBuffer=bgeneral.check("bufferSize",Value(-1)).asInt();

    featuresSize=bgeneral.check("dictionarySize",Value(-1)).asInt();
    nFeatures=bgeneral.check("nFeatures",Value(-1)).asInt();

    frameFeatureSize=bgeneral.find("frameFeatureSize").asInt();
    frameFeatures.resize(frameFeatureSize);

    return true;
}

bool GestRecognition::close()
{
    rpc.interrupt();
    rpc.close();

    out.interrupt();
    out.close();
    
    imagePort.interrupt();
    imagePort.close();

    if(RGBsmallPrevious!=NULL)
        cvReleaseImage(&RGBsmallPrevious);

    if(depthPrevious!=NULL)
        cvReleaseImage(&depthPrevious);

    if(visualizeFlow!=NULL)
        cvReleaseImage(&visualizeFlow);

    if(leftHand!=NULL)
        cvReleaseImage(&leftHand);

    if(rightHand!=NULL)
        cvReleaseImage(&rightHand);

    cvDestroyAllWindows();

    client.close();

    delete Classifiers;
    delete mutex;
    return true;
}
void GestRecognition::findNearestPlayer(int id, IplImage* depth)
{
    for (int i=0; i<depth->width; i++)
    {
        for (int j=0; j<depth->height; j++)
        {
            if((int)players(j,i)!=id)
                cvSet2D(depth,j,i,cvScalar(0,0,0,0));
        }
    }

}
bool GestRecognition::updateModule()
{
    client.getDepthAndPlayers(depth,players);
    client.getRgb(rgb);

    RGBsmallCurrent=(IplImage*) rgb.getIplImage();
    depthCurrentF=(IplImage*) depth.getIplImage();

    if (showImages)
        cvShowImage("depth image",depthCurrentF);

    bool tracked=client.getJoints(joint, KINECT_TAGS_CLOSEST_PLAYER);
    if(tracked)
    {
        client.getSkeletonImage(joint,skeletonImage);

        int nearestID=joint.ID;

        findNearestPlayer(nearestID,depthCurrentF);

        if(init)
        {
            RGBsmallPrevious=(IplImage*) cvClone(RGBsmallCurrent);
            depthPrevious=(IplImage*) cvClone(depthCurrentF);
            init=false;
            return true;
        }

        skeleton=(IplImage*)skeletonImage.getIplImage();

        computeFlowDense(RGBsmallPrevious,RGBsmallCurrent,optFlow);
        computeSceneFlow(depthPrevious,depthCurrentF,optFlow);
        computeFlowHistogram();

        segmentHand(depthCurrentF,rightHand,leftHand,joint,skeleton);

        if(leftInImage==false && rightInImage==false)
        {
            int idx=0;

            for (int i=0; i<frameFeatureSize; i++)
            {
                frameFeatures[idx]=0.0;
                idx++;
            }
        }
        else
        {
            computeHOG(rightHand, rightHist,0.0,-1);
            computeHOG(leftHand, leftHist,0.0,-1);
            computeHOG(depthCurrentF, bodyHist,0.0,-1);

            int idx=0;
            for (size_t i=0; i<flowHist.size(); i++)
            {
                frameFeatures[idx]=flowHist[i];
                idx++;
            }

            for (size_t i=0; i<bodyHist.size(); i++)
            {
                frameFeatures[idx]=bodyHist[i];
                idx++;
            }

            for (size_t i=0; i<rightHist.size(); i++)
            {
                frameFeatures[idx]=rightHist[i];
                idx++;
            }

            for (size_t i=0; i<leftHist.size(); i++)
            {
                frameFeatures[idx]=leftHist[i];
                idx++;
            }
        }

        if(bufferCount>SVMBuffer)
             featuresBuffer.pop_front();
        featuresBuffer.push_back(frameFeatures);
        bufferCount++;

        mutex->wait();
        if(bufferCount>SVMBuffer && rec)
        {
            scores.clear();
            Classifiers->computeOutput(featuresBuffer,scores);
            recognize();
        }
        mutex->post();

        mutex->wait();
        if(save)
            saveFeatures(frameFeatures);
        mutex->post();

        if(showflow)
        {
            if(visualizeFlow!=NULL)
                cvReleaseImage(&visualizeFlow);
            visualizeFlow=(IplImage*) cvClone(RGBsmallCurrent);
            drawMotionField(visualizeFlow, 7, 7, 0.0, 5, CV_RGB(255,0,0));
        }

        if(RGBsmallPrevious!=NULL)
            cvReleaseImage(&RGBsmallPrevious);

        if(depthPrevious!=NULL)
            cvReleaseImage(&depthPrevious);

        RGBsmallPrevious=(IplImage *) cvClone(RGBsmallCurrent);
        depthPrevious=(IplImage *) cvClone(depthCurrentF);
        if(showImages)
            cvShowImage("skeleton image",skeleton);
        
        if(imagePort.getOutputCount()>0)
        {
            ImageOf<PixelBgr> outImage;
            outImage.wrapIplImage(skeleton);
            imagePort.prepare()=outImage;
            imagePort.write();
        }
    }

    if (showImages)
        cvWaitKey(1);

    return true;
}

void GestRecognition::recognize()
{
    //scores are the SVM scores at time t
    int start=0;
    int end=0;
    int index=recognizer.recognize(scores,start,end);

    if (index!=-1)
    {
        Bottle winner;
        winner.addInt(index+1);
        out.write(winner);
        cout << "Action " << index+1  << " recognized" << " starting frame: " << start << " end frame: " << end  << endl;
    }
}

double GestRecognition::getPeriod()
{
    return 0.001;
}

bool GestRecognition::respond(const Bottle& cmd, Bottle& reply)
{
    switch (cmd.get(0).asVocab())
    {
        case VOCAB_CMD_REC:
        {
            mutex->wait();
            rec=true;
            reply.addVocab(VOCAB_CMD_ACK);
            mutex->post();
            return true;
        }
        case VOCAB_CMD_STOP:
        {
            mutex->wait();
            save=false;
            rec=false;
            frameNumber=1;
            featuresFile.close();
            reply.addVocab(VOCAB_CMD_ACK);
            mutex->post();
            return true;
        }
        case VOCAB_CMD_SAVE:
        {
            mutex->wait();
            stringstream o;
            o << countFile;
            countFile=countFile+1;
            string s = o.str();
            string filename=outDir+s+".txt";
            featuresFile.open(filename.c_str(), ios_base::trunc); 
            save=true;
            rec=false;
            reply.addVocab(VOCAB_CMD_ACK);
            mutex->post();
            return true;
        }
    }
    reply.addVocab(VOCAB_CMD_NACK);
    return false;
}

void GestRecognition::computeContrastandOrientation(IplImage* img, IplImage* arctan, IplImage* contrast, double contrastTh, int aperture)
{
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

    cvThreshold(contrast,contrast,contrastTh,1,CV_THRESH_BINARY);

    cvReleaseImage(&img_f);
    cvReleaseImage(&derivativeX);
    cvReleaseImage(&derivativeY);
    cvReleaseImage(&img_t);
}

void GestRecognition::computeEdges()
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

void GestRecognition::computeFlowDense(IplImage* previous, IplImage* current, Mat &optFlow)
{
    IplImage* velx=cvCreateImage(cvGetSize(previous), IPL_DEPTH_32F, 1);
    IplImage* vely=cvCreateImage(cvGetSize(previous), IPL_DEPTH_32F, 1);
    IplImage* pGray=cvCreateImage(cvGetSize(previous), IPL_DEPTH_8U, 1);
    IplImage* cGray=cvCreateImage(cvGetSize(previous), IPL_DEPTH_8U, 1);
    double lamda=0.5;

    cvCvtColor(previous, pGray, CV_BGR2GRAY);
    cvCvtColor(current, cGray, CV_BGR2GRAY);
    cvCalcOpticalFlowHS(pGray, cGray, 0, velx, vely, lamda, cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));

    int scale=1;

    if(optFlow.rows==480 && optFlow.cols==640)
        scale=2;

    yarp::sig::Vector v(4);
    flow2D.clear();
    for (int i=0; i<previous->height; i++)
    {
        for (int j=0; j<previous->width; j++)
        {
            v[0]=(float)j/scale; // prev X
            v[1]=(float)i/scale; // prev Y
            v[2]=((float*)(velx->imageData + i*velx->widthStep))[j]/scale; //deltaX
            v[3]=((float*)(vely->imageData + i*vely->widthStep))[j]/scale; // deltaY
            optFlow.ptr<float>(i)[2*j]=v[2];
            optFlow.ptr<float>(i)[2*j+1]=v[3];

            double distance=sqrt(v[2]*v[2] + v[3]*v[3]);

            if(distance>thModFlow)
                flow2D.push_back(v);
            else
            {
                optFlow.ptr<float>(i)[2*j]=0;
                optFlow.ptr<float>(i)[2*j+1]=0;
            }
        }
    }
    cvReleaseImage(&velx);
    cvReleaseImage(&vely);
    cvReleaseImage(&pGray);
    cvReleaseImage(&cGray);
}

void GestRecognition::computeFlowHistogram()
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
        flowHist[i]=flowHist[i]/normalizer;
}

void GestRecognition::computeHOG(IplImage* image, vector<double> &currHist, double Th, int aperture, bool show)
{
    IplImage * contrastTemplate= cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
    IplImage * arctanTemplate= cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
    IplImage * maskTemplate= cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
    IplImage * edges= cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
    IplImage * img=cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);

    computeContrastandOrientation(image,arctanTemplate,contrastTemplate,Th,aperture);
    cvConvertScale(contrastTemplate,maskTemplate,255,0);

    if(show)
    {
        cvNamedWindow("Contrast",1);
        cvShowImage("Contrast", maskTemplate);
    }

    int n_bins[]={nbins};
    float range[]={0.0F,(float)(2.0*CV_PI)};
    float * ranges[] = {range};
    CvHistogram* histTemp=cvCreateHist(1, n_bins, CV_HIST_ARRAY, ranges,1);
    cvCalcHist(&arctanTemplate, histTemp, 0, maskTemplate);
    cvNormalizeHist(histTemp,1);

    for(int i=0; i<nbins; i++)
    {
        float bin_value=cvQueryHistValue_1D(histTemp,i);
        currHist[i]=bin_value;
    }

    cvReleaseImage(&contrastTemplate);
    cvReleaseImage(&arctanTemplate);
    cvReleaseImage(&maskTemplate);
    cvReleaseImage(&edges);
    cvReleaseImage(&img);
    cvReleaseHist(&histTemp);
}

void GestRecognition::computeSceneFlow(IplImage* previousMask,IplImage* currentMask, Mat &optFlow)
{
    flow3D.clear();
    Mat maskTempO(previousMask);

    Mat maskTempN(currentMask);
    yarp::sig::Vector oldPoint(3);
    yarp::sig::Vector currPoint(3);
    Point3f flow;
    for(unsigned int i=0; i<flow2D.size(); i++)
    {
        float x,y,deltaX,deltaY=0;
        x=flow2D[i][0];
        y=flow2D[i][1];
        deltaX=flow2D[i][2];
        deltaY=flow2D[i][3];

        if((cvRound(y+deltaY))<0 || (cvRound(x+deltaX))<0 || (cvRound(y+deltaY))>=currentMask->height || (cvRound(x+deltaX))>=currentMask->width  )
        {
            optFlow.ptr<float>(cvRound(y))[2*cvRound(x)]=0;
            optFlow.ptr<float>(cvRound(y))[2*cvRound(x)+1]=0;
            continue;
        }

        if(cvRound(y)<0 || cvRound(x)<0 || cvRound(y)>=previousMask->height || cvRound(x)>=previousMask->width)
        {
            optFlow.ptr<float>(cvRound(y))[2*cvRound(x)]=0;
            optFlow.ptr<float>(cvRound(y))[2*cvRound(x)+1]=0;
            continue;
        }
        float valOld= maskTempO.ptr<float>(cvRound(y))[cvRound(x)];
        float valNew= maskTempN.ptr<float>(cvRound(y+deltaY))[cvRound(x+deltaX)];

        if (valOld==0 || valNew==0)
        {
            optFlow.ptr<float>(cvRound(y))[2*cvRound(x)]=0;
            optFlow.ptr<float>(cvRound(y))[2*cvRound(x)+1]=0;
            continue;
        }

        retrieve3DPoint(previousMask,(int)x,(int)y, oldPoint);
        retrieve3DPoint(currentMask,(int)(x+deltaX),(int)(y+deltaY),currPoint);

        flow.x=currPoint[0]-oldPoint[0];
        flow.y=currPoint[1]-oldPoint[1];
        flow.z=currPoint[2]-oldPoint[2];

        if(flow.x>0.001 || flow.y>0.001 || flow.z>0.001)
            flow3D.push_back(flow);
    }
}

void GestRecognition::drawMotionField(IplImage* imgMotion, int xSpace, int ySpace, float cutoff, float multiplier, CvScalar color)
{
    CvPoint p0 = cvPoint(0,0);
    CvPoint p1 = cvPoint(0,0);
    float deltaX, deltaY, angle, hyp;

    for(unsigned int i=0; i<flow2D.size(); i++)
    {
        p0.x = (int)flow2D[i][0];
        p0.y = (int)flow2D[i][1];
        deltaX = flow2D[i][2];
        deltaY = -(flow2D[i][3]);
        angle = atan2(deltaY, deltaX);
        hyp = sqrt(deltaX*deltaX + deltaY*deltaY);

        if(hyp > cutoff)
        {
            p1.x = p0.x + cvRound(multiplier*hyp*cos(angle));
            p1.y = p0.y + cvRound(multiplier*hyp*sin(angle));
            cvLine( imgMotion, p0, p1, color,1, CV_AA, 0);
            p0.x = p1.x + cvRound(3*cos(angle-CV_PI + CV_PI/4));
            p0.y = p1.y + cvRound(3*sin(angle-CV_PI + CV_PI/4));
            cvLine( imgMotion, p0, p1, color,1, CV_AA, 0);

            p0.x = p1.x + cvRound(3*cos(angle-CV_PI - CV_PI/4));
            p0.y = p1.y + cvRound(3*sin(angle-CV_PI - CV_PI/4));
            cvLine( imgMotion, p0, p1, color,1, CV_AA, 0);
        }       
    }
}

inline string GestRecognition::findNearest(yarp::sig::Vector &currPoint, Skeleton& jointsMap)
{
    double minDist = 100;
    string index="";

    double x1=currPoint[0];
    double y1=currPoint[1];
    double z1=currPoint[2];
    
    Skeleton::iterator iterator;
    for (iterator=jointsMap.begin(); iterator!=jointsMap.end(); iterator++)
    {
        double x2=iterator->second.x;
        double y2=iterator->second.y;
        double z2=iterator->second.z;

        double dist=(x1-x2)*(x1-x2);
        dist=dist+(y1-y2)*(y1-y2);
        dist=dist+(z1-z2)*(z1-z2);

        dist=sqrt(dist);

        if(dist<minDist)
        {
            minDist=dist;
            index=iterator->first;
        }
    }

    return index;
}

void GestRecognition::generateMatIndex(double num, int &index)
{
    index=-1;
    unsigned int i;
    for (i=0; i<edges.size()-1; i++)
        if((num>=edges.at(i)) && (num<edges.at(i+1)))
            break;
    index=i;
}

void GestRecognition::retrieve3DPoint(IplImage* depth, int u,int v, yarp::sig::Vector &point3D)
{
    if(cvRound(v)<0)
        v=0;
    if(cvRound(u)<0)
        u=0;
    if(cvRound(v)>=depth->height)
        v=depth->height-1;
    if(cvRound(u)>=depth->width)
        u=depth->width-1;

    CvScalar val= cvGet2D(depth,v,u);
    ushort realdepth =(ushort)((val.val[0])*0xFFF8/1.0);

    int x=u;
    int y=v;

    float fSkeletonZ = static_cast<float>(realdepth >> 3) / 1000.0f;
    float fSkeletonX = (x - depth_width/2.0f) * (320.0f/depth_width) * KINECT_CAMERA_DEPTH_IMAGE_TO_SKELETON_MULTIPLIER_320x240 * fSkeletonZ;
    float fSkeletonY = -(y - depth_height/2.0f) * (240.0f/depth_height) * KINECT_CAMERA_DEPTH_IMAGE_TO_SKELETON_MULTIPLIER_320x240 * fSkeletonZ;

    point3D[0]=fSkeletonX;
    point3D[1]=fSkeletonY;
    point3D[2]=fSkeletonZ;
}

void GestRecognition::segmentHand(IplImage * depth, IplImage* resR, IplImage* resL, Player & joints,  IplImage* skel)
{
    cvZero(resR);
    cvZero(resL);

    rightInImage=checkHand(joints.skeleton[KINECT_TAGS_BODYPART_HAND_R], joints.skeleton[KINECT_TAGS_BODYPART_ELBOW_R],joints.skeleton[KINECT_TAGS_BODYPART_SHOULDER_R]);
    leftInImage=checkHand(joints.skeleton[KINECT_TAGS_BODYPART_HAND_L], joints.skeleton[KINECT_TAGS_BODYPART_ELBOW_L],joints.skeleton[KINECT_TAGS_BODYPART_SHOULDER_L]);

    Skeleton jointsMap=joints.skeleton;
       
    yarp::sig::Vector currPoint(3);
    for (int u=0; u<depth->width; u++)
    {
        for (int v=0; v<depth->height; v++)
        {
            float val=((float*)(depth->imageData + v*depth->widthStep))[u];

            if(val==0.0)
                continue;

            retrieve3DPoint(depth,u,v,currPoint);
            string currIndex=findNearest(currPoint,jointsMap);
            if((currIndex==KINECT_TAGS_BODYPART_HAND_R || currIndex==KINECT_TAGS_BODYPART_WRIST_R) && rightInImage)
            {
                ((float*)(resR->imageData + v*resR->widthStep))[u]=val;
                if(skel!=NULL)
                {
                    ((char*)(skel->imageData + v*skel->widthStep))[u*skel->nChannels]=(char)211;
                    ((char*)(skel->imageData + v*skel->widthStep))[u*skel->nChannels+1]=(char)0;
                    ((char*)(skel->imageData + v*skel->widthStep))[u*skel->nChannels+2]=(char)148;
                }
            }

            if((currIndex==KINECT_TAGS_BODYPART_HAND_L || currIndex==KINECT_TAGS_BODYPART_WRIST_L) && leftInImage)
            {
                ((float*)(resL->imageData + v*resL->widthStep))[u]=val;
                if(skel!=NULL)
                {
                    ((char*)(skel->imageData + v*skel->widthStep))[u*skel->nChannels]=(char)255;
                    ((char*)(skel->imageData + v*skel->widthStep))[u*skel->nChannels+1]=(char)255;
                    ((char*)(skel->imageData + v*skel->widthStep))[u*skel->nChannels+2]=(char)0;
                }
            }
        }
    }

    float depthValueH=joints.skeleton[KINECT_TAGS_BODYPART_HAND_R].z;
    thresholdBW(depth,resR,depthValueH,0.1F);

    depthValueH=joints.skeleton[KINECT_TAGS_BODYPART_HAND_L].z;
    thresholdBW(depth,resL,depthValueH,0.1F);
}

void GestRecognition::thresholdBW(IplImage *img, IplImage* res, float distance, float metersTh)
{
    ushort mmdist=(ushort) (distance*1000);
    ushort mmth=(ushort) (metersTh*1000);
    mmdist=mmdist<<3;
    mmth=mmth<<3;
    distance=((1.0*mmdist/0xFFF8));
    metersTh=((1.0*mmth/0xFFF8));
    CvSize size =cvGetSize(img);

    IplImage *up= cvCreateImage(size,img->depth,1);
    IplImage * down=cvCreateImage(size,img->depth,1);
    IplImage * mask=cvCreateImage(size,IPL_DEPTH_8U,1);

    cvThreshold(img, up, distance+metersTh, 255, CV_THRESH_TOZERO_INV);
    cvThreshold(img, down, distance-metersTh, 255, CV_THRESH_TOZERO);

    cvConvertScale(res,mask,255,0);
    cvThreshold(mask,mask,0,255,CV_THRESH_BINARY);

    cvAnd(up,down,res,mask);

    cvReleaseImage(&up);
    cvReleaseImage(&down);
    cvReleaseImage(&mask);
}

int GestRecognition::trackSkeleton(IplImage* skeleton)
{
    handPositions3D.clear();
    handPositions2D.clear();
    return 1;
}

bool GestRecognition::checkHand(Joint &wrist, Joint &elbow, Joint &shoulder)
{
    bool inImg=false;

    Point2f we;
    we.x=wrist.x-elbow.x;
    we.y=wrist.z-elbow.z;

    double wey=wrist.y-elbow.y;

    Point2f ws;
    ws.x=wrist.x-shoulder.x;
    ws.y=wrist.z-shoulder.z;

    double wsy=wrist.y-shoulder.y;

    Point2f es;
    es.x=elbow.x-shoulder.x;
    es.y=elbow.z-shoulder.z;

    double esy=elbow.y-shoulder.y;

    double n1=norm(we);
    double n2=norm(ws);
    double n3=norm(es);

    if((n1>0.30 || n2>0.30 || n3>0.30) || (wey>0.0 || wsy>0.0 || esy>0.0) || fabs(wey)<0.15 || fabs(wsy)<0.15 || fabs(esy)<0.15)
        inImg=true;

    return inImg;
}



