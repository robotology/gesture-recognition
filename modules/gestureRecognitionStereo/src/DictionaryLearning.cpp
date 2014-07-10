#include "DictionaryLearning.h"

using namespace std;
using namespace yarp::sig;
using namespace yarp::os;
using namespace yarp::math;



/*float sse3_inner(const float* a, const float* b, unsigned int size)
{
        float z = 0.0f, fres = 0.0f;
        
        if ((size / 4) != 0) {
                const float* pa = a;
                const float* pb = b;
                /*__asm {
                        movss   xmm0, xmmword ptr[z]
                }
                asm("movss   %xmm0, xmmword ptr[z];");
                for (unsigned int i = 0; i < size / 4; i++) {
                        /*__asm {
                                mov     eax, dword ptr[pa]
                                mov     ebx, dword ptr[pb]
                                movups  xmm1, [eax]
                                movups  xmm2, [ebx]
                                mulps   xmm1, xmm2
                                addps   xmm0, xmm1
                        }
                        pa += 4;
                        pb += 4;
                }  
                /*__asm {
                        haddps  xmm0, xmm0
                        haddps  xmm0, xmm0
                        movss   dword ptr[fres], xmm0                        
                }              
        }

        return fres;
}*/



DictionaryLearning::DictionaryLearning(string dictionariesFilePath, string group)
{
    this->dictionariesFilePath=dictionariesFilePath;
    this->group=group;
    read();
}


double DictionaryLearning::customNorm(const yarp::sig::Vector &A,const yarp::sig::Vector &B)
{
       double norm=0;

       for (int i=0; i<128; i++)
       {
           double d=A[i]-B[i];
           norm+=d*d;
           
       }    
       
       
       return norm;

}

void DictionaryLearning::f_bow(std::vector<yarp::sig::Vector> & features, yarp::sig::Vector & code)
{
     fprintf(stdout, "Starting bow coding \n");
     double time=Time::now();
     


     code.resize(dictionarySize);
     code=0.0;

     int n_features=features.size();
     for (int feat_idx=0; feat_idx<n_features; feat_idx++)
     {
         float minNorm=(float)1e20;
         int winnerAtom;
         
         //Vector &A=features[feat_idx];
         double *A2 = features[feat_idx].data(); 

         //double time_atom=Time::now();
         for(int atom_idx=0; atom_idx<dictionarySize; atom_idx++)
         {
             //Vector &B=dictionaryBOW[atom_idx];  
             double *B2 = dictionaryBOW[atom_idx].data();        
             //double time_norm=Time::now();
             double norm=0.0;
             for (int i=0; i<featuresize; i++)
             {
                 //double d=A[i]-B[i];
                 double d=A2[i]-B2[i];
                 norm+=d*d;
                                  
                 if (norm > minNorm) 
                     goto ciao;
           
             }                

             if(norm<minNorm)
             {
                 minNorm=norm;
                 winnerAtom=atom_idx;
             }                 
             
             ciao:{}               
                 //time_norm=Time::now()-time_norm;
                 //fprintf(stdout, "norm %f \n",time_norm);
         }
         
         //time_atom=Time::now()-time_atom;
         //fprintf(stdout, "%f \n",time_atom);
         code[winnerAtom]++;
     }
     
     code=code/yarp::math::norm(code);
     time=Time::now()-time;
     fprintf(stdout, "%f \n",time);


}


void DictionaryLearning::bow(std::vector<yarp::sig::Vector> & features, yarp::sig::Vector & code)
{
    //transform to array of float
    int n_features=features.size();
    float *f_features=new float[n_features*featuresize];

    for (int feat_idx=0; feat_idx<n_features; feat_idx++)
    {
        float *tmp_f=f_features+feat_idx*featuresize;
        for (int i=0; i<featuresize; i++)
        {
            tmp_f[i]=features[feat_idx][i];
        } 
    }       

     fprintf(stdout, "Starting bow coding \n");
     double time=Time::now();
     


     code.resize(dictionarySize);
     code=0.0;

     for (int feat_idx=0; feat_idx<n_features; feat_idx++)
     {
         float minNorm=(float)1e20;
         int winnerAtom;
         
         //Vector &A=features[feat_idx];
         float *A2 = f_features+feat_idx*featuresize; 

         //double time_atom=Time::now();
         for(int atom_idx=0; atom_idx<dictionarySize; atom_idx++)
         {
             //Vector &B=dictionaryBOW[atom_idx];  
             float *B2 = f_dictionaryBOW+atom_idx*featuresize;        
             //double time_norm=Time::now();
             float norm=0.0f;
             //double norm2=0.0;
             for (int i=0; i<featuresize; i++)
             {
                 //double d2=A[i]-B[i];
                 float d=A2[i]-B2[i];
                 norm+=d*d;
                 //norm2+=d2*d2;
                 
                 //fprintf(stdout,"%.10f\n",d-d2);
                                  
                 if (norm > minNorm) 
                     goto ciao;
           
             }             
             //float  norm= sse3_inner(A2, B2, featuresize);
             if(norm<minNorm)
             {
                 minNorm=(float)norm;
                 winnerAtom=atom_idx;
             }                 
             
             ciao:{}               
                 //time_norm=Time::now()-time_norm;
                 //fprintf(stdout, "norm %f \n",time_norm);
         }
         
         //time_atom=Time::now()-time_atom;
         //fprintf(stdout, "%f \n",time_atom);
         code[winnerAtom]++;
     }
     
     code=code/yarp::math::norm(code);
     time=Time::now()-time;
     fprintf(stdout, "%f \n",time);
     
     delete [] f_features;


}

void DictionaryLearning::computeCode(const yarp::sig::Vector& feature, yarp::sig::Vector& descriptor)
{
    double eps=1e-7;
    Vector b=(-1)*feature*dictionary;

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
            for (unsigned int j=0; j<descriptor.size(); j++)
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

            for (unsigned int i=0; i<descriptor.size(); i++)
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
            for (unsigned int i=0; i<xnew.size(); i++)
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
            for (unsigned int i=0; i<xa.size(); i++)
            {
                if (xa[i]*xnew[i]<=0)
                {
                    changeSign=true;
                    s.push_back(i);
                }
            }

            if (!changeSign)
            {
                for (unsigned int i=0; i<a.size(); i++)
                    descriptor[a[i]]=xnew[i];
                break;
            }

            Vector xmin=xnew;
            double omin=onew;

            Vector d=xnew-xa;
            Vector t=d/xa;

            for (unsigned int i=0; i<s.size(); i++)
            {
                Vector x_s=xa-d/t[s[i]];
                x_s[s[i]]=0.0;

                Vector x_sidx;
                Vector baidx;
                Vector idx;
                sum=0.0;
                for (unsigned int j=0; j<x_s.size(); j++)
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

            for (unsigned int i=0; i<a.size(); i++)
                descriptor[a[i]]=xmin[i];
        }
        //grad=A*descriptor+b;
		
		// We can perform this operation since the matrix A is symmetric! 10 times faster
        grad=descriptor;
        grad*=A;
        grad+=b;

        Vector tmp;

        for (unsigned int i=0; i<descriptor.size(); i++)
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
    int indSize=indexes.size();
    Atmp.resize(indSize,indSize);
    int m=0;
    int n=0;
    for (int i=0; i<indSize; i++)
    {
        for (int j=0; j<indSize; j++)
        {
            Atmp(m,n)=A(indexes[i],indexes[j]);
            n++;
        }
        m++;
        n=0;
    }
}

void DictionaryLearning::max(const Vector& x, double& maxValue, int& index)
{
    maxValue=-1000;
    for (unsigned int i=0; i<x.size(); i++)
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

     dictionaryBOW.resize(dictionarySize);
     
     f_dictionaryBOW=new float[dictionarySize*featuresize];     
     for (int i=0; i<dictionarySize; i++)
     {
        dictionaryBOW[i]=dictionary.getCol(i);

        float *tmp_f=f_dictionaryBOW+i*featuresize;
        for (int j=0; j<featuresize; j++)
            tmp_f[j]=dictionaryBOW[i][j];   
     }
     
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
