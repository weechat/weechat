"""
	Response: Simple autoresponse script for WeeChat
	Author:   Christian Taylor <cht@chello.at>
	License:  GPL version 2 or later
"""

version = "0.1"

helptext = """  /response list
    Displays a list of your current responses.
  
  /response toggle <number>
    Activates or deactivates response with the specified number.
  
  /response delete <number>
    Deletes response with the specified number.
  
  /response add "<message-regexp>" "<command>"
    Adds a response that executes <command> when it encounters a message matching
    the specified regular expression in any channel.
    <command> can be an actual command to be executed (for instance "/nick xyz")
    or a simple text (for instance "xyz") which is then posted to the channel.
    The following sequences are replaced in <command>:
      \\nick      =>  nickname of the user that triggered the response
      \\mask      =>  hostmask of the user that triggered the response
      \\channel   =>  name of the channel in which the response was triggered
      \\<number>  =>  matches of the message regular expression subgroups, \\0 for whole match
      $<number>  =>  whitespace-separated word number <number> of the whole triggering message
                     (starts with 0; negative numbers count from the last word)
  
  /response add "<hostmask-regexp>" "<channel-regexp>" "<message-regexp>" "<command>"
    Same as above, but only triggers if hostmask and channel match the given additional regexps.
    Blank regexps ("") will trigger for any hostmask and/or channel.
  
  Examples:
    /response add "^!whoami" "/msg \\channel You are \\nick (\\mask) in \\channel!"
    /response add "^!make (.*)" "/msg \\channel No rule for \\1. Stop."
    /response add "somebody@somehost" "" "^!op" "/op \\nick"
"""

import weechat, sys, re, pickle

weechat.register("Response", version, "", "Execute commands on specific triggers")

weechat.add_message_handler("privmsg", "triggercheck")
weechat.add_command_handler("response", "triggercontrol", "  Allows you to view, add and delete responses:", "<operation>", helptext, "add|delete|list|toggle")

debug = 0

t_enabled = 0
t_nick = 1
t_channel = 2
t_trigger = 3
t_reaction = 4
t_outstream = 5

def triggercheck(server, args):
	null, context, message = args.split(":", 2)
	mask, null, channel = context.strip().split(" ", 2)
	nick, whois = mask.split("!", 1)
	
	if debug: tell("Server: " + server + " | Mask: " + mask + " | Channel: " + channel + " | Message: " + message, server)
	
	for trigger in triggerlist:
		if trigger[t_enabled]:
			if trigger[t_channel]:
				channelmatch = re.compile(trigger[t_channel]).search(channel)
			else:
				channelmatch = True
			
			if channelmatch and trigger[t_nick]:
				nickmatch = re.compile(trigger[t_nick]).search(whois)
			else:
				nickmatch = True
			
			if channelmatch and nickmatch and trigger[t_trigger]:
				messagematch = re.compile(trigger[t_trigger]).search(message)
			else:
				messagematch = False
			
			if channelmatch and nickmatch and messagematch:
				reaction = trigger[t_reaction]
				reaction = reaction.replace("\\nick", nick)
				reaction = reaction.replace("\\{nick}", nick)
				reaction = reaction.replace("\\mask", whois)
				reaction = reaction.replace("\\{mask}", whois)
				reaction = reaction.replace("\\channel", channel)
				reaction = reaction.replace("\\{channel}", channel)
				
				messagewords = message.split()
				
				wordstring = re.compile("\\${?(-?\\d+)}?").search(reaction)
				replacecount = 0
				while wordstring and replacecount < 256:
					replacecount += 1
					try:
						reaction = reaction.replace(wordstring.group(0), messagewords[int(wordstring.group(1))])
					except IndexError:
						reaction = reaction.replace(wordstring.group(0), "")
					wordstring = re.compile("\\${?(-?\\d+)}?").search(reaction)
				
				subgroupstring = re.compile("\\\\{?(\\d+)}?").search(reaction)
				replacecount = 0
				while subgroupstring and replacecount < 256:
					replacecount += 1
					try:
						reaction = reaction.replace(subgroupstring.group(0), messagematch.group(int(subgroupstring.group(1))))
					except (IndexError, TypeError):
						reaction = reaction.replace(subgroupstring.group(0), "")
					subgroupstring = re.compile("\\\\{?(\\d+)}?").search(reaction)
				
				if trigger[t_outstream] == 1: weechat.command(reaction, channel, server)
				if trigger[t_outstream] == 2: weechat.prnt(reaction, channel, server)
	
	return weechat.PLUGIN_RC_OK

def tell(text, server):
	return weechat.prnt(str(text), "[" + str(server) + "]", server)

def triggercontrol(server, args):
	try:
		if args.startswith("list"): return listdisplay(server, args)
		if args.startswith("toggle"): return activetoggle(server, args)
		if args.startswith("delete"): return deletetrigger(server, args)
		if args.startswith("add"): return addtrigger(server, args)
		if args.startswith("ostoggle"): return outstreamtoggle(server, args)
		if args.startswith("debug"): return debugtoggle(server, args)
		
		tell("Unknown operation. Supported operations are add / delete / list / toggle.", server)
	except ValueError:
		tell("Error: Incorrect syntax (malformed number).", server)
	except:
		(e_type, e_value, e_trace) = sys.exc_info()
		tell("Error: %s: %s" % (str(e_type).replace("exceptions.", ""), e_value), server)
	
	return weechat.PLUGIN_RC_OK

def savetriggers(varname):
	savestring = pickle.dumps(triggerlist)
	savestring = savestring.replace("\n", "\\$$$")
	return weechat.set_plugin_config(varname, savestring)

def loadtriggers(varname):
	try:
		oldtriggers = weechat.get_plugin_config(varname)
		if oldtriggers:
			oldtriggers = oldtriggers.replace("\\$$$", "\n")
			newtriggerlist = pickle.loads(oldtriggers)
			return newtriggerlist
		else:
			return oldtriggers
	except:
		return False

def triggerstring(i, trigger):
	if trigger:
		cl = str(i+1) + " "
		if i < 9: cl += " "
		
		if trigger[t_enabled]:
			cl += "[X]  "
		else:
			cl += "[ ]  "
		
		if trigger[t_nick]:
			cl += "\"" + str(trigger[t_nick]) + "\" in "
		else:
			cl += "(everyone) in "
		
		if trigger[t_channel]:
			cl += "\"" + str(trigger[t_channel]) + "\":  "
		else:
			cl += "(all channels):  "
		
		cl += "\"" + str(trigger[t_trigger]) + "\"  =>  "
		
		cl += "\"" + str(trigger[t_reaction]) + "\""
		
		#if trigger[t_outstream] == 1: cl += "  <public output>"
		if trigger[t_outstream] == 2: cl += "  <private output>"
		
		return cl
	else:
		return "No response number " + str(i+1)

def listdisplay(server, args):
	if triggerlist:
		tell(" --- Responses ---", server)
		for i, trigger in enumerate(triggerlist):
			tell(triggerstring(i, trigger), server)
	else:
		tell("No responses in the list.", server)
	
	return weechat.PLUGIN_RC_OK

def activetoggle(server, args):
	triggernum = int(args.replace("toggle ", ""))
	if 1 <= triggernum < len(triggerlist) +1:
		if triggerlist[triggernum-1][t_enabled]:
			triggerlist[triggernum-1][t_enabled] = 0
			tell("Deactivated response number " + str(triggernum), server)
		else:
			triggerlist[triggernum-1][t_enabled] = 1
			tell("Activated response number " + str(triggernum), server)
		savetriggers("responses")
	else:
		tell("There is no response number " + str(triggernum), server)
	
	return weechat.PLUGIN_RC_OK

def outstreamtoggle(server, args):
	triggernum = int(args.replace("ostoggle ", ""))
	if 1 <= triggernum < len(triggerlist) +1:
		if triggerlist[triggernum-1][t_outstream] == 1:
			triggerlist[triggernum-1][t_outstream] = 2
			tell("Set response number " + str(triggernum) + " to private output", server)
		else:
			triggerlist[triggernum-1][t_outstream] = 1
			tell("Set response number " + str(triggernum) + " to public output", server)
		savetriggers("responses")
	else:
		tell("There is no response number " + str(triggernum), server)
	
	return weechat.PLUGIN_RC_OK

def deletetrigger(server, args):
	triggernum = int(args.replace("delete ", ""))
	if 1 <= triggernum < len(triggerlist) +1:
		del triggerlist[triggernum-1]
		tell("Deleted response number " + str(triggernum), server)
		if triggerlist:
			listdisplay(server, args)
		savetriggers("responses")
	else:
		tell("There is no response number " + str(triggernum), server)
	
	return weechat.PLUGIN_RC_OK

def addtrigger(server, args):
	newtriggerargs = args.replace("add ", "")
	if newtriggerargs.startswith('"') and newtriggerargs.endswith('"'):
		newtriggerargs = newtriggerargs[1:-1]
		newtrigger = newtriggerargs.split('" "')
	elif newtriggerargs.startswith("'") and newtriggerargs.endswith("'"):
		newtriggerargs = newtriggerargs[1:-1]
		newtrigger = newtriggerargs.split("' '")
	else:
		tell ("Error: Incorrectly formatted input.", server)
		return weechat.PLUGIN_RC_OK
	
	if len(newtrigger) == 2:
		triggerlist.append([1, "", "", 1])
		triggerlist[-1][3:3] = newtrigger
		tell(triggerstring(len(triggerlist)-1, triggerlist[-1]), server)
	elif len(newtrigger) == 3:
		triggerlist.append([1, "", 1])
		triggerlist[-1][2:2] = newtrigger
		tell(triggerstring(len(triggerlist)-1, triggerlist[-1]), server)
	elif len(newtrigger) == 4:
		triggerlist.append([1, 1])
		triggerlist[-1][1:1] = newtrigger
		tell(triggerstring(len(triggerlist)-1, triggerlist[-1]), server)
	else:
		tell("Error: Incorrectly formatted input.", server)
		return weechat.PLUGIN_RC_OK
	
	savetriggers("responses")
	
	return weechat.PLUGIN_RC_OK

def debugtoggle(server, args):
	global debug
	if debug:
		debug = 0
		tell("Debugging output deactivated", server)
	else:
		debug = 1
		tell("Debugging output activated", server)
	
	return weechat.PLUGIN_RC_OK


triggerlist = []
newtriggerlist = loadtriggers("responses")
if newtriggerlist:
	triggerlist = newtriggerlist
	weechat.prnt("Response: Loaded " + str(len(triggerlist)) + " responses, " + str(len([1 for x in triggerlist if x[t_enabled]])) + " are active.")
else:
	weechat.prnt("Response: No previously saved responses. Type '/help response' for usage information.")
