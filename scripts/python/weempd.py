#Author: Pablo Escobar <pablo__escobar AT tlen DOT pl>
#What it does: This script shows the currently played song in mpd 
#Usage: /weempd - Displays the songname 
#Released under GNU GPL v2 or newer

#/usr/bin/python
#coding: utf-8

import weechat
import re
import codecs
from os import popen

weechat.register ('weempd', '0.01', '', """mpd-weechat current song script (usage: /weempd)""")
weechat.add_command_handler ('weempd', 'show_it_to_them')

default = {
  "msg_head": "is playing",
  "msg_tail": "with mpd",
  "spacer": "★",
  "colour_title": "C12",
  "colour_spacer": "C08",
}

for k, v in default.items():
  if not weechat.get_plugin_config(k):
    weechat.set_plugin_config(k, v)

def show_it_to_them(server, args):
	spacer = weechat.get_plugin_config("spacer")
	msg_tail = weechat.get_plugin_config("msg_tail")
	msg_head = weechat.get_plugin_config("msg_head")
	colour_title = weechat.get_plugin_config("colour_title")
	colour_spacer = weechat.get_plugin_config("colour_spacer")
	tempinfo = popen('mpc').readline().rstrip()
	all = '/me ' + msg_head + ' %' + colour_spacer + spacer + ' %' + colour_title + tempinfo + ' %' + colour_spacer + spacer + " %C00" + msg_tail 
	weechat.command(all)
	return 0


