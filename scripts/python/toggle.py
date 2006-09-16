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
# This script adds a simple command to toggle weechat options         #
#                                                                     #
# Name:    Toggle                                                     #
# Licence: GPL v2                                                     #
# Author:  Marcus Eggenberger <i@egs.name>                            #
#                                                                     #
#  Usage:                                                             #
#   /toggle look_nicklist                                             #
#                                                                     #
#   use /help command for  detailed information                       #
#                                                                     #
#  Changelog:                                                         #
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
NAME = 'Toggle'
VERSION = '0.1'
DESCRIPTION = "Simple tool to toggle ON/OFF settings"

VALUES = ['OFF','ON']

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
def weeToggle(server, args="%o"):
    """<setting>
    Toggles any setting from ON to OFF and vice versa.
    Example:
     /toggle look_nicklist
    """
    option = args
    currentvalue = weechat.get_config(option).upper()
    if currentvalue not in VALUES:
        print "%s cannot be toggled!" % option
    else:
        newvalue = VALUES[VALUES.index(currentvalue) - 1]
        weechat.set_config(option, newvalue)
        print "%s = %s" % (option, newvalue)
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

                      
