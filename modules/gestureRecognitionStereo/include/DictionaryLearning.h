#include <deque>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <yarp/sig/Vector.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Property.h>
#include <yarp/math/Math.h>
#include <yarp/os/Time.h>

#ifndef __DICTIONARYLEARNING__
#define __DICTIONARYLEARNING__


class DictionaryLearning
{
    std::string dictionariesFilePath;
    yarp::sig::Matrix dictionary;
    yarp::sig::Matrix A;
    std::vector<yarp::sig::Vector>  dictionaryBOW;
    float                           *f_dictionaryBOW;
    std::string group;

    int dictionarySize;
    int featuresize;
    double lamda;

    void subMatrix(const yarp::sig::Matrix& A, const yarp::sig::Vector& indexes, yarp::sig::Matrix& Atmp);
    void max(const yarp::sig::Vector& x, double& maxVal, int& index);
    bool read();
    void printMatrixYarp(const yarp::sig::Matrix& A);
    double customNorm(const yarp::sig::Vector &A,const yarp::sig::Vector &B);

public:

    DictionaryLearning(std::string dictionariesFilePath, std::string group);
    void computeCode(const yarp::sig::Vector& feature, yarp::sig::Vector& descriptor);
    void learnDictionary();
    void bow(std::vector<yarp::sig::Vector> & features, yarp::sig::Vector & code);
    void f_bow(std::vector<yarp::sig::Vector> & features, yarp::sig::Vector & code);
    ~DictionaryLearning();
};

#endif

