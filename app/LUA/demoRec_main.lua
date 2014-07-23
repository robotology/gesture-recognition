#!/usr/bin/lua

require("rfsm")
require("yarp")
--require("demoRec_funcs")

yarp.Network()


-------
shouldExit = false

-- initilization
ispeak_port = yarp.BufferedPortBottle()
speechRecog_port = yarp.Port()
demoRec_port = yarp.Port()
gestRec_port = yarp.Port()

-- defining objects and actions vocabularies
objects = {"octopus", "lego", "toy", "ladybug", "turtle", "car", "bottle", "box"}
-- defining speech grammar for Menu
grammar = "Let's start | You rock | You lost | Your turn | Game over | Good bye | Lets save | I stop | Save one | Save two | Save three | Save four | Save five | Save six | Let's train | Let's recognize | Initial position"

-- load state machine model and initalize it
rf = yarp.ResourceFinder()
rf:setDefaultContext("gesture-recognition/LUA")
rf:configure(arg)
fsm_file = rf:findFile("demoRec_root_fsm.lua")
fsm_model = rfsm.load(fsm_file)
fsm = rfsm.init(fsm_model)
rfsm.run(fsm)

repeat
    rfsm.run(fsm)
    yarp.Time_delay(0.09)
until shouldExit ~= false

print("finishing")
-- Deinitialize yarp network
yarp.Network_fini()
