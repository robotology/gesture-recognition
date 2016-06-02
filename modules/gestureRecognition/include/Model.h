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
#include <cstdio>
#include <vector>
#include <iostream>
#include <fstream>

#include <yarp/sig/Vector.h>

class Model
{

public:

    virtual void computeOutput(std::deque<yarp::sig::Vector> &features, yarp::sig::Vector &output)=0;
    virtual void trainModel() {};
    virtual int getNumActions()=0;
    virtual bool read()=0;
    virtual void write()=0;
    virtual void printModel()=0;
    virtual ~Model() {};

};

