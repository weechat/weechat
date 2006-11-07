"""
	PyExec:  Python code execution script for WeeChat
	Author:  Christian Taylor <cht@chello.at>
	License: GPL version 2 or later
"""

version = "0.2"
helptext = """  The WeeChat script-API is imported into the global namespace, you can
  call all API functions (for instance "get_info") directly. The modules
  "sys", "os" and "math" are imported by default.
  Any occurance of ";; " is treated as a newline.

  For automatic argument conversion to string, use:
    "send" instead of "command"
    "echo" instead of "prnt" (prints only to current buffer)
    (also provided: "echo_server", "echo_infobar")

  Additional shortcut functions:
    "nicks()" returns a dictionary of nicknames for the current channel.
    It takes a channelname and a servername as optional arguments.

  Examples:
    /pyexec for i in range(3): send(i+1);; echo("Done")
    /pyexec for nick in nicks(): send("/voice " + nick)
    /pyexec echo(2**64)
"""


from __future__ import division
import sys, os, math
from weechat import *

register("PyExec", version, "", "Run Python code in WeeChat")
add_command_handler("pyexec", "pyexec", "  Runs Python code directly from the WeeChat command line.", "[Python code]", helptext)

def echo(text):
	return prnt(str(text))

def echo_server(text):
	return print_server(str(text))

def echo_infobar(time, text):
	return print_infobar(time, str(text))

def send(text):
	return command(str(text))

def nicks(channel=None, server=None):
	if not server:
		server = get_info("server")
	if not channel:
		channel = get_info("channel")
	return get_nick_info(server, channel)

def pyexec(server, pycode):
	try:
		exec pycode.replace(";; ", "\n")
	except:
		(e_type, e_value, e_trace) = sys.exc_info()
		prnt("PyExec: %s: %s" % (str(e_type).replace("exceptions.", ""), e_value))
	
	return PLUGIN_RC_OK
