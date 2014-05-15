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

#include <Recognizer.h>

using namespace std;
using namespace yarp::sig;
using namespace yarp::os;
using namespace yarp::math;
using namespace iCub::ctrl;
string path1="";
string path2="";
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

int Recognizer::recognize(const Vector& u, int& start, int& end)
{
    if (init)
    {
        init=false;
        time++;
        start=time;

        return -1;
    }

    Vector out=u;

    Vector ones(out.size()); ones=1.0;
    double mean=dot(out,ones)/out.size();
    double variance=0.0;
    
    for (size_t k=0; k<out.size(); k++)
        variance+=(out[k]-mean)*(out[k]-mean);

    variance=sqrt(variance/(out.size()-1));

    if (time>=buffer)
    {
        Vector tmp=variances.subVector(1,variances.size()-1);
        tmp.push_back(variance);
        variances=tmp;
        Vector ones2(buffer); ones2=1.0;

        double mvar=dot(variances,ones2)/buffer;

        //if the variance is greater than the mean, rec is false
        if (!rec)
        {
            //if variance has just gone under the mean recognize and
            //clear the score, set rec true
            if (variances[buffer-1]<(mvar-0.2))
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
        variances[time]=variance;
        time++;
        return -1;
    }
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

