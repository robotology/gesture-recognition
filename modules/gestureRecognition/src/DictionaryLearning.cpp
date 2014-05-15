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

#include "DictionaryLearning.h"

using namespace std;
using namespace yarp::sig;
using namespace yarp::os;
using namespace yarp::math;

DictionaryLearning::DictionaryLearning(string dictionariesFilePath, string group)
{
    this->dictionariesFilePath=dictionariesFilePath;
    this->group=group;
    read();
}

void DictionaryLearning::computeDictionary(const yarp::sig::Vector& feature, yarp::sig::Vector& descriptor)
{
    double eps=1e-7;
    Vector b=(-1)*dictionary.transposed()*feature;

    Vector grad=b;
    descriptor.resize(dictionarySize,0.0);
    double maxValue; int index=-1;
    max(grad,maxValue,index);

    while(true)
    {
        if (grad[index]>lamda+eps)
            descriptor[index]=(lamda-grad[index])/A(index,index);
        else if (grad[index]<-lamda-eps)
            descriptor[index]=(-lamda-grad[index])/A(index,index);
        else
        {
            double sum=0.0;
            for (size_t j=0; j<descriptor.size(); j++)
                sum+=descriptor[j];
            if (sum==0.0)
                break;
        }

        while(true)
        {
            Matrix Aa;
            Vector ba;
            Vector xa;
            Vector a;
            Vector signes;

            for (size_t i=0; i<descriptor.size(); i++)
            {
                if (descriptor[i]!=0.0)
                {
                    a.push_back(i);
                    ba.push_back(b[i]);
                    xa.push_back(descriptor[i]);
                    if (descriptor[i]>0)
                        signes.push_back(1);
                    else
                        signes.push_back(-1);
                }
            }

            subMatrix(A,a,Aa);

            Vector vect=(-lamda*signes)-ba;
            Vector xnew=luinv(Aa)*vect;

            Vector vectidx;
            Vector bidx;
            Vector xnewidx;

            double sum=0.0;
            for (size_t i=0; i<xnew.size(); i++)
            {
                if (xnew[i]!=0.0)
                {
                    vectidx.push_back(vect[i]);
                    bidx.push_back(ba[i]);
                    xnewidx.push_back(xnew[i]);
                    sum+=abs(xnew[i]);
                }
            }

            double onew=dot((vectidx/2+bidx),xnewidx)+lamda*sum;

            Vector s;
            bool changeSign=false;
            for (size_t i=0; i<xa.size(); i++)
            {
                if (xa[i]*xnew[i]<=0)
                {
                    changeSign=true;
                    s.push_back(i);
                }
            }

            if (!changeSign)
            {
                for (size_t i=0; i<a.size(); i++)
                    descriptor[(size_t)a[i]]=xnew[i];
                break;
            }

            Vector xmin=xnew;
            double omin=onew;

            Vector d=xnew-xa;
            Vector t=d/xa;

            for (size_t i=0; i<s.size(); i++)
            {
                Vector x_s=xa-d/t[(size_t)s[i]];
                x_s[(size_t)s[i]]=0.0;

                Vector x_sidx;
                Vector baidx;
                Vector idx;
                sum=0.0;
                for (size_t j=0; j<x_s.size(); j++)
                {
                    if (x_s[j]!=0)
                    {
                        idx.push_back(j);
                        sum+=abs(x_s[j]);
                        x_sidx.push_back(x_s[j]);
                        baidx.push_back(ba[j]);
                    }
                }
                Matrix Atmp2;
                subMatrix(Aa,idx,Atmp2);

                Vector otmp=Atmp2*(x_sidx/2)+baidx;
                double o_s=dot(otmp,x_sidx)+lamda*sum;

                if (o_s<omin)
                {
                    xmin=x_s;
                    omin=o_s;
                }
            }

            for (size_t i=0; i<a.size(); i++)
                descriptor[(size_t)a[i]]=xmin[i];
        }
        grad=A*descriptor+b;

        Vector tmp;

        for (size_t i=0; i<descriptor.size(); i++)
        {
            if (descriptor[i]==0.0)
                tmp.push_back(grad[i]);
            else
                tmp.push_back(0);
        }
        max(tmp,maxValue,index);

        if (maxValue<=lamda+eps)
            break;
    }
}

void DictionaryLearning::subMatrix(const Matrix& A, const Vector& indexes, Matrix& Atmp)
{
    Atmp.resize(indexes.size(),indexes.size());
    int m=0;
    int n=0;
    for (size_t i=0; i<indexes.size(); i++)
    {
        for (size_t j=0; j<indexes.size(); j++)
        {
            Atmp(m,n)=A((int)indexes[i],(int)indexes[j]);
            n++;
        }
        m++;
        n=0;
    }
}

void DictionaryLearning::max(const Vector& x, double& maxValue, int& index)
{
    maxValue=-1000;
    for (size_t i=0; i<x.size(); i++)
    {
        if(abs((double)x[i])>maxValue)
        {
            maxValue=abs(x[i]);
            index=i;
        }
    }
}

void DictionaryLearning::learnDictionary()
{

}

bool DictionaryLearning::read()
{
    Property config; config.fromConfigFile(dictionariesFilePath.c_str());
    Bottle& bgeneral=config.findGroup("GENERAL");
    lamda=bgeneral.find("lambda").asDouble();
    dictionarySize=bgeneral.find("dictionarySize").asInt();
    Bottle& dictionaryGroup=config.findGroup(group.c_str());
    featuresize=dictionaryGroup.find("featureSize").asInt();
    dictionary.resize(featuresize,dictionarySize);

    for (int i=0; i<featuresize; i++)
    {
        ostringstream oss;
        oss << (i+1);
        string num = "line"+oss.str();
        Bottle* line=dictionaryGroup.find(num.c_str()).asList();
        
        for (int j=0; j<dictionarySize; j++)
            dictionary(i,j)=line->get(j).asDouble();
    }

    double beta=1e-4;
    Matrix trans=dictionary.transposed();
    Matrix identity=eye(dictionarySize,dictionarySize);
    Matrix diagonal=2*beta*identity;
    A=trans*dictionary+diagonal;

    return true;
}
void DictionaryLearning::printMatrixYarp(const yarp::sig::Matrix &A) 
{
    cout << endl;
    for (int i=0; i<A.rows(); i++) 
        for (int j=0; j<A.cols(); j++) 
            cout<<A(i,j)<<" ";
        cout<<endl;
    cout << endl;
}


DictionaryLearning::~DictionaryLearning()
{
    //write();
}


