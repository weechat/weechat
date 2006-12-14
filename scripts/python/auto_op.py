# --------------------------------------------------------------------
#
# Copyright (c) 2006 by Jean-Marie Favreau <jm@jmtrivial.info>
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
# --------------------------------------------------------------------
# This script automatically op and voice some nicks
# --------------------------------------------------------------------
#
# 2006-12-14, FlashCode <flashcode@flashtux.org>:
#   fixed message split (for servers like quakenet with no ":" after "JOIN")

import weechat
import re


# regexp list for /op
U_OP = { "server": { "#chan" : [ "nick1@domain.com", "nick2.*@.*"] } }

# chan list where all nicks are /voice
C_VOICE = { "server": [ "#chan1", "#chan2" ] }


def auto_op(server, args):
	'''Handle connect'''	
	result = weechat.PLUGIN_RC_OK
	# first, watch if need /op
	if U_OP.has_key(server):
		chans = U_OP[server]
		try:
			# find nick and channel
			user, channel = args.split(" JOIN ")
			nick, next = user.split("!")
			if channel.startswith(':'):
				channel = channel[1:]
		except ValueError:		           
			result = weechat.PLUGIN_RC_KO
		else:			
			if chans.has_key(channel):
				users = chans[channel]
				for userExpr in users:
					if re.search("^n=" + userExpr, next): 
						weechat.command("/op "+nick, channel, server) # op nick 
						weechat.prnt("[op] "+nick+" on "+channel+"("+server+")") # print 
						return result # exit

	
	# then watch if need /voice
	if C_VOICE.has_key(server):
		chans = C_VOICE[server]
		try:
			# find nick and channel
			user, channel = args.split(" JOIN ")
			nick, next = user.split("!")
			if channel.startswith(':'):
				channel = channel[1:]
		except ValueError:
			result = weechat.PLUGIN_RC_KO
		else:
			if channel in chans:
				weechat.command("/voice "+nick, channel, server) # voice nick
				weechat.prnt("[voice] "+nick+" on "+channel+"("+server+")") # print info
				return result # exit
														
	# otherwise: nothing to do
	return result
	
	
# register and add function to weechat
weechat.register("auto_op", "0.3", "", "auto op plug-in for weechat")
weechat.add_message_handler ("join", "auto_op")


