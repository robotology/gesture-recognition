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

#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>

#include <yarp/os/Thread.h>
#include <yarp/math/Math.h>

#include <iCub/ctrl/filters.h>

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
