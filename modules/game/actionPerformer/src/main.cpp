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

/** 
@ingroup robotology
\defgroup actionPerformer actionPerformer

A module that executes gestures specified in a config file as a set of arm joint positions.

\section intro_sec Description 
This module simply allows the robot to perform a set of gestures that can be
specified through a set of arm joint positions as in the file app/conf/actions.ini. 
 
\section rpc_port Commands:
The commands sent as bottles to the module port /<modName>/rpc
are described in the following:

<b>ACTION</b> \n
format: [action] <id> \n
action: the module receives the gesture IDs to be performed and execute them.

<b>STOP</b> \n
format: [stop] \n
action: the module stops executing the sequence.

\section lib_sec Libraries 
- YARP libraries. 

\section portsc_sec Ports Created 

- \e /<modName>/rpc remote procedure call. It always replies something.
- \e /<modName>/out through this port the modules outputs when the execution
  of the sequence has finished.

\section parameters_sec Parameters 
The following are the options that are usually contained 
in the configuration file, which is config.ini:

--name \e name
- specify the module name, which is \e actionPerformer by 
  default.

--robot \e robot
- the robot that should execute the gestures.

--stereo \e stereo
- it is on when the gesture recognition module based on stereo
  is being used.

\section tested_os_sec Tested OS
Windows, Linux

\author Ilaria Gori, Sean Ryan Fanello
**/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <deque>
#include <stdio.h>
#include <stdlib.h>
#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/sig/Vector.h>
#include <yarp/sig/Matrix.h>
#include <yarp/math/Math.h>
#include <yarp/dev/Drivers.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Port.h>
#include <yarp/os/Time.h>
#include <yarp/dev/ControlBoardInterfaces.h>

using namespace std;
using namespace yarp::sig;
using namespace yarp::os;
using namespace yarp::math;

class GestureRecognitionActionPerformer: public RFModule
{
protected:

    struct Action
    {
        std::string tag;
        yarp::sig::Vector qRight;
        yarp::sig::Vector qLeft;
    };

    bool executing;
    bool go;
    int numActions;
    double toll;
    bool closing;

    deque<Action> actions;
    deque<Action> currentSequence;

    yarp::dev::PolyDriver                   polyArmRight;
    yarp::dev::PolyDriver                   polyArmLeft;
    yarp::dev::IPositionControl*            posArmRight;
    yarp::dev::IPositionControl*            posArmLeft;
    yarp::dev::IEncoders*                   encRight;
    yarp::dev::IEncoders*                   encLeft;

    Port rpc;
    Port out;

    bool close()
    {
        if (polyArmRight.isValid())
            polyArmRight.close();
        if (polyArmLeft.isValid())
            polyArmLeft.close();

        rpc.close();

        return true;
    }

    bool interruptModule()
    {
        rpc.interrupt();
        closing=true;
        return true;
    }

    double getPeriod()
    {
        return 0.1;
    }

    bool respond(const Bottle& cmd, Bottle& reply) 
    {
        if (cmd.get(0).asString()=="action")
        {
            if (!executing)
            {
                if (cmd.size()==2)
                {
                    reply.addString("ack");
                    executing=true;
                    int sequenceInt=cmd.get(1).asInt();
                    ostringstream oss;
                    oss << sequenceInt;
                    string sequence = oss.str();
                    accumulate(sequence);
                }
                else
                {
                    reply.addString("no sequence added");
                }
            }
            else
            {
                reply.addString("I'm executing a sequence");
            }
            return true;
        }
        else if (cmd.get(0).asString()=="stop")
        {
            executing=false;
            reply.addString("stopping execution module");
            fprintf(stdout, "Stopping execution module\n");
            return true;
        }
        else
        {
            reply.addString("command not recognized");
            return true;
        }
    }

    bool updateModule()
    {
        if (executing)
        {
            executeActions();
            Bottle done,reply;
            done.addString("done");
            out.write(done,reply);
        }
        return true;
    }

    void accumulate(const string& sequence)
    {
        currentSequence.clear();
        for (unsigned int i=0; i<sequence.size(); i++)
        {
            string tag=sequence.substr(i,1);
            fprintf(stdout, "tag, %s\n", tag.c_str());
            int number=atoi(tag.c_str());
            fprintf(stdout, "number %d\n", number);
            currentSequence.push_back(actions.at(number-1));
        }
    }

    void executeActions()
    {
        fprintf(stdout, "Executing sequence\n");
        for (unsigned int i=0; i<currentSequence.size(); i++)
        {
            Action action=currentSequence.at(i);
            if (executing)
                execute(action);
        }

        executing=false;
    }

    void execute(const Action& action)
    {
        Vector accsA;
        accsA.resize(16, 6000);
        Vector spdsA;
        spdsA.resize(16, 35.0);
        spdsA[9]=40.0;
        spdsA[10]=40.0;
        spdsA[11]=40.0;
        spdsA[12]=55.0;
        spdsA[13]=40.0;
        spdsA[14]=55.0;
        spdsA[15]=110.0;
        //posArmRight->setRefAccelerations(accsA.data());
        posArmRight->setRefSpeeds(spdsA.data());
        posArmRight->positionMove(action.qRight.data());
        //posArmLeft->setRefAccelerations(accsA.data());
        posArmLeft->setRefSpeeds(spdsA.data());
        posArmLeft->positionMove(action.qLeft.data());

        bool right,left;
        right=false;
        left=false;
        double sec=2.8;
        double t=Time::now();
        while (((!right)||(!left))&&!closing)
        {
            Vector vRight(16); vRight=0.0;
            Vector vLeft(16); vLeft=0.0;
            encRight->getEncoders(vRight.data());
            encLeft->getEncoders(vLeft.data());
            double normRight=norm(vRight-action.qRight);
            double normLeft=norm(vLeft-action.qLeft);
            if ((normRight<toll)||(Time::now()-t>sec))
                right=true;
            if ((normLeft<toll)||(Time::now()-t>sec))
                left=true;
        }

        Vector zeros(16); zeros=0.0;
        zeros[0]=-14.0;
        zeros[1]=30.0;
        zeros[3]=15.0;
        zeros[7]=15.0;
        zeros[8]=30.0;
        zeros[9]=3.0;
        zeros[10]=1.0;
        zeros[11]=3.0;
        zeros[13]=2.0;
        zeros[14]=2.0;
        zeros[15]=11.0;

        posArmRight->setRefAccelerations(accsA.data());
        posArmRight->setRefSpeeds(spdsA.data());
        posArmRight->positionMove(zeros.data());
        posArmLeft->setRefAccelerations(accsA.data());
        posArmLeft->setRefSpeeds(spdsA.data());
        posArmLeft->positionMove(zeros.data());

        right=false;
        left=false;
        t=Time::now();
        double secBase=2.3;
        while (((!right)||(!left))&&!closing)
        {
            Vector vRight(16); vRight=0.0;
            Vector vLeft(16); vLeft=0.0;
            encRight->getEncoders(vRight.data());
            encLeft->getEncoders(vLeft.data());
            double normRight=norm(vRight-zeros);
            double normLeft=norm(vLeft-zeros);
            if ((normRight<toll)||(Time::now()-t>secBase))
                right=true;
            if ((normLeft<toll)||(Time::now()-t>secBase))
                left=true;
        }

        Time::delay(0.1);
    }

    bool createDatabase(const string& filename)
    {
        Property config; config.fromConfigFile(filename.c_str());

        Vector qright(16); qright=0.0;
        Vector qleft(16); qleft=0.0;

        Bottle& bgeneral=config.findGroup("general");
        numActions=bgeneral.find("numActions").asInt();

        for (int i=0; i<numActions; i++)
        {
            ostringstream oss;
            oss << (i+1);
            string num = oss.str();
            string actionName="action"+num;

            Bottle& baction=config.findGroup(actionName.c_str());
            Bottle* right=baction.find("right").asList();
            for (int j=0; j<right->size(); j++)
                qright[j]=right->get(j).asDouble();
            Bottle* left=baction.find("left").asList();
            for (int j=0; j<left->size(); j++)
                qleft[j]=left->get(j).asDouble();

            Action action;
            action.tag=actionName;
            action.qRight=qright;
            action.qLeft=qleft;

            actions.push_back(action);
        }

        return true;
    }

public:
    bool configure(ResourceFinder &rf)
    {
        executing=false;
        go=false;
        closing=false;
        toll=5.0;

        string name=rf.check("name",Value("actionPerformer")).asString().c_str();
        string robot=rf.check("robot",Value("icubSim")).asString().c_str();
        string stereo=rf.check("stereo",Value("off")).asString().c_str();
        bool useStereo=stereo=="on";
        string rpcName="/"+name+"/rpc";
        string outName="/"+name+"/out";
        string filename;
        if (!useStereo)
        {
            if (robot=="icubSim")
                filename=rf.findFile("actionsSim").c_str();
            else
                filename=rf.findFile("actions").c_str();
        }
        else
        {
            if (robot=="icubSim")
                filename=rf.findFile("actionsStereoSim").c_str();
            else
                filename=rf.findFile("actionsStereo").c_str();
        }
        out.open(outName.c_str());

        if (!createDatabase(filename))
            return false;

        Property optArmRight;
        optArmRight.put("device", "remote_controlboard");
        optArmRight.put("remote",("/"+robot+"/right_arm").c_str());
        optArmRight.put("local",("/"+name+"/right_arm/position").c_str());

        Property optArmLeft;
        optArmLeft.put("device", "remote_controlboard");
        optArmLeft.put("remote",("/"+robot+"/left_arm").c_str());
        optArmLeft.put("local",("/"+name+"/left_arm/position").c_str());

        if (polyArmRight.open(optArmRight))
        {
            polyArmRight.view(posArmRight);
            polyArmRight.view(encRight);
        }
        else
            return false;

        if (polyArmLeft.open(optArmLeft))
        {
            polyArmLeft.view(posArmLeft);
            polyArmLeft.view(encLeft);
        }
        else
        {
            polyArmRight.close();
            return false;
        }

        rpc.open(rpcName.c_str());
        attach(rpc);

        return true;
    }
};

int main(int argc, char *argv[])
{
    Network yarp;

    ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultContext("gesture-recognition");
    rf.setDefaultConfigFile("config.ini");
    rf.setDefault("actions","actions.ini");
    rf.setDefault("actionsSim","actionsSim.ini");
    rf.setDefault("actionsStereo","actionsStereo.ini");
    rf.setDefault("actionsStereoSim","actionsStereoSim.ini");
    rf.configure(argc,argv);

    GestureRecognitionActionPerformer mod;
    mod.runModule(rf);

    return 0;
}

