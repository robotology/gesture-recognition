#include "gestureRecognition.h"

using namespace std;
using namespace cv;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;

GestRecognition::GestRecognition()
{
    frameNumber=1;
    bufferCount=1;
    countFile=1;

    mutex=new Semaphore(1);

    init=true;

    frameNumber=1;
    bufferCount=1;
    sumInitialFrame=0;

    frameDiff=NULL;
    initPosition=NULL;
    outDisp=NULL;
    outOpt=NULL;
    maskNew=NULL;
    tmpMaskNew=NULL;
}

bool GestRecognition::configure(ResourceFinder &rf)
{
    string robot=rf.check("robot",Value("icub")).asString().c_str();
    string name=rf.check("name",Value("gest_rec")).asString().c_str();
    string imL=rf.check("imL",Value("/leftPort")).asString().c_str();
    string imR=rf.check("imR",Value("/rightPort")).asString().c_str();
    string rpcName=rf.check("rpcName",Value("/gest_rec/rpc")).asString().c_str();
    string dispName=rf.check("dispPort",Value("/gest_rec/dispPort")).asString().c_str();
    string optName=rf.check("optPort",Value("/gest_rec/optPort")).asString().c_str();
    useDictionary=rf.check("dictionary");
    printf("usedictionary %s\n", useDictionary?"true":"false");
    threshold=rf.check("threshold",Value(50)).asInt();
    value=rf.check("value",Value(80)).asInt();
    context=rf.getHomeContextPath();

    outDir=rf.check("outDir",Value("/tmp/dataAction/")).asString().c_str();

    rpc.open(rpcName.c_str());
    attach(rpc);

    featureOutput.open(("/"+name+"/features:o").c_str());
    scoresInput.open(("/"+name+"/scores:i").c_str());
    rpcClassifier.open(("/"+name+"/classify:rpc").c_str());
    out.open(("/"+name+"/scores").c_str());
    outSpeak.open(("/"+name+"/ispeak").c_str());
    outImage.open(("/"+name+"/outImage").c_str());

    imagePortInLeft.open(imL.c_str());
    imagePortInRight.open(imR.c_str());
    dispPort.open(dispName.c_str());
    optPort.open(optName.c_str());

    imageL=new ImageOf<PixelRgb>;
    imageR=new ImageOf<PixelRgb>;
    init=true;
    initL=initR=false;

    string configFileDisparity=rf.check("ConfigDisparity",Value("icubEyes.ini")).asString().c_str();

    ResourceFinder cameraFinder;
    cameraFinder.setDefaultContext("cameraCalibration");
    cameraFinder.setDefaultConfigFile(configFileDisparity.c_str());
    int argc=0; char *argv[1];
    cameraFinder.configure(argc,argv);

    disp=new DisparityThread(name,cameraFinder,false,false);
    //disp->setDispParameters(true,15,50,16,64,7,0,63,0);
    opt=new OpticalFlowThread(rf);
    sceneT=new SceneFlowThread(rf,disp,opt);
    hogT=new HOGThread(rf);
    disp->start();
    opt->start();
    hogT->start();
    
    SVMBuffer=10;
    string pooling=rf.check("pool",Value("concatenate")).asString().c_str();
    int levels=(rf.check("HOGlevels",Value(3)).asInt());

    int spatialBins=0;
    for (int i=0; i<levels; i++)
        spatialBins=spatialBins+(int)std::pow(2.0,(double)i+i);

    if (useDictionary)
    {
        string dictionaryHogFile=rf.findFile("dictionary_hog").c_str();
        Property config; config.fromConfigFile(dictionaryHogFile.c_str());

        Bottle& bgeneral=config.findGroup("GENERAL");
        int dictionaryHogSize=bgeneral.find("dictionarySize").asInt();

        string dictionaryFlowFile=rf.findFile("dictionary_flow").c_str();
        Property config2; config2.fromConfigFile(dictionaryFlowFile.c_str());

        Bottle& bgeneral2=config2.findGroup("GENERAL");
        int dictionaryFlowSize=bgeneral2.find("dictionarySize").asInt();

        if (pooling=="max")
            frameFeatureSize=dictionaryFlowSize+dictionaryHogSize;
        else
            frameFeatureSize=dictionaryFlowSize+(dictionaryHogSize*spatialBins);
    }
    else
    {
        int dimFlow=rf.check("HOFnbins",Value(5)).asInt();
        int featureFlowSize=dimFlow*dimFlow*dimFlow;

        int dimHog=rf.check("HOGnbins",Value(128)).asInt();
        int featureHogSize=dimHog;
        for (int i=1; i<levels; i++)
        {
            featureHogSize+=pow(2.0,i)*pow(2.0,i)*dimHog;
        }
        frameFeatureSize=featureFlowSize+featureHogSize;
    }

    frameFeatures.resize(frameFeatureSize);

    state=STATE_DONOTHING;

    return true;
}

bool GestRecognition::close()
{
    rpc.interrupt();
    rpc.close();

    outImage.interrupt();
    outImage.close();

    out.interrupt();
    out.close();

    featureOutput.interrupt();
    featureOutput.close();
    scoresInput.interrupt();
    scoresInput.close();
    rpcClassifier.interrupt();
    rpcClassifier.close();

    disp->stop();
    delete disp;
    opt->stop();
    delete opt;

    hogT->stop();
    delete hogT;
    sceneT->stop();
    delete sceneT;

    imagePortInLeft.interrupt();
    imagePortInLeft.close();
    imagePortInRight.interrupt();
    imagePortInRight.close();
    dispPort.interrupt();
    dispPort.close();
    optPort.interrupt();
    optPort.close();

    if(outDisp!=NULL)
        cvReleaseImage(&outDisp);
    if(outOpt!=NULL)
        cvReleaseImage(&outOpt);
    if(maskNew!=NULL)
        cvReleaseImage(&maskNew);
    if (initPosition!=NULL)
        cvReleaseImage(&initPosition);
    if (frameDiff!=NULL)
        cvReleaseImage(&frameDiff);
    if(tmpMaskNew!=NULL)
        cvReleaseImage(&tmpMaskNew);

    delete imageL;
    delete imageR;

    delete mutex;
    return true;
}

bool GestRecognition::updateModule()
{
    ImageOf<PixelRgb> *tmpL = imagePortInLeft.read(false);
    ImageOf<PixelRgb> *tmpR = imagePortInRight.read(false);

    if(tmpL!=NULL)
    {
        *imageL=*tmpL;
        imagePortInLeft.getEnvelope(TSLeft);
        initL=true;
    }

    if(tmpR!=NULL) 
    {
        *imageR=*tmpR;
        imagePortInRight.getEnvelope(TSRight);
        initR=true;
    }

    if(initL && initR )
    {
        if (init)
        {
            mutex->wait();
            imgLNext= (IplImage*) imageL->getIplImage();
            imgRNext= (IplImage*) imageR->getIplImage();
            imgLPrev=NULL;
            imgRPrev=NULL;
            width=imgLNext->width;
            height=imgLNext->height;

            outDisp=cvCreateImage(cvSize(imgLNext->width,imgLNext->height),8,3);
            outOpt=cvCreateImage(cvSize(imgLNext->width,imgLNext->height),8,3);
            maskNew=cvCreateImage(cvSize(imgLNext->width,imgLNext->height),8,1);
            initPosition=cvCreateImage(cvSize(imgLNext->width,imgLNext->height),8,1);
            cvZero(initPosition);
            frameDiff=cvCreateImage(cvSize(imgLNext->width,imgLNext->height),8,1);
            depthToDisplay.resize(imgLNext->width,imgLNext->height);

            Mat tmpLNext(imgLNext);
            Mat tmpRNext(imgRNext);
            disp->setImages(tmpLNext,tmpRNext);
            while (!disp->checkDone())
                Time::delay(0.01);

            disp->getDisparity(dispNew);
            IplImage disparity=dispNew;
            thresholdBW(&disparity, maskNew, threshold,0,0,value);
            Mat tmp(maskNew);
            sceneT->setImage(tmp);
            sceneT->start();
            while (!sceneT->checkDone())
                Time::delay(0.01);

            init=false;
            initL=initR=false;

            if (imgLPrev!=NULL)
                cvReleaseImage(&imgLPrev);
            if (imgRPrev!=NULL)
                cvReleaseImage(&imgRPrev);

            imgLPrev=(IplImage*)cvClone(imgLNext);
            imgRPrev=(IplImage*)cvClone(imgRNext);
            mutex->post();
        }
        else
        {
            mutex->wait();
            imgLNext= (IplImage*) imageL->getIplImage();
            imgRNext= (IplImage*) imageR->getIplImage();

            cvSmooth(imgLNext, imgLNext, CV_GAUSSIAN, 3, 0, 0, 0);
            cvSmooth(imgRNext, imgRNext, CV_GAUSSIAN, 3, 0, 0, 0);

            Mat tmpLNext(imgLNext);
            Mat tmpRNext(imgRNext);
            Mat tmpLPrev(imgLPrev);
            
            disp->setImages(tmpLNext,tmpRNext);
            opt->setImages(tmpLPrev,tmpLNext);
            opt->resume();

            if(disp==NULL || opt==NULL)
            {
                mutex->post();
            	return true;
            }
            	
            while (!disp->checkDone()||!opt->checkDone())
              Time::delay(0.01);
            
            disp->getDisparity(dispNew);
            IplImage disparity=dispNew;
            thresholdBW(&disparity, maskNew, threshold,0,0,value);
            cvAbsDiff(initPosition,maskNew,frameDiff);
            cvThreshold(frameDiff,frameDiff,10,1,CV_THRESH_BINARY);
            CvScalar result=cvSum(frameDiff);
            if (sumInitialFrame==0)
                sumFrameDiff=0;
            else
                sumFrameDiff=result.val[0]/sumInitialFrame;

            if (tmpMaskNew!=NULL)
                cvReleaseImage(&tmpMaskNew);
            tmpMaskNew=(IplImage*)cvClone(maskNew);

            cvCvtColor(maskNew,outDisp,CV_GRAY2BGR);
            if (outImage.getOutputCount()>0)
            {
                depthToDisplay.wrapIplImage(outDisp);
                outImage.prepare()=depthToDisplay;
                outImage.write();
            }

            Mat tmp(tmpMaskNew);
            sceneT->setImage(tmp);
            sceneT->resume();
            hogT->setImage(tmp);
            hogT->resume();

            while (!sceneT->checkDone() || !hogT->checkDone())
              Time::delay(0.01);
            if (useDictionary)
            {
                flowHist=sceneT->getCode();
                hogHist=hogT->getCode();
            }
            else
            {
                hogHist.clear();
                flowHist=sceneT->getHist();
                std::vector<yarp::sig::Vector> tmp=hogT->getHist();
                for (size_t i=0; i<tmp.size(); i++)
                {
                    for (size_t j=0; j<tmp[i].size(); j++)
                    {
                        hogHist.push_back(tmp[i][j]);
                    }
                }
            }
            
            int idx=0;
            for (unsigned int i=0; i<flowHist.size(); i++)
            {
                frameFeatures[idx]=flowHist[i];
                idx++;
            }

            for (unsigned int i=0; i<hogHist.size(); i++)
            {
                frameFeatures[idx]=hogHist[i];
                idx++;
            }

            if(bufferCount>SVMBuffer)
                 featuresBuffer.pop_front();
            featuresBuffer.push_back(frameFeatures);
            bufferCount++;

            /*cvShowImage("disparity",maskNew);
            cvWaitKey(0);*/

            if(bufferCount>SVMBuffer)
            {
                scores.clear();
                yarp::sig::Vector tmp(featuresBuffer[0].size());
                tmp=0.0;
                Bottle fea;

                for (unsigned int i=0; i<featuresBuffer.size(); i++)
                {
                    for (unsigned int k=0; k<featuresBuffer[i].size(); k++)
                        tmp[k]+=featuresBuffer[i][k];
                }

                tmp/=featuresBuffer.size();

                for (unsigned int i=0; i<tmp.size(); i++)
                    fea.addDouble(tmp[i]);

                featureOutput.write(fea);

                if (state==STATE_RECOGNITION && scoresInput.getInputCount()>0)
                {
                    Bottle class_scores;
                    scoresInput.read(class_scores);
                    scores.clear();
                    retrieveOrderedScores(class_scores);
                    
                    int start=0;
                    int end=0;
                    int index=recognizer.recognize(scores,sumFrameDiff,start,end);

                    if (index!=-1)
                    {
                        Bottle winner;
                        winner.addInt(index+1);
                        out.write(winner);
                        //cout << "Action " << index+1  << " recognized" << " starting frame: " << start << " end frame: " << end  << endl;
                        printf("%i\n", index+1);

                        Bottle winnerString;
                        
                        if (index==0)
                            winnerString.addString("one");
                        if (index==1)
                            winnerString.addString("two");
                        if (index==2)
                            winnerString.addString("three");
                        if (index==3)
                            winnerString.addString("four");
                        if (index==4)
                            winnerString.addString("five");
                        if (index==5)
                            winnerString.addString("six");

                        outSpeak.write(winnerString);
                    }
                }
                else if (state==STATE_WRITE)
                {
                    std::vector<yarp::sig::Vector> hoghist=hogT->getHist();
                    yarp::sig::Vector flowhist=sceneT->getHist();
                    for (unsigned int i=0; i<hoghist.size(); i++)
                        saveFeatures(featuresHOGFile,hoghist[i]);
                    saveFeatures(featuresHOFFile,flowhist);
                    frameNumber++;
                }
            }

            if (imgLPrev!=NULL)
                cvReleaseImage(&imgLPrev);
            if (imgRPrev!=NULL)
                cvReleaseImage(&imgRPrev);

            imgLPrev=(IplImage*)cvClone(imgLNext);
            imgRPrev=(IplImage*)cvClone(imgRNext);
            mutex->post();
        }
    }

    return true;
}

double GestRecognition::getPeriod()
{
    return 0.001;
}

bool GestRecognition::respond(const Bottle& cmd, Bottle& reply) 
{
    switch (cmd.get(0).asVocab())
    {
        case VOCAB_CMD_UPDATE_INITIAL:
        {
            state=STATE_DONOTHING;
            Bottle cmdTr;
            cmdTr.addString("stop");
            Bottle trReply;
            rpcClassifier.write(cmdTr,trReply);
            reply.addVocab(VOCAB_CMD_ACK);
            mutex->wait();
            disp->getDisparity(dispNew);
            IplImage disparity=dispNew;
            thresholdBW(&disparity, maskNew, threshold,0,0,value);
            if (initPosition!=NULL)
                cvReleaseImage(&initPosition);
            initPosition=(IplImage*)cvClone(maskNew);
            hogT->setInitialPositionFrame(initPosition);
            IplImage* tmp=cvCreateImage(cvSize(initPosition->width,initPosition->height),8,1);
            cvThreshold(initPosition,tmp,0,1,CV_THRESH_BINARY);
            CvScalar result=cvSum(tmp);
            sumInitialFrame=result.val[0];
            cvReleaseImage(&tmp);
            state=STATE_DONOTHING;
            mutex->post();
            return true;
        }
        case VOCAB_CMD_REC:
        {
            mutex->wait();
            reply.addVocab(VOCAB_CMD_ACK);
            Bottle cmdClass;
            cmdClass.addString("recognize");
            Bottle classReply;
            rpcClassifier.write(cmdClass,classReply); 
            if (classReply.toString()=="ack")
            {
                recognizer.resetScores();      
                state=STATE_RECOGNITION;
            }
            mutex->post();
            return true;
        }
        case VOCAB_CMD_SET:
        {
            mutex->wait();
            if (cmd.get(1).asString()=="threshold")
                this->threshold=cmd.get(2).asInt();
            else if (cmd.get(1).asString()=="value")
                this->value=cmd.get(2).asInt();
            reply.addVocab(VOCAB_CMD_ACK);
            mutex->post();
            return true;
        }
        //Send train to the linear classifier
        case VOCAB_CMD_TRAIN:
        {
            mutex->wait();
            Bottle cmdTr;
            cmdTr.addString("train");
            Bottle trReply;
            rpcClassifier.write(cmdTr,trReply);
            reply.addVocab(VOCAB_CMD_ACK);
            state=STATE_DONOTHING;
            mutex->post();
            return true;
        }
        //Save features on linearClassifier
        case VOCAB_CMD_SAVE:
        {
            mutex->wait();
            Bottle cmdClass;
            cmdClass.addString("save");
            cmdClass.addString(cmd.get(1).asString().c_str());
            Bottle classReply;
            rpcClassifier.write(cmdClass,classReply);
            reply.addVocab(VOCAB_CMD_ACK);
            state=STATE_DONOTHING;
            mutex->post();
            return true;
        }
        //Stop writing on files
        case VOCAB_CMD_STOP:
        {
            mutex->wait();
            frameNumber=1;
            if (featuresHOGFile.is_open())
                featuresHOGFile.close();
            if (featuresHOFFile.is_open())
                featuresHOFFile.close();
            Bottle cmdTr;
            cmdTr.addString("stop");
            Bottle trReply;
            rpcClassifier.write(cmdTr,trReply);            
            reply.addVocab(VOCAB_CMD_ACK);
            state=STATE_DONOTHING;
            mutex->post();
            return true;
        }
        //Write on files
        case VOCAB_CMD_WRITE:
        {
            mutex->wait();
            frameNumber=1;
            stringstream o;
            o << countFile;
            countFile=countFile+1;
            string s = o.str();
            string filenameHOG=outDir+"hog"+s+".txt";
            featuresHOGFile.open(filenameHOG.c_str(), ios_base::trunc); 
            string filenameHOF=outDir+"hof"+s+".txt";
            featuresHOFFile.open(filenameHOF.c_str(), ios_base::trunc); 
            state=STATE_WRITE;
            reply.addVocab(VOCAB_CMD_ACK);
            mutex->post();
            return true;
        }
    }
    reply.addVocab(VOCAB_CMD_NACK);
    return false;
}

void GestRecognition::saveFeatures(ofstream &featuresFile, yarp::sig::Vector &features) 
{
    featuresFile << frameNumber << " " << sumFrameDiff;
    for(unsigned int i=0; i<features.size(); i++) 
        featuresFile << " " << features[i];

    featuresFile << "\n";
}

void GestRecognition::thresholdBW(IplImage *img, IplImage* res, int threshold, int x, int y, double value)
{
    cvZero(res);
    CvSize size =cvGetSize(img);

    CvScalar s;
    if(value==0)
    {
        if(x<=0 || y<=0 || x>size.width || y>size.height) 
        {
            int centerW= (int) size.width/2;
            int centerH= (int) size.height/2;
            s = cvGet2D(img,centerH,centerW);
        } 
        else 
        {
            s = cvGet2D(img,y,x);
        }
    }
    else 
    {
        s.val[0]=value;
        s.val[1]=value;
        s.val[2]=value;
    }

    IplImage* tmpInf=cvCreateImage(cvGetSize(res),res->depth,res->nChannels);
    IplImage* tmpSup=cvCreateImage(cvGetSize(res),res->depth,res->nChannels);
    cvZero(tmpInf);
    cvZero(tmpSup);
    cvThreshold(img, tmpInf, s.val[0]+threshold, 255, CV_THRESH_BINARY_INV);
    cvThreshold(img, tmpSup, s.val[0]-threshold, 255, CV_THRESH_BINARY);
    cvAnd(tmpInf,tmpSup,tmpSup);
    cvAnd(img,tmpSup,res);

    //cvThreshold(img, res, s.val[0]-threshold, 255, CV_THRESH_TOZERO);
    cvReleaseImage(&tmpInf);
    cvReleaseImage(&tmpSup);


    for (int i=0; i<res->width; i++)
    {          
        for (int j=0; j<res->height; j++)
        {
            if (i<30 || i>res->width-30 || j<30 || j>res->height-30)
                cvSet2D(res,j,i,cvScalar(0,0,0,0));
        }
    }
}

void GestRecognition::retrieveOrderedScores(Bottle &class_scores)
{
    scores.resize(class_scores.size(),0.0);
    for (int i=0; i<class_scores.size(); i++)
    {
        int index;
        Bottle* tmp=class_scores.get(i).asList();
        string name=tmp->get(0).asString().c_str();
        name=name.substr(name.size()-1,1);
        std::stringstream ss(name);
        ss >> index;
        scores[index-1]=tmp->get(1).asDouble();
    }
}
