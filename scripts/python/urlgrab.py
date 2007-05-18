#
# UrlGrab, version 1.1, for weechat version 0.2.4
#
#   Listens to all channels for URLs, collects them in a list, and launches
#   them in your favourite web server on the local host or a remote server.
#
# Usage:
#
#   The /url command provides access to all UrlGrab functions.  Run
#   '/url help' for complete command usage.
#
#   In general, use '/url list' to list the entire url list for the current
#   channel, and '/url <n>' to launch the nth url in the list.  For
#   example, to launch the first (and most-recently added) url in the list,
#   you would run '/url 1'
#
#   From the server window, you must specify a specific channel for the
#   list and launch commands, for example:
#     /url list #weechat 
#     /url 3 #weechat
#
# Configuration:
#
#   The '/url set' command lets you get and set the following options:
#
#   historysize
#     The maximum number of URLs saved per channel.  Default is 10
#
#   method
#     Must be one of 'local' or 'remote' - Defines how URLs are launched by
#     the script.  If 'local', the script will run 'localcmd' on the host.
#     If 'remote', the script will run 'remotessh remotehost remotecmd' on
#     the local host which should normally use ssh to connect to another
#     host and run the browser command there.
#
#   localcmd
#     The command to run on the local host to launch URLs in 'local' mode.
#     The string '%s' will be replaced with the URL.  The default is
#     'firefox %s'.
#
#   remotessh
#     The command (and arguments) used to connect to the remote host for
#     'remote' mode.  The default is 'ssh -x' which will connect as the
#     current username via ssh and disable X11 forwarding.
#
#   remotehost
#     The remote host to which we will connect in 'remote' mode.  For ssh,
#     this can just be a hostname or 'user@host' to specify a username
#     other than your current login name.  The default is 'localhost'.
#
#   remotecmd
#     The command to execute on the remote host for 'remote' mode.  The
#     default is 'bash -c "DISPLAY=:0.0 firefox %s"'  Which runs bash, sets
#     up the environment to display on the remote host's main X display,
#     and runs firefox.  As with 'localcmd', the string '%s' will be
#     replaced with the URL.
#
#   cmdoutput
#     The file where the command output (if any) is saved.  Overwritten
#     each time you launch a new URL.  Default is ~/.weechat/urllaunch.log
#
#   default
#     The command that will be run if no arguemnts to /url are given.
#     Default is help
#
# Requirements:
#
#  - Designed to run with weechat version 0.2.4 or better.
#      http://weechat.flashtux.org/
#
# Acknowlegements:
#
#  - Based on an earlier version called 'urlcollector.py' by 'kolter' of
#    irc.freenode.net/#weechat Honestly, I just cleaned up the code a bit and
#    made the settings a little more useful (to me).
#
#  - With changes by Leonid Evdokimov (weechat at darkk dot net another dot ru):
#    http://darkk.net.ru/weechat/urlgrab.py
#    v1.1:  added better handling of dead zombie-childs
#           added parsing of private messages
#           added default command setting
#           added parsing of scrollback buffers on load
#
# Copyright (C) 2005 Jim Ramsay <i.am@jimramsay.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
# USA.
#

import sys
import os
import weechat
import subprocess

UC_NAME="UrlGrab"
UC_VERSION="1.1"

def urlGrabPrint(message):
    weechat.prnt("-[%s]- %s" % ( UC_NAME, message ) )

class WeechatSetting:
    def __init__(self, name, default, description = "" ):
        self.name = name
        self.default = default
        self.description = description
        test = weechat.get_plugin_config( name )
        if test is None or test == "":
            weechat.set_plugin_config( name, default )

class UrlGrabSettings:
    def __init__(self):
        self.settings = {
            'default':WeechatSetting('default', 'help',
                "default command to /url to run if none given" ),
            'historysize':WeechatSetting('historysize', '10',
                "Number of URLs to keep per channel" ),
            'method':WeechatSetting('method', 'local',
                """Where to launch URLs
         If 'local', runs %localcmd%.
         If 'remote' runs the following command:
           `%remotessh% %remotehost% %remotecmd`"""),
            'localcmd':WeechatSetting('localcmd', 'firefox %s',
                """Command to launch local URLs.  '%s' becomes the URL.
         Default 'firefox %s'"""),
            'remotessh':WeechatSetting('remotessh', 'ssh -x',
                """Command (and arguments) to connect to a remote machine.
         Default 'ssh -x'"""),
            'remotehost':WeechatSetting('remotehost', 'localhost',
                """Hostname for remote launching
         Default 'localhost'"""),
            'remotecmd':WeechatSetting('remotecmd',
                'bash -c \"DISPLAY=:0.0 firefox %s\"',
                """Command to launch remote URLs.  '%s' becomes the URL.
         Default 'bash -c \"DISPLAY=:0.0 firefox %s\"'"""),
            'cmdlog':WeechatSetting('cmdlog',
                '~/.weechat/urllaunch.log',
                """File where command output is saved.  Overwritten each
         time an URL is launched
         Default '~/.weechat/urllaunch.log'""" )
		}

    def has(self, name):
        return self.settings.has_key(name)

    def names(self):
        return self.settings.keys()

    def description(self, name):
        return self.settings[name].description

    def set(self, name, value):
        # Force string values only
        if type(value) != type("a"):
            value = str(value)
        if name == "method":
            if value.lower() == "remote":
                weechat.set_plugin_config( 'method', "remote" )
            elif value.lower() == "local":
                weechat.set_plugin_config( 'method', "local" )
            else:
                raise ValueError( "\'%s\' is not \'local\' or \'remote\'" % value )
        elif name == "localcmd":
            if value.find( "%s" ) == -1:
                weechat.set_plugin_config( 'localcmd', value + " %s" )
            else:
                weechat.set_plugin_config( 'localcmd', value )
        elif name == "remotecmd":
            if value.find( "%s" ) == -1:
                weechat.set_plugin_config( 'remotecmd', value + " %s" )
            else:
                weechat.set_plugin_config( 'remotecmd', value )
        elif self.has(name):
            weechat.set_plugin_config( name, value )
            if name == "historysize":
                urlGrab.setHistorysize(int(value))
        else:
            raise KeyError( name )

    def get(self, name):
        if self.has(name):
            return weechat.get_plugin_config(name)
        else:
            raise KeyError( name )

    def prnt(self, name, verbose = True):
        weechat.prnt( " %s = %s" % (name.ljust(11), self.get(name)) )
        if verbose:
            weechat.prnt( "  -> %s" % (self.settings[name].description) )

    def prntall(self):
        for key in self.names():
            self.prnt(key, verbose = False)

    def createCmdList(self, url):
        if weechat.get_plugin_config( 'method' ) == 'remote':
            tmplist = weechat.get_plugin_config( 'remotessh' ).split(" ")
            tmplist.append(weechat.get_plugin_config( 'remotehost' ))
            tmplist.append(weechat.get_plugin_config( 'remotecmd' ) % (url))
        else:
            tmplist = (weechat.get_plugin_config( 'localcmd' ) % (url) ).split(" ")
        return tmplist

class UrlGrabber:
    def __init__(self, historysize):
        # init
        self.urls = {}
        self.historysize = 5
        # control
        self.setHistorysize(historysize)

    def setHistorysize(self, count):
        if count > 1:
            self.historysize = count

    def getHistorysize(self):
        return self.historysize

    def addUrl(self, url, channel, server):
        # check for server
        if not self.urls.has_key(server):
            self.urls[server] = {}
        # check for chan
        if not self.urls[server].has_key(channel):
            self.urls[server][channel] = []
        # add url
        if url in self.urls[server][channel]:
            self.urls[server][channel].remove(url)
        self.urls[server][channel].insert(0, url)
        # removing old urls
        while len(self.urls[server][channel]) > self.historysize:
            self.urls[server][channel].pop()

    def hasIndex( self, index, channel, server ):
        return self.urls.has_key(server) and \
                self.urls[server].has_key(channel) and \
                len(self.url[server][channel]) >= index

    def hasChannel( self, channel, server ):
        return self.urls.has_key(server) and \
                self.urls[server].has_key(channel)

    def hasServer( self, server ):
        return self.urls.has_key(server)

    def getUrl(self, index, channel, server):
        url = ""
        if self.urls.has_key(server):
            if self.urls[server].has_key(channel):
                if len(self.urls[server][channel]) >= index:
                    url = self.urls[server][channel][index-1]
        return url
        

    def prnt(self, channel, server):
        found = True
        if self.urls.has_key(server):
            if self.urls[server].has_key(channel):
                urlGrabPrint(channel + "@" +  server)
                if len(self.urls[server][channel]) > 0:
                    i = 1
                    for url in self.urls[server][channel]:
                        weechat.prnt(" --> " + str(i) + " : " + url)
                        i += 1
                else:
                    found = False
            elif channel == "*":
                for channel in self.urls[server].keys():
                  self.prnt(channel, server)
            else:
                found = False
        else:
            found = False

        if not found:
            urlGrabPrint(channel + "@" +  server + ": no entries")

def urlGrabParsemsg(server, command):
    # :nick!ident@host PRIVMSG dest :foobarbaz
    l = command.split(' ')
    mask = l[0][1:]
    dest = l[2]
    message = ' '.join(l[3:])[1:]
    ###########################################
    #nothing, info, message = command.split(":", 2)
    #info = info.split(' ')
    if dest == weechat.get_info('nick', server):
        source = mask.split("!")[0]
    else:
        source = dest
    return (source, message)

def urlGrabCheckMsgline(server, chan, message):
    # Ignore output from 'tinyurl.py'
    if message.startswith( "[AKA] http://tinyurl.com" ):
        return weechat.PLUGIN_RC_OK
    # Check for URLs
    for word in message.split(" "):
        if word[0:7] == "http://" or \
           word[0:8] == "https://" or \
           word[0:6] == "ftp://":
            urlGrab.addUrl(word, chan, server)

def urlGrabCheck(server, args):
    global urlGrab
    chan, message = urlGrabParsemsg(server, args)
    urlGrabCheckMsgline(server, chan, message)
    return weechat.PLUGIN_RC_OK

def urlGrabCheckOnload():
    for buf in weechat.get_buffer_info().itervalues():
        if len(buf['channel']):
            lines = weechat.get_buffer_data(buf['server'], buf['channel'])
            for line in reversed(lines):
                urlGrabCheckMsgline(buf['server'], buf['channel'], line['data'])

def urlGrabOpen(index, channel = None):
    global urlGrab, urlGrabSettings

    server = weechat.get_info("server")
    if channel is None or channel == "":
        channel = weechat.get_info("channel")
    
    if channel == "":
        urlGrabPrint( "No current channel, you must specify one" )
    elif not urlGrab.hasChannel( channel, server ):
        urlGrabPrint("No URL found - Invalid channel")
    else:
        if index <= 0:
            urlGrabPrint("No URL found - Invalid index")
            return
        url = urlGrab.getUrl(index, channel, server)
        if url == "":
            urlGrabPrint("No URL found - Invalid index")
        else:
            urlGrabPrint("loading %s %sly" % (url, urlGrabSettings.get("method")))
            logfile = os.path.expanduser( urlGrabSettings.get( 'cmdlog' ) )

            argl = urlGrabSettings.createCmdList( url )
            dout = open(logfile, "w")
            dout.write( "UrlGrab: Running '%s'\n" % (" ".join(argl)) )
            dout.close()
            try:
                childpid = os.fork()
            except:
                urlGrabPrint("Fork failed!")
            if childpid == 0:
                logfile = os.path.expanduser( urlGrabSettings.get( 'cmdlog' ) )
                try:
                    subprocess.Popen(
                            argl, 
                            stdin  = open('/dev/null', 'r'),
                            stdout = open(logfile, 'a'),
                            stderr = subprocess.STDOUT)
                except:
                    urlGrabPrint(traceback.format_exc(None))
                sys.exit(0)
            else:
                # In the parent - we may wait for stub to fork again and let the init kill grand-child
                try:
                    os.waitpid(childpid, 0)        
                except:
                    pass
                return

def urlGrabList( args ):
    global urlGrab
    channel = ""
    if len(args) == 0:
        channel = weechat.get_info("channel")
    else:
        channel = args[0]
        
    if channel == "" or channel == "all":
        channel = "*"
    urlGrab.prnt(channel, weechat.get_info("server"))
        
def urlGrabHelp():
    weechat.prnt("")
    urlGrabPrint("Help")
    weechat.prnt(" Usage : ")
    weechat.prnt("    /url help")
    weechat.prnt("        -> display this help")
    weechat.prnt("    /url list [channel]")
    weechat.prnt("        -> display list of recorded urls in the specified channel")
    weechat.prnt("           If no channel is given, lists the current channel")
    weechat.prnt("    /url set [name [[=] value]]")
    weechat.prnt("        -> set or get one of the parameters")
    weechat.prnt("    /url n [channel]")
    weechat.prnt("        -> launch the nth url in `/url list`")
    weechat.prnt("           or the nth url in the specified channel")
    weechat.prnt("")

def urlGrabMain(server, args):
    largs = args.split(" ")
    #strip spaces
    while '' in largs:
        largs.remove('')
    while ' ' in largs:
        largs.remove(' ')
    if len(largs) == 0:
        largs = [urlGrabSettings.get('default')]
    if largs[0] == 'help':
        urlGrabHelp()
    elif largs[0] == 'list':
        urlGrabList( largs[1:] )
    elif largs[0] == 'set':
        try:
            if (len(largs) == 1):
                urlGrabPrint( "Available settings:" )
                urlGrabSettings.prntall()
            elif (len(largs) == 2):
                name = largs[1]
                urlGrabPrint( "Get %s" % name )
                urlGrabSettings.prnt( name )
            elif (len(largs) > 2):
                name = largs[1]
                value = None
                if( largs[2] != "="):
                    value = " ".join(largs[2:])
                elif( largs > 3 and largs[2] == "=" ):
                    value = " ".join(largs[3:])
                urlGrabPrint( "set %s = \'%s\'" % (name, value) )
                if value is not None:
                    try:
                        urlGrabSettings.set( name, value )
                        urlGrabSettings.prnt( name, verbose=False )
                    except ValueError, msg:
                        weechat.prnt( "  Failed: %s" % msg )
                else:
                    weechat.prnt( "  Failed: No value given" )
        except KeyError:
            weechat.prnt( "  Failed: Unrecognized parameter '%s'" % name )
    else:
        try:
            no = int(largs[0])
            if len(largs) > 1:
                urlGrabOpen(no, largs[1])
            else:
                urlGrabOpen(no)
        except ValueError:
            urlGrabPrint( "Unknown command '%s'.  Try '/url help' for usage" % largs[0])
    return weechat.PLUGIN_RC_OK

if weechat.register (UC_NAME, UC_VERSION, "", "Url collector/launcher for weechat"):
    urlGrabSettings = UrlGrabSettings()
    urlGrab = UrlGrabber( urlGrabSettings.get('historysize') )
    urlGrabCheckOnload()
    weechat.add_message_handler("privmsg", "urlGrabCheck")
    weechat.add_command_handler("url", "urlGrabMain", "Controls UrlGrab -> '/url help' for usage")

