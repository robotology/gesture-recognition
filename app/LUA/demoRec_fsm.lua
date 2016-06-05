
event_table = {
   start     = "e_start",
   rock      = "e_start",
   turn      = "e_start",
   lost      = "e_start",
   over      = "e_start",
   bye       = "e_exit",
   save      = "e_start",
   stop      = "e_start",
   one       = "e_start",
   two       = "e_start",
   three     = "e_start",
   four      = "e_start",
   five      = "e_start",
   six       = "e_start",
   train     = "e_start",
   position  = "e_start",
   recognize = "e_start",
   }

interact_fsm = rfsm.state{

   ----------------------------------
   -- state SUB_MENU               --
   ----------------------------------
   SUB_MENU = rfsm.state{
           entry=function()
                   print("in substate MENU : waiting for speech command!")
           end,

           doo = function()
            while true do
                           ---speak(ispeak_port, "What should I do?")
                           result = SM_Reco_Grammar(speechRecog_port, grammar)
                           print("received REPLY: ", result:toString() )
                           local cmd =  result:get(3):asString()
                           rfsm.send_events(fsm, event_table[cmd])
                                              rfsm.yield(true)
            end
           end
   },

   ----------------------------------
   -- states                       --
   ----------------------------------

   SUB_EXIT = rfsm.state{
           entry=function()
                   speak(ispeak_port, "Ok, bye bye")
                   rfsm.send_events(fsm, 'e_menu_done')
           end
   },

   SUB_ACTIONS = rfsm.state{
           entry=function()
                   local act = result:get(3):asString()
                                   if act == "start" then
                                           demoRec_actions(demoRec_port,"start")
                                   elseif act == "rock" then
                                           demoRec_actions(demoRec_port,"win")
                                   elseif act == "lost" then
                                           demoRec_actions(demoRec_port,"lose")
                                   elseif act == "turn" then
                                           demoRec_actions(demoRec_port,"turn")
                                   elseif act == "over" then
                                           demoRec_actions(demoRec_port,"over")
                                   elseif act == "exit" then
                                           demoRec_actions(demoRec_port,"skip")
                                   elseif act == "save" then
                                           demoRec_actions(gestRec_port,"save")
                                   elseif act == "one" then
                                           demoRec_save(demoRec_port,1)
                                   elseif act == "two" then
                                           demoRec_save(demoRec_port,2)
                                   elseif act == "three" then
                                           demoRec_save(demoRec_port,3)
                                   elseif act == "four" then
                                           demoRec_save(demoRec_port,4)
                                   elseif act == "five" then
                                           demoRec_save(demoRec_port,5)
                                   elseif act == "six" then
                                           demoRec_save(demoRec_port,6)
                                   elseif act == "train" then
                                           demoRec_actions(demoRec_port,"trained")
                                   elseif act == "recognize" then
                                           demoRec_actions(gestRec_port,"rec")
                                   elseif act == "position" then
                                           demoRec_actions(gestRec_port,"pos")
                                   elseif act == "stop" then
                                           demoRec_actions(gestRec_port,"stop")
                                   else
                                           speak(ispeak_port,"I don't understand")
                                   end
           end
   },

   ----------------------------------
   -- state transitions            --
   ----------------------------------

   rfsm.trans{ src='initial', tgt='SUB_MENU'},
   rfsm.transition { src='SUB_MENU', tgt='SUB_EXIT', events={ 'e_exit' } },

   rfsm.transition { src='SUB_MENU', tgt='SUB_ACTIONS', events={ 'e_start' } },
   rfsm.transition { src='SUB_ACTIONS', tgt='SUB_MENU', events={ 'e_done' } },

   

}
