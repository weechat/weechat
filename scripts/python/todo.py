"""
  :Author: Henning Hasemann <hhasemann [at] web [dot] de>

  :What it does:
    This script listens for messages beginning
    with a special token and appends these messages
    to a file.
    This way people can post you short assignments which
    you cant forget and dont have to look up in your look.
    (try appending "cat /path/to/mytodofile" at your .bashrc)

  :Usage:
    - Load this file
    - Make sure to set suitable configuration values.
      token: The piece of text that signals a TODO
      file: The file where the TODOs should be appended
      allowed_sources: A space-seperated list of nicknames
        which are allowed to send TODOs to you.
    - Whenever any allowed person sends a message beginning
      with your desired token, the rest of that message is
      append to the desired TODO-file.

  Released under GPL licence.
"""
__version__ = "0.1"

import weechat as wc

wc.register("todo", __version__, "", "automatic TODO")
wc.add_message_handler("privmsg", "on_msg")

default = {
  "token": "##todo ",
  "file": "/home/USER/todo",
  "allowed_sources": "",
}

for k, v in default.items():
  if not wc.get_plugin_config(k):
    wc.set_plugin_config(k, v)

def source_allowed(src):
  return src in wc.get_plugin_config("allowed_sources").split()

def on_msg(server, args):
  # args looks like:
  # :foo!foo@host PRIVMSG #channel :Hello, how are you?

  token = wc.get_plugin_config("token")
  filename = wc.get_plugin_config("file")

  try:
    nothing, info, message = args.split(":", 2)
    hostmask, privmsg, channel = info.split(None, 2)
    source = hostmask.split("!")[0]
  except ValueError:
    # Parsing didnt work,
    # this happens mostly when strange messages
    # arrive that dont have the format described above
    return 0

  if source_allowed(source) and message.lower().startswith(token):
    wc.print_infobar(5, "NEW TASK: " + str(message[7:]))
    f = open(filename, "a")
    f.write("%s (%s)\n" % (message[7:], source))
    f.close()

  return 0

