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
#include <yarp/sig/Vector.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Property.h>
#include <yarp/math/Math.h>
#include <yarp/os/ResourceFinder.h>
#include "Model.h"

class ModelSVM: public Model
{
    std::string actionsFilePath;
    std::deque<yarp::sig::Vector> models;
    int numActions;
    int dictionarySize;
    int buffer;
    int nFeatures;
    int frameFeatureSize;

public:

    ModelSVM(std::string actionsFilePath);
    void computeOutput(std::deque<yarp::sig::Vector> &features, yarp::sig::Vector &output);
    void concatenate(std::deque<yarp::sig::Vector> &features, yarp::sig::Vector &descriptor);
    void trainModel();
    int getNumActions();
    bool read();
    void write();
    void printModel();
    ~ModelSVM();
};

