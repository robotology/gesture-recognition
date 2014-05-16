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

using namespace std;
using namespace yarp::sig;
using namespace yarp::os;
using namespace yarp::math;

class GestureRecognitionGameManager: public RFModule
{
protected:

    Port rpc;
    Port out;
    Port outspeak;
    BufferedPort<Bottle> inSequence;
    RpcClient attSelRpc;
    string sequence;
    string currentSequence;
    bool myturn;
    bool yourturn;
    bool gameEnding;
    int numActions;
    int count;
    int temp;

    RandScalar randGen;

    bool close()
    {
        rpc.close();
        out.close();
        outspeak.close();
        inSequence.close();
        attSelRpc.close();
        return true;
    }

    bool interruptModule()
    {
        rpc.interrupt();
        out.interrupt();
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
        if (cmd.get(0).asString()=="start")
        {
            count=1;
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
            out.write(bot1,bot2);
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
            cmd.addString("recognize");
            cmd.addInt(atoi(sequence.c_str()));
            out.write(cmd,reply);
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
                    out.write(bot1,bot2);
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
                    out.write(bot1,bot2);
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
        string name="gestureRecognitionGameManager";
        string rpcName="/"+name+"/rpc";
        string outName="/"+name+"/out";
        string inName="/"+name+"/in";
        rpc.open(rpcName.c_str());
        attach(rpc);
        out.open(outName.c_str());
        inSequence.open(inName.c_str());
        string outspeakname="/"+name+"/outspeak";
        outspeak.open(outspeakname.c_str());
        attSelRpc.open(("/"+name+"/attsel:rpc").c_str());

        sequence="";
        currentSequence="";
        myturn=false;
        yourturn=false;
        gameEnding=true;
        count=0;
        numActions=rf.check("numActions",Value(8)).asInt();
        temp=0;

        return true;
    }
};

int main(int argc, char *argv[])
{
    Network yarp;

    ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultContext("gestureRecognition");
    rf.setDefaultConfigFile("config.ini");
    rf.configure(argc,argv);

    GestureRecognitionGameManager mod;
    mod.runModule(rf);

    return 0;
}

