#Author: Pablo Escobar <pablo__escobar AT tlen DOT pl>
#What it does: This script shows the currently played song in exaile 
#Usage: /weexaile - Displays the songname
#Released under GNU GPL v2 or newer

#/usr/bin/python
#coding: utf-8

import weechat
import re
import codecs
from os import popen

weechat.register ('exaile', '0.01', '', """exaile-weechat current song script (usage: /weexaile)""")
weechat.add_command_handler ('weexaile', 'show_it_to_them')

default = {
  "msg_head": "is playing",
  "msg_tail": "with exaile",
  "spacer": "★",
  "colour_artist": "C03",
  "colour_title": "C02",
  "colour_lenght": "C05",
  "colour_spacer": "C08",
}

for k, v in default.items():
  if not weechat.get_plugin_config(k):
    weechat.set_plugin_config(k, v)

def show_it_to_them(server, args):
	spacer = weechat.get_plugin_config("spacer")
	msg_tail = weechat.get_plugin_config("msg_tail")
	msg_head = weechat.get_plugin_config("msg_head")
	colour_artist = weechat.get_plugin_config("colour_artist")
	colour_title = weechat.get_plugin_config("colour_title")
	colour_lenght = weechat.get_plugin_config("colour_lenght")
	colour_spacer = weechat.get_plugin_config("colour_spacer")
	exaile_running = popen ('exaile --get-title')
	exaile_running_text = exaile_running.readline().rstrip()
	if exaile_running_text != "No running Exaile instance found.":
		song_name = popen ('exaile --get-title')
		song_name_text = song_name.readline().rstrip()
		song_artist = popen ('exaile --get-artist')
		song_artist_text = song_artist.readline().rstrip()
		song_length = popen ('exaile --get-length')
		song_length_text = song_length.readline().rstrip()
		song_current = popen ('exaile --current-position')
		song_current_text = str(round(float(song_current.readline().rstrip()),2))
		all = '/me ' + msg_head + ' %' + colour_title + song_name_text + ' %' + colour_spacer + spacer + ' %' + colour_artist + song_artist_text + ' %' + colour_spacer + spacer + ' %' + colour_lenght + song_length_text + " (" + song_current_text + "%)" + " %C00" + msg_tail 
		weechat.command(all)
	return 0

