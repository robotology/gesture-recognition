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

#ifndef __DICTIONARYLEARNING__
#define __DICTIONARYLEARNING__

#include <deque>
#include <cstdio>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

#include <yarp/os/Time.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Property.h>
#include <yarp/sig/Vector.h>
#include <yarp/math/Math.h>

class DictionaryLearning
{
    std::string dictionariesFilePath;
    yarp::sig::Matrix dictionary;
    yarp::sig::Matrix A;
    std::vector<yarp::sig::Vector>  dictionaryBOW;
    float                          *f_dictionaryBOW;
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

