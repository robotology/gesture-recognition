
function speak(port, str)
   local wb = port:prepare()
    wb:clear()
    wb:addString(str)
    port:write()
   yarp.Time_delay(1.0)
end

----------------------------------
-- functions ACTIONS - demoRecognition        --
----------------------------------

function demoRec_actions(port, actName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
   wb:addString(actName)
   port:write(wb,reply)
   return reply
end

function demoRec_save(port, actName)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
   wb:addString("save")
   wb:addString(actName)
   port:write(wb,reply)
   return reply
end

----------------------------------
-- functions SPEECH             --
----------------------------------

function SM_RGM_Expand(port, vocab, word)
    local wb = yarp.Bottle()
   local reply = yarp.Bottle()
    wb:clear()
    wb:addString("RGM")
   wb:addString("vocabulory")
   wb:addString("add")
   wb:addString(vocab)
   wb:addString(word)
    port:write(wb,reply)
   --print(reply:get(1):asString())
   return reply:get(1):asString()
end

function SM_Expand_asyncrecog(port, gram)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("asyncrecog")
   wb:addString("addGrammar")
   wb:addString(gram)
    port:write(wb,reply)
end

function SM_Reco_Grammar(port, gram)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("recog")
   wb:addString("grammarSimple")
   wb:addString(gram)
    port:write(wb,reply)
   return reply
end

function SM_RGM_Expand_Auto(port, vocab)
   local wb = yarp.Bottle()
   local reply = yarp.Bottle()
   wb:clear()
    wb:addString("RGM")
   wb:addString("vocabulory")
   wb:addString("addAuto")
   wb:addString(vocab)
    port:write(wb,reply)
   return reply:get(1):asString()
end

--[[
proc SM_Reco_Grammar { gram } {

   bottle clear
   bottle addString "recog"
   bottle addString "grammarSimple"
   bottle addString $gram
   SpeechManagerPort write bottle reply
   puts "Received from SpeechManager : [reply toString] "
   set wordsList ""
   for { set i 1 } { $i< [reply size] } {incr i 2} {
           set wordsList [lappend wordsList [ [reply get $i] toString] ]
   }
   return $wordsList
}
]]--
