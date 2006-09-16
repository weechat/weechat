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
# This script enables the execution of events triggered commands      #
# where the events are attach or detach of the screen weechat         #
# is running in.                                                      #
#                                                                     #
# Name:    OnAttach                                                   #
# Licence: GPL v2                                                     #
# Author:  Marcus Eggenberger <i@egs.name>                            #
#                                                                     #
#  Usage:                                                             #
#   /onattach or /ondetach to add new events                          #
#   /screenevents to manage events                                    #
#                                                                     #
#   use /help command for  detailed information                       #
#                                                                     #
#  Changelog:                                                         #
#   0.5: added screen guessing to backup screen-detection             #
#   0.4: fixed TypeError in weechat 0.1.9                             #
#   0.3: now checking on startup if weechat runs in a screen          #
#   0.2: added usage of get/set_plugin_config to store eventlist      #
#   0.1: first version released                                       #
#                                                                     #
#######################################################################



# ====================
#     Imports
# ====================
import os
import sys

# ====================
#     Constants
# ====================
NAME = 'OnAttach'
VERSION = '0.5'
DESCRIPTION = "Executing commands on screen Attach/Detach"

OFF = False
ON  = True

# ====================
#     Exceptions
# ====================
class OnAttachError(Exception):
    pass

# ====================
#     Helpers
# ====================
class WeePrint(object):
    def write(self, text):
        text = text.rstrip(' \0\n')                                    # strip the null byte appended by pythons print
        if text:
            weechat.prnt(text,'')

def getScreenByPpid():
    # the screen we are in should be our parents parent
    # aka something like:
    # SCREEN
    #  \_ -/bin/bash
    #      \_ weechat-curses
    ppid = os.getppid()

    # get SCREEN pid
    pipe = os.popen("ps o ppid= -p %d" % ppid)
    screenpid = pipe.read().strip()
    pipe.close()

    # check if screen pid really belongs to a screen
    pipe = os.popen("ps o cmd= -p %s" % screenpid)
    cmd = pipe.read().strip()
    pipe.close()

    if 'screen' not in cmd.lower():
        raise OnAttachError
    else:
        return screenpid

def getScreenByList():
    # checks if there is only one attached screen
    # if so use: it! ;)
    pipe = os.popen("screen -list | grep -i attached")
    screens = pipe.read().strip()
    pipe.close()
    screens = screens.splitlines()

    if len(screens) > 1:
        print "There are more then one screen currently attached."
        print "Detach all other running screens and reload OnAttach"
        print "to ensure correct screen detection."
        raise OnAttachError

    try:
        socket, status = screens[0].split()
    except:
        # thats no common screen list...
        print "failed!"
        raise OnAttachError
    else:
        print "Using screen %s" % socket
        return socket

def getScreenPid():
    try:
        return getScreenByPpid()
    except OnAttachError:
        print "!!! Unable to determine screen by parentid!"
        print "Trying to guess screen..."
        return getScreenByList()
    
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
#     Classes
# ====================
class Event(object):
    ESCAPECHAR = '\\'
    SEPARATOR = ';'

    def __init__(self, delay, step, channel, server, command, activehigh=True):
        self.delay = int(delay)
        self.step = step
        self.channel = channel
        self.server = server
        self.command = command
        self.activestate = activehigh
        self.reset()

    #@classmethod
    def unserialize(cls, step, serial):
        try:
            # let's try the easy way :)
            delay, channel, server, command, activestate = serial.split()
        except ValueError:
            # ok we got an escaped separator... :/
            data = serial.split(cls.SEPARATOR)
            for i in range(len(data)):
                if data[i].endswith(cls.ESCAPECHAR):
                    data[i+1] = data[i] + cls.SEPARATOR + data[i+1]
            data = [item.replace(cls.ESCAPECHAR + cls.SEPARATOR, cls.SEPARATOR) for item in data if not item.endswith(cls.ESCAPECHAR)]
            delay, channel, server, command, activestate = data


        delay = int(delay)
        activestate = activestate == 'True'
            
        return cls(delay, step, channel, server, command, activestate)
    # lets go for 2.3 compatiblity
    unserialize = classmethod(unserialize)
        
    #@property
    def serialized(self):
        data = [self.delay, self.channel, self.server, self.command, self.activestate]
        data = [str(item).replace(self.SEPARATOR, self.ESCAPECHAR + self.SEPARATOR) for item in data]
        return self.SEPARATOR.join(data)
    # lets go for 2.3 compatiblity
    serialized = property(serialized)
    
    def reset(self):
        self.currentdelay = self.delay
        self.fired = False

    def fire(self):
        weechat.command(self.command, self.channel, self.server)
        self.fired = True
        
    def stimulus(self, state):
        if state != self.activestate:
            self.reset()
        else:
            self.currentdelay -= self.step
            if self.currentdelay <= 0 and not self.fired:
                self.fire()

class CheckStatus(object):
    ESCAPECHAR = '\\'
    SEPARATOR = '|'

    eventlist = []
    step = 5
    def __init__(self):
        try:
            self.screenpid = getScreenPid()
        except OnAttachError:
            raise

        # try to read config data
        serializedEvents = weechat.get_plugin_config("events")
        if not serializedEvents:
            return

        events = serializedEvents.split(self.SEPARATOR)
        for i in range(len(events)):
            if events[i].endswith(self.ESCAPECHAR):
                events[i+1] = events[i] + self.SEPARATOR + ss[i+1]
        events = [event.replace(self.ESCAPECHAR + self.SEPARATOR, self.SEPARATOR) for event in events if not event.endswith(self.ESCAPECHAR)]
        for event in events:
            try:
                self.eventlist.append(Event.unserialize(self.step, event))
            except:
                print "Unable to unserialize event!"
                print "Try to add the event manualy and save config again."
        
    def __call__(self, server="", args=""):
        # check if the screen is attached or detatched
        pipe = os.popen("screen -list | grep %s" % self.screenpid)
        screenlist = pipe.read().strip()
        pipe.close()

        if 'attached' in screenlist.lower():
            state = ON
        elif 'detached' in screenlist.lower():
            state = OFF
        else:
            print "Unable to determine screen status"
            return weechat.PLUGIN_RC_KO

        for event in self.eventlist:
            event.stimulus(state)

        return weechat.PLUGIN_RC_OK

    def addEvent(self, delay, channel, server, command, activehigh=True):
        event = Event(delay, self.step, channel, server, command, activehigh)
        self.eventlist.append(event)

    def showEvents(self):
        format = "%2s | %-10s | %-10s | %-15s | %-7s | %-2s %s"
        separator = "---+------------+------------+-----------------+---------+----"
        print separator
        print format % ("ID", "Server", "Channel", "Command", "Delay", "on", "")
        print separator
        for i in range(len(self.eventlist)):
            event = self.eventlist[i]
            if event.activestate:
                atde = 'AT'
            else:
                atde = 'DE'

            if event.fired:
                fired = '*'
            else:
                fired = ''
            print format % (i, event.server[:10], event.channel[:10], event.command[:15], "%3d sec" % event.delay, atde, fired)
        print separator

    def save(self):
        weechat.set_plugin_config("events",self.serialized)

    #@property
    def serialized(self):
        events = [str(event.serialized).replace(self.SEPARATOR, self.ESCAPECHAR + self.SEPARATOR) for event in self.eventlist]
        return self.SEPARATOR.join(events)
    # lets go for 2.3 compatiblity
    serialized = property(serialized)
    
    def dropEvent(self, eventid):
        del self.eventlist[eventid]

# ====================
#     Functions
# ====================
def addEvent(server, args, activehigh=True):
    delay, channel, command = args.split(' ', 2)
    delay = int(delay)
    
    channel = channel.strip("'\"")
    checkStatus.addEvent(delay, channel, server, command, activehigh)
    if activehigh:
        atde = "at"
    else:
        atde = "de"
        
    print "Added %stach event '%s' for channel %s on network %s with %d seconds delay" % (atde, command, channel, server, delay)

def weeOnAttach(server, args):
    """delay channel command
    Adds a command which is executed after attaching the screen
    The command is executed <delay> seconds after the attach.
    Use "" or '' as channel if the command should be executed in
    the server buffer.

    Example:
      /onattach 180 &bitlbee account on 0
    """
    try:
        addEvent(server, args)
    except:
        weechat.command("/help onattach", "", server)
        return 0
    else:
        return 1

def weeOnDetach(server, args):
    """delay channel command
    Adds a command which is executed after detaching the screen
    The command is executed <delay> seconds after the detach.
    Use "" or '' as channel if the command should be executed in
    the server buffer.

    Example:
      /ondetach 180 &bitlbee account on 0
    """
    try:
        addEvent(server, args, activehigh=False)
    except:
        weechat.command("/help onattach", "", server)
        return 0
    else:
        return 1

def weeScreenevents(server, args="show|save|drop"):
    """show | save | drop eventid [eventid [...]]
    shows, saves or drops events
    show:
      Shows all current active events    

    save:
      Saves current events permanently

    drop:
      Delete event from onAttach/onDetach list.
      Use /showevents to get eventid
    """
    try:
        action, args = args.split(' ',1)
    except ValueError:
        action = args.strip()
        args = ''

    actions = {'show':showEvents,
               'drop':dropEvent,
               'save':saveEvents}
    try:
        action = actions[action]
    except KeyError:
        weechat.command("/help screenevents", "", server)
        return 0
    else:
        return action(server, args)



def showEvents(server, args):
    checkStatus.showEvents()
    return 1

def dropEvent(server, args):
    try:
        eventids = [int(id) for id in args.split()]
    except:
        print "eventid musst be a number!"
        return 0

    eventids.sort()
    eventids.reverse()
    for eventid in eventids:
        try:
            checkStatus.dropEvent(eventid)
        except:
            print "Unable to drop Event %d" % eventid
            return 0
    print "dropped %d events!" % len(eventids)
    return 1

def saveEvents(server, args):
    checkStatus.save()
    return 1

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
    try:
        checkStatus = CheckStatus()
    except OnAttachError:
        print "!!! Requirements for %s not met:" % NAME
        print "!!!  - WeeChat is not running in a screen or not able to get screen PID"
        print "!!! --> Run WeeChat in a screen to fix the problem!"
    else:

        weechat.add_timer_handler(checkStatus.step, "checkStatus")                      

        registerFunctions()

                      
