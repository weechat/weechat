"""
  :Author: Henning Hasemann <hhasemann [at] web [dot] de>

  :What it does:
    This plugin lets you inform all users in the current
    channel about the song which music-player-daemon (MPD)
    is currently playing.

  :Usage:
    /mpdnp - Display file mpd is playing to current channel.

  :Configuration Variables:
    ============= ==========================================
    Variable name Meaning
    ============= ==========================================
    host          The host where your mpd runs
    port          The port to connect to mpd (usually 6600)
    format        How this script should display
                  whats going on.
                  You may use the following variables here:
                  $artist, $title_or_file,
                  $length_min, $length_sec, $pct,
                  $pos_min, $pos_sec
  
  Released under GPL licence.
"""

todo = """
  - maybe support sending commands to mpd.
  - maybe make condional display
    (displaying some characters only
     if preceding or trailing variables are set)
"""

import weechat as wc
import mpdclient as mpd
import re
from os.path import basename, splitext

default_fmt = "/me 's MPD plays: $artist - $title_or_file ($length_min:$length_sec)"

wc.register("mpdnp", "0.3", "", "np for mpd")

def subst(text, values):
  out = ""
  n = 0
  for match in re.finditer(findvar, text):
    if match is None: continue 
    else:
      l, r = match.span()
      nam = match.group(1)
      out += text[n:l+1] + values.get(nam, "") #"$" + nam)
      n = r
  return out + text[n:]

def np(server, args):
  """
    Send information about the currently
    played song to the channel.
  """
  host = wc.get_plugin_config("host")
  port = int(wc.get_plugin_config("port"))
  cont = mpd.MpdController(host=host, port=port)
  song = cont.getCurrentSong()
  pos, length, pct = cont.getSongPosition()

  # insert artist, title, album, track, path
  d = song.__dict__
  d.update({
      "title_or_file": song.title or splitext(basename(song.path))[0],
      "pos_sec": "%02d" % (pos / 60),
      "pos_min": str(pos / 60),
      "length_sec": "%02d" % (length % 60),
      "length_min": str(length / 60),
      "pct": "%2.0f" % pct,
  })
  
  wc.command(subst(wc.get_plugin_config("format"), d))
  return 0
  
def dbgnp(server, args):
  try:
    return np(server, args)
  except Exception, e:
    print e
  
wc.add_command_handler("mpdnp", "np", "", "", np.__doc__)

findvar = re.compile(r'[^\\]\$([a-z_]+)(\b|[^a-z_])')

default = {
  "host": "localhost",
  "port": "6600",
  "format": default_fmt,
}

for k, v in default.items():
  if not wc.get_plugin_config(k):
    wc.set_plugin_config(k, v)


