"""
	PyExec:  Python code execution script for WeeChat
	Author:  Christian Taylor <cht@chello.at>
	License: GPL version 2 or later
"""

version = "0.1"
helptext = """  The WeeChat script-API is imported into the global namespace, you can
  call all API functions (for instance get_nick_info) directly. The modules
  "sys", "os" and "math" are imported by default.
  Any occurance of ";; " is treated as a newline.

  For automatic string conversion, use:
    "send" instead of "command"
    "echo" instead of "prnt" (prints only to current buffer)
    (also provided: "echo_server", "echo_infobar")

  Examples:
    /pyexec for i in range(3): send(i+1);; echo("Done")
    /pyexec for nick in ["nick1", "nick2"]: send("/op " + nick)
"""


from weechat import *
import sys, os, math

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

def pyexec(server, pycode):
	try:
		for pyline in pycode.split(";; "):
			exec pyline
	except:
		(e_type, e_value, e_trace) = sys.exc_info()
		prnt("PyExec: %s [%s] in line: \"%s\"" % (e_type, e_value, pyline))
	
	return PLUGIN_RC_OK
