#
# Copyright (c) 2006 by SpideR <spider312@free.fr> http://spiderou.net
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

# WeeChat clone scanner
# Scans clones on a chan when you ask it to do (/clones)
# Able to scan for a nick's clones on each join, if you ask it to do (/autoclones)

import weechat

SCRIPT_NAME="clonescan"
SCRIPT_VERSION="0.1"
SCRIPT_DESC="clonescan script for weechat"
SCRIPT_DISP=SCRIPT_NAME+" v"+SCRIPT_VERSION

# Register, Handlers, config check/creation
if weechat.register(SCRIPT_NAME, SCRIPT_VERSION, "unload", SCRIPT_DESC):
	weechat.add_command_handler("clones","scanchan",
			"Scans clones on specified or current chan",
			"[#chan]")
	weechat.add_command_handler("autoclones","toggleauto",
			"Manage auto clone-scanning",
			"[enable|disable|show]")
	weechat.add_message_handler ("join", "scanjoin")
	autoscan = weechat.get_plugin_config("autoscan")
	if ( ( autoscan != "true" ) and ( autoscan != "false" ) ):
		weechat.set_plugin_config("autoscan","false")
		weechat.prnt("Unconfigured autoscan set to 'disabled', to enable : /autoclones enable")
	weechat.prnt(SCRIPT_DISP+" loaded")
else:
	weechat.prnt(SCRIPT_DISP+" not loaded")

# Unload handler
def unload():
	weechat.prnt("starting "+SCRIPT_DISP+" unload ...")
	return 0

# Auto scan on JOIN
def scanjoin(server,args):
	result = weechat.PLUGIN_RC_OK
	if ( weechat.get_plugin_config("autoscan") == "true" ):
		try: # Cut args because it contains nick, host and chan
			nothing, user, chan = args.split(":") # :Mag!Magali@RS2I-35243B84.ipt.aol.com JOIN :#bringue
			nick, next = user.split("!") # Mag!Magali@RS2I-35243B84.ipt.aol.com JOIN 
			userathost, nothing = next.split(" JOIN ") # Magali@RS2I-35243B84.ipt.aol.com JOIN 
			host = removeuser(userathost) # Magali@RS2I-35243B84.ipt.aol.com
			# Problems with IPv6 hosts' ":" : 
			# [:higuita!n=higuita@2001:b18:400f:0:211:d8ff:fe82:b10e JOIN :#weechat]
		except ValueError:		           
			result = weechat.PLUGIN_RC_KO
			weechat.prnt("Eror parsing args : ["+args+"]",server,server)
		else:
			clones = scannick(server,chan,nick,host) # Scan for that user's clones
			if ( len(clones) > 0):
				disp = "Clone sur "+chan+"@"+server+" : "+dispclones(nick,clones,host)
				weechat.print_infobar(5,disp) # Display on infobar
				weechat.prnt(disp) # Display on current buffer
				weechat.prnt(disp,server,server) # Display on server buffer
	return result

# Config auto scan
def toggleauto(server,args):
	# Get current value
	autoscan = weechat.get_plugin_config("autoscan")
	# Testing / repairing
	if ( autoscan == "true" ):
		auto = True
	elif ( autoscan == "false" ):
		auto = False
	else:
		weechat.prnt("Unknown value ["+autoscan+"], disabling")
		weechat.set_plugin_config("autoscan","false")
		auto = False
	# managing arg
	if ( args == "enable" ):
		if auto:
			weechat.prnt("Auto clone scanning remain enabled")
		else:
			weechat.set_plugin_config("autoscan","true")
			weechat.prnt("Auto clone scanning is now enabled")
	elif ( args == "disable" ):
		if auto:
			weechat.set_plugin_config("autoscan","false")
			weechat.prnt("Auto clone scanning is now disabled")
		else:
			weechat.prnt("Auto clone scanning remain disabled")
	elif ( args == "break" ):
		weechat.set_plugin_config("autoscan","blah")
	else:
		if auto:
			weechat.prnt("Auto clone scanning enabled")
		else:
			weechat.prnt("Auto clone scanning disabled")
	return weechat.PLUGIN_RC_OK

# Manual channel scan
def scanchan(server,args):
	# Defining chan to scan (contained in args, current chan otherwise)
	if ( args == "" ):
		chan = weechat.get_info("channel",server)
	else:
		chan = args
	# Scan
	if ( chan != "" ):
		nicks = weechat.get_nick_info(server,chan)
		allclones = [] # List containing all detected clones, for not to re-scan them
		nbclones = 0 # number of clones
		if nicks != None:
			if nicks != {}:
				weechat.prnt("Scanning "+chan+" ...")
				for nick in nicks:
					if nick not in allclones:
						host = removeuser(nicks[nick]["host"])
						clones = scannick(server,chan,nick,host)
						if ( len(clones) > 0 ):
							allclones = allclones + clones
							nbclones = nbclones+1
							weechat.prnt(" - "+dispclones(nick,clones,host))
				weechat.prnt(str(nbclones)+" clones found")
			else:
				weechat.prnt("Nobody on "+chan+", are you sure it's a chan and you are present on it ?")
		else:
			weechat.prnt("Eror reading nick list")
	else:
		weechat.prnt("Not on a chan")
	return weechat.PLUGIN_RC_OK

# Scan of a nick
# Returns list of nick clones (not containing nick himself)
def scannick(server,chan,nick,host):
	cloneof = []
	compares = weechat.get_nick_info(server,chan)
	if compares != None:
		if compares != {}:
			for compare in compares:
				if ( ( nick != compare ) and ( host == removeuser(compares[compare]["host"])) ):
					cloneof.append(compare)
		else:
			weechat.prnt("pas de pseudo")
			
	else:
		weechat.prnt("erreur de lecture des pseudos")
	return cloneof

# Display of one clone line
def dispclones(nick,clones,host):
	clones.append(nick)
	clones.sort()
	return str(clones)+" ("+host+")"

# Return host by user@host
def removeuser(userathost):
	splitted = userathost.split("@")
	return splitted[1]
