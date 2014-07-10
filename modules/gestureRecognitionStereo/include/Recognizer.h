#include <string>
#include <vector>
#include <cmath>
#include <yarp/os/Thread.h>
#include <yarp/math/Math.h>
#include <iCub/ctrl/filters.h>
#include <iostream>
#include <fstream>
class Recognizer 
{
    std::vector<yarp::sig::Vector> scores;
    yarp::sig::Vector variances;
    int buffer;
    int time;
    int start;
    bool init;
    bool rec;

    int getIndex();

public:
    Recognizer();
    int recognize(const yarp::sig::Vector& u, double sumFrameDiff, int& start, int& end);
    void resetScores();
    void saveFilteredScores(const yarp::sig::Vector &scores, std::string &path);
    ~Recognizer();
};
