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

#include "Recognizer.h"

using namespace std;
using namespace yarp::sig;
using namespace yarp::os;
using namespace yarp::math;
using namespace iCub::ctrl;

int Recognizer::getIndex()
{
    int index=-1;
    if (scores.size()>0)
    {
        Vector sum(scores.at(0).size()); sum=0.0;
        for (unsigned int i=0; i<scores.size(); i++)
            sum=sum+scores.at(i);
        double max=-100000000;
        for (unsigned int j=0; j<sum.size(); j++)
        {
            if (sum[j]>max)
            {
                index=j;
                max=sum[j];
            }
        }
    }
    return index;
}

int Recognizer::recognize(const Vector& out, double sumFrameDiff, int& start, int& end)
{
     if (init)
    {
        init=false;
        time++;
        start=time;

        return -1;
    }

    Vector ones(out.size()); ones=1.0;
    double variance=sumFrameDiff;

    if (time>=buffer)
    {
        Vector tmp=variances.subVector(1,variances.size()-1);
        tmp.push_back(variance);
        variances=tmp;
        Vector ones2(buffer); ones2=1.0;

        double mvar=dot(variances,ones2)/buffer;
        int index=getIndex();

        /*Vector aa;
        aa.push_back(variance);
        aa.push_back(mvar);
        saveFilteredScores(aa,path2);*/

        //if the variance is greater than the mean, rec is false
        if (!rec)
        {
            //if variance has just gone under the mean recognize and
            //clear the score, set rec true
            if (variances[buffer-1]<(mvar-0.1))
            {
                end=time-buffer;
                start=this->start;
                rec=true;
                int index=getIndex();
                scores.clear();
                
                time++;
                return index;
            }
            //accumulate the scores
            else
            {
                scores.push_back(out);
                time++;
                return -1;
            }
        }
        //if the variance is lower than the mean, rec is true
        else
        {
            //if the variance is greater than the mean, accumulate the first
            //score and set rec equal to false, saving the start time
            if (variances[buffer-1]>mvar)
            {
                scores.push_back(out);
                this->start=time-buffer;
                rec=false;
            }
            //otherwise don't do anything
            time++;
            return -1;
        }
    }
    //if there are no values of variance enough, just 
    //save the variances
    else
    {
        /*Vector aa;
        aa.push_back(variance);
        aa.push_back(0);
        saveFilteredScores(aa,path2);*/
        variances[time]=variance;
        time++;
        return -1;
    }
}

void Recognizer::resetScores()
{
    scores.clear();
}

Recognizer::~Recognizer()
{

}

Recognizer::Recognizer()
{
    buffer=30;
    time=0;
    start=time;
    rec=false;
    variances.resize(buffer,0.0);
    init=true;
}

void Recognizer::saveFilteredScores(const yarp::sig::Vector &scores, string &path)
{
    ofstream outFile(path.c_str(), ios_base::app);

    for (unsigned int i=0; i<scores.size(); i++)
    {
        outFile << scores[i] <<  " ";
    }
    outFile << endl;
    outFile.close();
}

