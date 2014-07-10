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

