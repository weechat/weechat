"""
  :Author: Henning Hasemann <hhasemann [at] web [dot] de>
  :Updated: Daga <daga [at] daga [dot] dyndns [dot] org>

  :What it does:
    With this script you can configure weechat
    to give custom responses to CTCP-Requests.

  :Usage:
    Load this file somehow.
    You can configure replies to CTCP-requests
    with the /set_ctcp - command. (examples below)

  Released under GPL license.
  
  Hope you enjoy it :-)
    -- Henning aka asmanian
    -- Leonid Evdokimov (weechat at darkk dot net one more dot ru)
"""

version = "0.10"
history = """
  0.1 initial
  0.2
    - now displays a "CTCP FOOBAR received from ..."
  0.3 
    - now /help set_ctcp is available
  0.4
    - corrected a few typos
    - made special variables (e.g. version) easier to use
  0.5
    - removed debugging messages
  0.6
    - bugfix (sometimes /set_ctcp did not work)
  0.7 (Daga)
    - added multi-server support
  0.8
    - fixed on_msg (occasionally caused a minor fault)
  0.9
    - added dump_to_servchan and dump_to_current setting
  0.10
    - added SILENT (asmanian)
"""

short_syntax = """[REQUEST ANSWER]"""

syntax = """  Examples:

    /set_ctcp 
      show settings for common CTCP-Requests.
      where "OFF" means "use weechat default behaviour,
      "SILENT" means: "dont answer at all".

    /set_ctcp VERSION I prefer using weechat $version
      Reply with a fancy message.
      $version is substituted with weechats version.
      Other variables are: $away, $nick and $server.
      (If you find something else that could make sense
       here, let me know!)

    /set_ctcp HOW_ARE_YOU Good.
      Set answer to a rather unusual CTCP-request.
    
    /set_ctcp VERSION OFF
      Disable special behavior when CTCP VERSION comes in.
      atm this leaves an entry in plugins.rc which
      can be safely removed.
"""


import weechat as wc
import re

if wc.register("ctcp", version, "", "Customize CTCP replies"):
  wc.add_command_handler("set_ctcp", "set", "", short_syntax, syntax)
  wc.add_message_handler("privmsg", "on_msg")

  if not wc.get_plugin_config("requests"):
    wc.set_plugin_config("requests", " ".join(["VERSION", "USERINFO", "FINGER", "TIME", "PING"]))

  for key, default in {'dump_to_servchan': 'OFF', 'dump_to_current': 'ON'}.iteritems():
    value = wc.get_plugin_config(key)
    if not value in ['ON', 'OFF']:
      if value:
        wc.prnt("[ctcp]: invalid %s value, resetting to %s" % (key, default))
      wc.set_plugin_config(key, default)
  
def get_answers():
  # Strangely, get_plugin_config sometimes returns
  # the integer 0
  requests = wc.get_plugin_config("requests")
  if requests:
    requests = requests.split()
  else:
    requests = ["VERSION", "USERINFO", "FINGER", "TIME", "PING"]

  d = {}
  for r in requests:
    d[r] = wc.get_plugin_config("CTCP_" + r)
  return d
  
def set_answer(k, v):
  wc.set_plugin_config("CTCP_" + k, v)
  requests = wc.get_plugin_config("requests").split()
  if k.upper() not in requests:
    requests.append(k.upper())
    wc.set_plugin_config("requests", " ".join(requests))

def show_syntax():
  wc.prnt("usage: /set_ctcp %s" % short_syntax)
  wc.prnt("(For more information type /help set_ctcp)")

def on_msg(server, args):
  nothing, info, message = args.split(":", 2)
  hostmask = info.split(None, 2)[0]
  source = hostmask.split("!")[0]

  answers = get_answers()
  
  try:
    if message.startswith("\x01") and message.endswith("\x01"):
      req = message[1:-1]
      ans = answers[req]
      if not ans or ans == "OFF":
        raise ValueError

      if ans == "SILENT":
        return wc.PLUGIN_RC_OK_IGNORE_ALL

      info = {
        "version": wc.get_info("version"),
        "nick": wc.get_info("nick"),
        "away": wc.get_info("away") and "away" or "there",
        "server": server,
      }

      while True:
        match = re.search(r'[^\\]\$([a-z]+)\b', ans) #, r'--\1--', ans)
        if match is None: break
        else:
          l, r = match.span()
          ans = ans[:l+1] + info.get(match.group(1), "$" + match.group(1)) + ans[r:]
      
      if wc.get_plugin_config("dump_to_servchan") == 'ON':
        wc.prnt("CTCP %s received from %s (on %s)" % (req, source, server), server)
      if wc.get_plugin_config("dump_to_current") == 'ON':
        wc.prnt("CTCP %s received from %s (on %s)" % (req, source, server))
      wc.command("/quote NOTICE %s :\x01%s %s\x01" % (
            source, req, ans), "", server)
      return wc.PLUGIN_RC_OK_IGNORE_ALL
  
  except Exception, e:
    pass

  return wc.PLUGIN_RC_OK
  
def set(server, args):
  try:
    argv = args.split(None, 1)
    answers = get_answers()
    
    if not len(argv):
      for k, v in answers.items():
        wc.prnt("%10s: %s" % (k, v or "OFF"))

    elif len(argv) == 2:
      set_answer(argv[0], argv[1])
    
    else:
      show_syntax()
  except Exception, e:
    pass

  return wc.PLUGIN_RC_OK
  
