# Author: Skippy the Kangoo <Skippythekangoo AT yahoo DOT fr>
# This script manages basic controls of mpd (play/pause, next song, previous song, increase volume, decrease volume, display current song) 
# Usage: /mpd toggle|next|prev|volume_up|volume_down|status
# Released under GNU GPL v2 or newer

import weechat
from os import popen

weechat.register ('mpd', '0.0.2', '', 'mpd control script')

weechat.add_command_handler('mpd', 'mpd', 'mpd control', 'toggle|next|prev|volume_up|volume_down|status', '', 'toggle|next|prev|volume_up|volume_down|status')

def toggle (server, args,):
	popen('mpc toggle')
	return weechat.PLUGIN_RC_OK

def next (server, args,):
	popen('mpc next')
	return weechat.PLUGIN_RC_OK

def prev (server, args,):
	popen('mpc prev')
	return weechat.PLUGIN_RC_OK

def volume_up (server, args,):
	popen('mpc volume +10')
	return weechat.PLUGIN_RC_OK

def volume_down (server, args,):
	volume_down = popen('mpc volume -10')
	return weechat.PLUGIN_RC_OK

def status (server, args,):
	status = popen('mpc').readline().rstrip()
	weechat.print_infobar(3, status)
	return weechat.PLUGIN_RC_OK

def mpd(server, args):

	largs = args.split(" ")    

	#strip spaces
	while '' in largs:
		largs.remove('')
	while ' ' in largs:
		largs.remove(' ')

	if len(largs) ==  0:
		weechat.command("/help mpd")
		return weechat.PLUGIN_RC_OK
	else:
		inchan = False

	if largs[0] == 'toggle':
		toggle(" ".join(largs[1:]), inchan)
	elif largs[0] == 'next':
		next(" ".join(largs[1:]), inchan)
	elif largs[0] == 'prev':
		prev(' '.join(largs[1:]), inchan)
	elif largs[0] == 'volume_up':
		volume_up(' '.join(largs[1:]), inchan)
	elif largs[0] == 'volume_down':
		volume_down(' '.join(largs[1:]), inchan)
	elif largs[0] == 'status':
		status(" ".join(largs[1:]), inchan)
	return weechat.PLUGIN_RC_OK
