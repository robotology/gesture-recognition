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
\defgroup gameManager gameManager

The module that implements the rules of All Gestures You Can game. It manages the
interactions between gestureRecognition and actionPerformer module.

\section intro_sec Description 
This module manages the interactions between gestureRecognition and actionPerformer 
modules. It keeps track of the turns between the human and the robot, and communicates
with both the recognition and the performer modules.
 
\section rpc_port Commands:
The commands sent as bottles to the module port /<modName>/rpc
are described in the following:

<b>SAVE</b> \n
format: [save param] \n
action: this command serves to save all the features in the linear classifier
module, for the successive training procedure. param can be a number from 1 to 
the number of actions that you want to train. This command is used only when the 
gestureRecognitionStereo module is running, it is not valid for the gestureRecognition
using kinect.

<b>TRAINED</b> \n
format: [trained] \n
action: this command is needed to let the linearClassifierModule train all the
gestures saved so far. This command also is valid only for the gestureRecognitionStereo
module, not for the one using kinect.

<b>START</b> \n
format: [start] \n
action: through this command the game starts. The module chooses the opponent that
should start performing a gesture.

<b>WIN</b> \n
format: [win] \n
action: this command just communicates that the robot has won and terminates the game.

<b>LOSE</b> \n
format: [lose] \n
action: this command just communicates that the robot has lost and terminates
the game.

<b>OVER</b> \n
format: [over] \n
action: this command forcely terminates the game.

<b>DONE</b> \n
format: [done] \n
action: this is to notify that the robot has just finished performing
the sequence, so it's time for the human to replicate the gestures.

<b>TURN</b> \n
format: [turn] \n
action: this is to notify the robot that the human has just finished
performing the sequence, so it's time for the robot to replicate
the gestures.

\section lib_sec Libraries 
- YARP libraries. 

\section portsc_sec Ports Created 

- \e /<modName>/rpc remote procedure call. It always replies something.
- \e /<modName>/performer:o it sends the sequence to perform to the actionPerformer
  module.
- \e /<modName>/gestRec:o it sends command to the gestureRecognition module.
- \e /<modName>/scores:i this port reads the recognized gestures.
- \e /<modName>/outspeak this port sends sentences to the iSpeak module.

\section parameters_sec Parameters 
The following are the options that are usually contained 
in the configuration file, which is config.ini:

--name \e name
- specify the module name, which is \e gameManager by 
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
#include <yarp/os/RpcClient.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Time.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/math/Rand.h>

#define VOCAB_CMD_UPDATE_INITIAL VOCAB3('p','o','s')
#define VOCAB_CMD_REC            VOCAB3('r','e','c')
#define VOCAB_CMD_STOP           VOCAB4('s','t','o','p')
#define VOCAB_CMD_TRAIN          VOCAB4('t','r','a','i')
#define VOCAB_CMD_SAVE           VOCAB4('s','a','v','e')

using namespace std;
using namespace yarp::sig;
using namespace yarp::os;
using namespace yarp::math;

class GestureRecognitionGameManager: public RFModule
{
protected:

    Port rpc;
    Port outPerformer;
    Port outGestRec;
    Port outspeak;
    BufferedPort<Bottle> inSequence;
    RpcClient attSelRpc;
    string sequence;
    string currentSequence;
    bool myturn;
    bool yourturn;
    bool gameEnding;
    int numActions;
    bool useStereo;
    int count;
    int temp;

    RandScalar randGen;

    bool close()
    {
        rpc.close();
        outPerformer.close();
        outGestRec.close();
        outspeak.close();
        inSequence.close();
        attSelRpc.close();
        return true;
    }

    bool interruptModule()
    {
        rpc.interrupt();
        outPerformer.interrupt();
        outGestRec.interrupt();
        outspeak.interrupt();
        inSequence.interrupt();
        attSelRpc.interrupt();
        temp=-1;
        return true;
    }

    double getPeriod()
    {
        return 0.1;
    }

    bool respond(const Bottle& cmd, Bottle& reply) 
    {
        if (cmd.get(0).asString()=="save")
        {
            Bottle bot1,bot2;
            bot1.addVocab(VOCAB_CMD_STOP);
            outGestRec.write(bot1,bot2);
            string a="Ok, show me the gesture";
            Bottle w;
            w.addString(a.c_str());
            outspeak.write(w);
            Bottle bot7,bot8;
            bot7.addVocab(VOCAB_CMD_UPDATE_INITIAL);
            outGestRec.write(bot7,bot8);
            Bottle bot3,bot4;
            bot3.addVocab(VOCAB_CMD_SAVE);
            stringstream ss;
            ss << cmd.get(1).asInt();
            string str = "action"+ss.str();
            bot3.addString(str.c_str());
            outGestRec.write(bot3,bot4);
            reply.addString("Starting saving features");
            return true;
        }
        if (cmd.get(0).asString()=="trained")
        {
            string a="Training";
            Bottle w;
            w.addString(a.c_str());
            outspeak.write(w);
            Bottle bot1,bot2;
            bot1.addVocab(VOCAB_CMD_TRAIN);
            outGestRec.write(bot1,bot2);
            string b="Thanks, now I know the gesture";
            Bottle w1;
            w1.addString(b.c_str());
            outspeak.write(w1);
            reply.addString("Training done");
            return true;
        }
        if (cmd.get(0).asString()=="start")
        {
            count=1;
            if (useStereo)
            {
                Bottle bot1,bot2;
                bot1.addVocab(VOCAB_CMD_UPDATE_INITIAL);
                outGestRec.write(bot1,bot2);
                Bottle bot3,bot4;
                bot3.addVocab(VOCAB_CMD_REC);
                outGestRec.write(bot3,bot4);
            }
            double number=randGen.get(0,1);
            string turn;
            if (number<=0.5)
                turn="your turn";
            else
                turn="my turn";
            string a="Ok, let's start the game, it is "+turn;
            reply.addString("Ok, let's start the game ");
            reply.addString(yourturn?"your turn":"my turn");
            Bottle w;
            w.addString(a.c_str());
            outspeak.write(w);
            if(number>0.5)
                Time::delay(3.5);
            int pending=inSequence.getPendingReads();
            for (int i=0; i<pending; i++)
                inSequence.read(false);
            gameEnding=false;
            if (number>0.5)
                myturn=true;
            else
                yourturn=true;
            return true;
        }
        else if (cmd.get(0).asString()=="win"||cmd.get(0).asString()=="lose"||cmd.get(0).asString()=="over")
        {
            reply.addString("Quitting game");
            Bottle bot1,bot2;
            bot1.addString("stop");
            outPerformer.write(bot1,bot2);
            if (useStereo)
            {
                Bottle bot3,bot4;
                bot3.addVocab(VOCAB_CMD_STOP);
                outGestRec.write(bot3,bot4);
            }
            sequence="";
            currentSequence="";
            gameEnding=true;
            myturn=false;
            yourturn=false;
            fprintf(stdout, "Quitting game\n");

            if (cmd.get(0).asString()=="win")
            {
                Bottle w;
                w.addString("Yay! I'm happy!");
                outspeak.write(w);
                fprintf(stdout, "yay! I'm happy!\n");
            }
            else if (cmd.get(0).asString()=="lose")
            {
                Bottle w;
                w.addString("I'm really sad!");
                outspeak.write(w);
                fprintf(stdout, "Oh No!\n");
            }
            else
            {
                Bottle w;
                w.addString("Game over!");
                outspeak.write(w);
                fprintf(stdout, "Quitting!\n");
            }

            return true;
        }
        else if (cmd.get(0).asString()=="done"&&!gameEnding)
        {
            if (useStereo)
            {
                Bottle bot1,bot2;
                bot1.addVocab(VOCAB_CMD_UPDATE_INITIAL);
                outGestRec.write(bot1,bot2);
                Bottle bot3,bot4;
                bot3.addVocab(VOCAB_CMD_REC);
                outGestRec.write(bot3,bot4);
            }
            yourturn=true;
            int pending=inSequence.getPendingReads();
            for (int i=0; i<pending; i++)
                inSequence.read(false);
            reply.addString("ack");
            Bottle w;
            w.addString("It's your turn");
            outspeak.write(w);
            return true;
        }
        else if(cmd.get(0).asString()=="turn")
        {
            if (useStereo)
            {
                Bottle bot3,bot4;
                bot3.addVocab(VOCAB_CMD_STOP);
                outGestRec.write(bot3,bot4);
            }
            if(!gameEnding)
            {
                myturn=true;
                yourturn=false;
                reply.addString("It is my turn now");
            }
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
        if (!gameEnding && myturn)
        {
            //int number=rand() % numActions+1;
            int number=(int) randGen.get(1,numActions+0.99);
            ostringstream action;
            action << number;
            sequence=sequence+action.str();
            currentSequence=currentSequence+action.str();

            Bottle cmd,reply;
            cmd.addString("action");
            cmd.addInt(atoi(sequence.c_str()));
            outPerformer.write(cmd,reply);
            myturn=false;
            sequence="";
            count++;
        }
        if (!gameEnding && yourturn)
        {
            bool right=true;
            double t=Time::now();
            temp=0;
            while(!gameEnding /*&& temp<count && (Time::now()-t<4 || temp==0)*/ &&(yourturn && !myturn))
            {
                Bottle* bot=inSequence.read(false);
                if (bot!=NULL)
                {
                    if ((bot->get(0).asInt()>numActions) || (temp>(int)currentSequence.size()))
                    {
                        right=false;
                        break;
                    }
                    //int sub=atoi(currentSequence.substr(temp,1).c_str());
                    /*if (bot->get(0).asInt()!=sub && count!=1 && temp!=count-1)
                    {
                        fprintf(stdout, "Entrato, %d %d %d\n", count, sub, bot->get(0).asInt());
                        right=false;
                        break;
                    }*/
                    ostringstream buffer;
                    buffer << bot->get(0).asInt();
                    sequence=sequence+buffer.str();
                    t=Time::now();
                    temp++;
                }
            }
            if (temp==-1)
                return true;

            fprintf(stdout, "sequence: %s\n", sequence.c_str());
            if (!right)
            {
                if(!gameEnding)
                {
                    Bottle w;
                    w.addString("I think you're wrong");
                    fprintf(stdout, "I think you're wrong\n");
                    outspeak.write(w);
                    myturn=false;
                    yourturn=false;
                    count=0;
                    currentSequence="";
                    sequence="";
                    Bottle bot1,bot2;
                    bot1.addString("stop");
                    outPerformer.write(bot1,bot2);
                    if (useStereo)
                    {  
                        Bottle bot3,bot4;
                        bot3.addVocab(VOCAB_CMD_STOP);
                        outGestRec.write(bot3,bot4);
                    }
                    return true;
                }
                return true;
            }
            else if (sequence!="" && sequence.substr(0,sequence.size()-1)==currentSequence && !gameEnding)
                currentSequence=currentSequence+sequence.substr(sequence.size()-1,1);
            else
            {
                if(!gameEnding)
                {
                    Bottle w;
                    w.addString("I think you're wrong");
                    fprintf(stdout, "I think you're wrong\n");
                    outspeak.write(w);
                    myturn=false;
                    yourturn=false;
                    count=0;
                    currentSequence="";
                    sequence="";
                    Bottle bot1,bot2;
                    bot1.addString("stop");
                    outPerformer.write(bot1,bot2);
                    if (useStereo)
                    {  
                        Bottle bot3,bot4;
                        bot3.addVocab(VOCAB_CMD_STOP);
                        outGestRec.write(bot3,bot4);
                    }
                    return true;
                }
            }

            if(!gameEnding)
            {
                count++;
                myturn=true;
                yourturn=false;
            }
        }

        if (attSelRpc.getOutputCount()>0)
        {
            Bottle cmd,reply;
            cmd.addString("track");
            cmd.addString("partner");
            attSelRpc.write(cmd,reply);
        }

        return true;
    }

public:
    bool configure(ResourceFinder &rf)
    {
        string name=rf.check("name",Value("gameManager")).asString().c_str();
        rpc.open(("/"+name+"/rpc").c_str());
        attach(rpc);
        outPerformer.open(("/"+name+"/performer:o").c_str());
        outGestRec.open(("/"+name+"/gestRec:o").c_str());
        inSequence.open(("/"+name+"/scores:i").c_str());
        outspeak.open(("/"+name+"/outspeak").c_str());
        attSelRpc.open(("/"+name+"/attsel:rpc").c_str());

        sequence="";
        currentSequence="";
        myturn=false;
        yourturn=false;
        gameEnding=true;
        count=0;
        string stereo=rf.check("stereo",Value("off")).asString().c_str();
        useStereo=(stereo=="on");
        string robot=rf.check("robot",Value("icubSim")).asString().c_str();
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

        Property config; config.fromConfigFile(filename.c_str());
        Bottle& bgeneral=config.findGroup("general");
        numActions=bgeneral.find("numActions").asInt();
        temp=0;

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

    GestureRecognitionGameManager mod;
    mod.runModule(rf);

    return 0;
}

