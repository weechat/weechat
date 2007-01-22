# mp3blaster nowplaying script by Stalwart <stlwrt@gmail.com>
# written for Caleb
# released under GNU GPL v2 or newer

import weechat
from os.path import expandvars, expanduser

def mp3blasternp(server, args):
		try:
				statusfile = open(expandvars(expanduser(weechat.get_plugin_config('statusfile'))), 'rU')
				info = statusfile.readlines()
				statusfile.close()
				artist = title = path = ''
				for line in info:
						if (line.split()[0] == 'artist'):
								artist = line.strip(' \n')[7:]
						elif (line.split()[0] == 'title'):
								title = line.strip(' \n')[6:]
						elif (line.split()[0] == 'path'):
								path = line.strip(' \n')[5:]
				if (title):
						if (artist):
								weechat.command('/me np: '+artist+' - '+title)
						else:
								weechat.command('/me np: '+title)
				else:
						weechat.command('/me np: '+path)
		except:
				if (weechat.get_plugin_config('statusfile') == ''): 
						weechat.set_plugin_config('statusfile', '~/.mp3blasterstatus')
				weechat.prnt('mp3blaster status file not found. Set proper using /setp python.mp3blaster.statusfile = /path/to/file')

		return 0

weechat.register ('mp3blaster', '1.0', '', 'Posts track now played by mp3blaster')
weechat.add_command_handler ('np', 'mp3blasternp')
