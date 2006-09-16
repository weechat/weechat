# This script sends "uname -a" output to current channel.
# 	Just type /uname while chatting on some channel ;)

# 		by Stalwart <stlwrt doggy gmail.com>
#
# Released under GPL licence.

import weechat
from os import popen

def senduname(server, args):
	unameout = popen ('uname -a')
        uname = unameout.readline()
	weechat.command(uname[:-1])
	return 0

weechat.register ('uname', '1.0', '', """Sends "uname -a" output to current channel""")
weechat.add_command_handler ('uname', 'senduname')
