"""
  :Author: Tim Schumacher <tim AT we-are-teh-b DOT org>

  :What it does: 
    This script informs the active window of your last
    submitted song at last.fm

  :Usage:
    /np - Displays your last submitted song.

  :Configuration Variables:
    ============= ==========================================
    Variable name Meaning
    ============= ==========================================
    lastfmuser    Your username at last.fm
  
  Released under GPL licence.
"""
#!/usr/bin/python

todo = """
 - Realy check if a song was recent.
 - Define 'recent' through a userdefinedvar
 - Add more help

"""

import urllib
import weechat as wc

wc.register("lastfmnp", "0.2.1", "", "now playing for last.fm")

def getLastSong(server, args):
    """ 
    Provides your last submitted song in last.fm to the actual context
    """
    user = wc.get_plugin_config("lastfmuser")
    url = "http://ws.audioscrobbler.com/1.0/user/" + user + "/recenttracks.txt"
    url_handle = urllib.urlopen(url)
    lines = url_handle.readlines()
    song = lines[0].split(",")[1].replace("\n","");
    if song == '':
        song = 'nothing :-)';
    wc.command("np: " + song)
    return 0


wc.add_command_handler("np", "getLastSong")

default = {
  "lastfmuser": "timds235"
}

for k, v in default.items():
  if not wc.get_plugin_config(k):
    wc.set_plugin_config(k, v)
