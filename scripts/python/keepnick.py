# -*- encoding: iso-8859-1 -*-
#
# Copyright (c) 2006 by EgS <i@egs.name>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#


#######################################################################
#                                                                     #
# This script enables the user to keep their nicks and recover it in  #
# case it get's stolen. It uses the servers prefered nicks, so there  #
# is no need for any kind of setup.                                   #
#                                                                     #
# Name:    Keepnick                                                   #
# Licence: GPL v2                                                     #
# Author:  Marcus Eggenberger <i@egs.name>                            #
#                                                                     #
#  Usage:                                                             #
#   /keepnick on|off|<positive number>                                #
#                                                                     #
#   use /help command for  detailed information                       #
#                                                                     #
#  Changelog:                                                         #
#   0.4: now starts on load and features user defined check intervals #
#   0.3: Fixed major bug with continuous nickchanges                  #
#   0.2: Fixed Bug: now only checks connected servers                 #
#   0.1: first version released                                       #
#                                                                     #
#######################################################################

# ====================
#     Imports
# ====================
import sys

# ====================
#     Constants
# ====================
NAME = 'Keepnick'
VERSION = '0.4'
DESCRIPTION = "Plugin to keep your nick and recover it in case it's stolen"

ISON = '/ison %s'
NICK = '/nick %s'

NICKSTEALER_LEFT = "Nickstealer left Network: %s!"
KEEPNICK_ARG_ERROR = "\"%s\" is not a valid argument to /keepnick"
KEEPNICK_ON = "Keepnick locked and loaded! (checking every %s seconds)"
KEEPNICK_OFF = "Keepnick offline!"

DELAY = 10

# ====================
#     Exceptions
# ====================

# ====================
#     Helpers
# ====================
class WeePrint(object):
    def write(self, text):
        text = text.rstrip(' \0\n')                                    # strip the null byte appended by pythons print
        if text:
            weechat.prnt(text,'')

def registerFunction(function):
    # Register a python function as a commandhandler
    # Function needs to be named like weeFunction and
    # is bound to /function
    # docstring is used for weechat help
    functionname = function.__name__                                # guess what :)
    weecommand = functionname[3:].lower()                           # strip the wee

    doc = function.__doc__.splitlines()
    arguments = doc[0]                                              # First docstring line is the arguments string
    description = doc[1][4:]
    args_description = '\n'.join([line[4:] for line in doc[2:-1]])  # join rest and strip indentation

    if not function.func_defaults:                                  # use args default value as template
        template = ''
    elif len(function.func_defaults) == 1:
        template = function.func_defaults[0]
    elif len(function.func_defaults) == 2:
        template = func.func_defaults[1]
        
    weechat.add_command_handler(weecommand, functionname, description, arguments, args_description, template)

def registerFunctions():
    functions = [function for name, function in globals().iteritems() if name.startswith('wee') and callable(function)]
    for func in functions:
        registerFunction(func)
        
# ====================
#     Functions
# ====================
def servernicks(servername):
    server = weechat.get_server_info()[servername]
    servernicks = [server['nick1'], server['nick2'], server['nick3']]
    return servernicks
        
def ison(server, nicklist):
    weechat.command(ISON % ' '.join(nicklist), "", server)

def gotnick(server):
    server = weechat.get_server_info()[server]
    return server['nick'].lower() == server['nick1'].lower()

def checknicks():
    for servername, server in weechat.get_server_info().iteritems():
        if not gotnick(servername) and server['ssl_connected'] + server['is_connected']:
            ison(servername, servernicks(servername))
    return weechat.PLUGIN_RC_OK

def grabnick(server, nick):
    if not gotnick(server):
        print NICKSTEALER_LEFT % server
        weechat.command(NICK % nick, '', server)

def isonhandler(server, args):
    nothing, message, nicks = args.split(':')
    nicks = [nick.lower() for nick in nicks.split()]
    for nick in servernicks(server):
        if nick.lower() == weechat.get_info("nick",server):
            return weechat.PLUGIN_RC_OK_IGNORE_ALL
        elif nick.lower() not in nicks:
            grabnick(server, nick)
            return weechat.PLUGIN_RC_OK_IGNORE_ALL

    if 0 in [nick.lower() in [mynick.lower() for mynick in servernicks(server)] for nick in nicks]:
        # if any nick wich is return by ison is not on our checklist we're not the caller
        return weechat.PLUGIN_RC_OK
    else:
        # seems like we're the caller -> ignore the output
        return weechat.PLUGIN_RC_OK_IGNORE_ALL

def weeKeepnick(server, args="ON|OFF"):
    """ON|OFF|<positive number>
    Enables or Disables the use of keepnick.
    When active Keepnick ensures that you keep your preferred nicks.
    Keepnick checks for the preferred nicks of each server config,
    so there is no need to configure anything. Using a number as the
    argument it sets the checkperiod to that number in seconds.

    Example:
     /keepnick ON
    """
    VALIDARGS = ['ON','OFF']

    weechat.remove_handler('303', "isonhandler")
    weechat.remove_timer_handler("checknicks")

    try:
        delay = int(args)
        if delay < 1:
            raise ValueError
        globals()['DELAY'] = delay
    except ValueError:
        if args.upper() not in VALIDARGS:
            print KEEPNICK_ARG_ERROR % args
            return weechat.PLUGIN_RC_OK

    if args.upper() != 'OFF':
        weechat.add_timer_handler(DELAY,"checknicks")
        weechat.add_message_handler('303', "isonhandler")
        print KEEPNICK_ON % DELAY
    else:
        print KEEPNICK_OFF
        
    return weechat.PLUGIN_RC_OK
    
# ====================
#   Let's Register!
# ====================
if __name__ == '__main__':
    try:
        import weechat
    except ImportError:
        print "This script is to be loaded as PythonScript in WeeChat"
        print "Get WeeChat now at: http://weechat.flashtux.org/"
        import sys
        sys.exit(10)

    # kick pythons print to weechat.prnt(msg, '')
    sys.stdout = WeePrint()
    weechat.register(NAME, VERSION, "", DESCRIPTION)
    registerFunctions()
    weeKeepnick('','ON')



                      
