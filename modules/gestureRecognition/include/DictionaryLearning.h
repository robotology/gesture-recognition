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

#include <deque>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <yarp/sig/Vector.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Property.h>
#include <yarp/math/Math.h>

class DictionaryLearning
{
    std::string dictionariesFilePath;
    yarp::sig::Matrix dictionary;
    yarp::sig::Matrix A;

    std::string group;

    int dictionarySize;
    int featuresize;
    double lamda;

    void subMatrix(const yarp::sig::Matrix& A, const yarp::sig::Vector& indexes, yarp::sig::Matrix& Atmp);
    void max(const yarp::sig::Vector& x, double& maxVal, int& index);
    bool read();
    void printMatrixYarp(const yarp::sig::Matrix& A);

public:

    DictionaryLearning(std::string dictionariesFilePath, std::string group);
    void computeDictionary(const yarp::sig::Vector& feature, yarp::sig::Vector& descriptor);
    void learnDictionary();
    ~DictionaryLearning();
};

