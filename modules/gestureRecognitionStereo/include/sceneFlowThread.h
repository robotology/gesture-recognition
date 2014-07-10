
#include <iCub/iKin/iKinFwd.h>
#include <yarp/os/all.h>
#include "iCub/stereoVision/disparityThread.h"
#include "iCub/stereoVision/opticalFlowThread.h"
#include"DictionaryLearning.h"

#include <yarp/sig/Matrix.h>
#include <yarp/sig/Image.h>
#include <yarp/os/Stamp.h>

using namespace std; 
using namespace yarp::os; 
using namespace yarp::sig;
using namespace yarp::dev;
using namespace yarp::math;
using namespace iCub::ctrl;
using namespace iCub::iKin;

class SceneFlowThread : public RateThread
{
private:
    cv::Mat disparity;

    bool success;
    bool done;
    bool work;
    bool useDictionary;
    DisparityThread* disp;
    OpticalFlowThread* opt;
    yarp::sig::Vector flowHist;
    yarp::sig::Vector code;

    std::vector<double> edges;
    int dimension;
    double minimum;
    double maximum;
    DictionaryLearning *sparse_coder;

    Mat optFlow;
    Mat points3D;
    Mat points3Dold;

    bool init;

    void get3DPoints(Mat &points);
    void triangulate(Point2f &pixel,Point3f &point);
    void printMatrix(Mat &matrix);
    void computeEdges();
    void generateMatIndex(double num, int &index);
    void computeFlowCode(yarp::sig::Vector& flowHist,yarp::sig::Vector& code);
    void computeFlowHistogram(const vector<Point3f>& flow3D, yarp::sig::Vector& flowHist);
    void drawFlowModule(IplImage* imgMotion);
public:

    SceneFlowThread(yarp::os::ResourceFinder &rf, DisparityThread* d, OpticalFlowThread* o);
    ~SceneFlowThread(void) {};

    bool isOpen();

    bool threadInit(void);
    void run(void); 
    void onStop(void);
    void close();
    void getSceneFlow(vector<Point3f> &flow3D);
    void setImage(cv::Mat &disparity);
    void threadRelease(); 
    bool checkDone();
    yarp::sig::Vector getHist();
    yarp::sig::Vector getCode();
    Point3f getSceneFlowPixel(int u, int v);    
    IplImage* draw2DMotionField();

};
