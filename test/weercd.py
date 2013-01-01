#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2011-2013 Sebastien Helleu <flashcode@flashtux.org>
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

#
# weercd - the WeeChat IRC server for testing purposes
#
# weercd is an IRC server that is designed to test client resistance and memory
# usage (quickly detect memory leaks, for example with client scripts).
# Various IRC commands are sent in a short time (privmsg, notice, join/quit, ..)
#
# This script works with Python 2.x and 3.x.
#
# It is *STRONGLY RECOMMENDED* to connect this server with a client in a test
# environment:
# - for WeeChat, another home with: `weechat-curses --dir /tmp/weechat`
# - on a test machine, because CPU will be used a lot by client to display
#   messages from weercd
# - if possible locally (ie server and client on same machine), to speed up data
#   exchange between server and client.
#
# Instructions to use this server with WeeChat:
#   1. open a terminal and run server:
#        python weercd.py
#   2. open another terminal and run WeeChat with home in /tmp:
#        weechat-curses --dir /tmp/weechat
#   3. optional: install script(s) in /tmp/weechat/<language>/autoload/
#   4. add server and connect to it:
#        /server add weercd 127.0.0.1/7777
#        /connect weercd
#   5. wait some months.....
#      WeeChat still not crashed and does not use 200 TB of RAM ?
#      Yeah, it's stable \o/
#

import sys, socket, select, time, random, string, re

NAME    = 'weercd'
VERSION = '0.6'

options = {
    'host'       : ['',      'Host for socket bind'],
    'port'       : ['7777',  'Port for socket bind'],
    'debug'      : ['off',   'Debug (on/off)'],
    'action'     : ['flood', 'Action of server: "flood" = flood client, "user" = type messages to send to client, "file" = messages sent from a file'],
    'wait'       : ['0',     'Time to wait before flooding client (float, in seconds)'],
    'sleep'      : ['0',     'Sleep for select, delay between 2 messages sent to client (float, in seconds)'],
    'nickused'   : ['0',     'Send 433 (nickname already in use) this number of times before accepting nick'],
    'maxchans'   : ['5',     'Max channels to join'],
    'maxnicks'   : ['100',   'Max nicks per channel'],
    'usernotices': ['on',    'Send notices to user (on/off)'],
    'channotices': ['on',    'Send notices to channels (on/off)'],
    'file'       : ['',      'Filename used for sending messages to client (only for action "file")'],
}

def usage():
    """Display usage."""
    global options
    print('\nUsage: %s [option=value [option=value ...]] | [-h | --help]\n' % sys.argv[0])
    print('  option=value  option with value (see table below)')
    print('  -h, --help    display this help\n')
    print('Options (name, default value, description):\n')
    for key in options:
        v = '"%s"' % options[key][0]
        print('  %-12s %-10s %s' % (key, v, options[key][1]))
    print('\nOptions can be written in %s.conf, with format for each line: option=value\n' % NAME)
    sys.exit(0)

def setoption(string):
    """Set option with string using format "option=value"."""
    global options
    items = string.strip().split('=', 1)
    if len(items) == 2:
        key = items[0].strip()
        if not key.startswith('#'):
            value = items[1].strip()
            if key in options:
                options[key][0] = value
            else:
                print('WARNING: unknown option "%s"' % key)

def getoption(option):
    """Get value of an option."""
    global options
    if option in options:
        return options[option][0]
    return None

def getdictoptions():
    """Get dict with options and values."""
    global options
    d = {}
    for key in options:
        d[key] = options[key][0]
    return d

def readconfig(filename):
    """Read configuration file."""
    try:
        lines = open(filename, 'rb').readlines()
        for line in lines:
            setoption(str(line.decode('utf-8')))
    except:
        pass

def strrand(minlength=1, maxlength=50, spaces=False):
    """Return string with random lenght and content."""
    length = random.randint(minlength, maxlength)
    strspace = ''
    if spaces:
        strspace = ' '
    return ''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits + strspace) for x in range(length))

class Client:
    def __init__(self, sock, addr, **kwargs):
        self.sock, self.addr = sock, addr
        self.action = getoption('action')
        self.nick = ''
        self.nicknumber = 0
        self.channels = {}
        self.lastbuf = ''
        self.incount, self.outcount, self.inbytes, self.outbytes = 0, 0, 0, 0
        self.quit, self.endmsg, self.endexcept = False, '', None
        self.sleep = float(getoption('sleep'))
        self.nickused = int(getoption('nickused'))
        self.maxchans = int(getoption('maxchans'))
        self.maxnicks = int(getoption('maxnicks'))
        self.usernotices = (getoption('usernotices') == 'on')
        self.channotices = (getoption('channotices') == 'on')
        self.starttime = time.time()
        self.file = None
        if self.action == 'file':
            self.filename = getoption('file')
            if not self.filename:
                print('Error: please specify file name with option "file=..."')
                return
            try:
                self.file = open(self.filename, 'r')
            except IOError:
                print('Error: unable to open file "%s"' % self.filename)
                return
        self.connect()
        if not self.quit:
            if self.action == 'flood':
                self.action_flood()
            elif self.action == 'user':
                self.action_user()
            elif self.action == 'file':
                self.action_file()
            else:
                print('Unknown action: "%s"' % self.action)
                return

    def send(self, data):
        """Send one message to client."""
        if getoption('debug') == 'on':
            print('<-- %s' % data)
        msg = '%s\r\n' % data
        self.outbytes += len(msg)
        self.sock.send(msg.encode('UTF-8'))
        self.outcount += 1

    def recv(self, data):
        """Read one message from client."""
        if getoption('debug') == 'on':
            print('--> %s' % data)
        if data.startswith('PING '):
            args = data[5:]
            if args[0] == ':':
                args = args[1:]
            self.send('PONG :%s' % args)
        elif data.startswith('NICK '):
            self.nick = data[5:]
        elif data.startswith('PART '):
            m = re.search('^PART :?(#[^ ]+)', data)
            if m:
                channel = m.group(1)
                if channel in self.channels:
                    del self.channels[channel]
        elif data.startswith('QUIT '):
            self.quit = True
        self.incount += 1

    def read(self, timeout):
        """Read data from client."""
        inr, outr, exceptr = select.select([self.sock], [], [], timeout)
        if inr:
            data = self.sock.recv(4096)
            if data:
                data = data.decode('UTF-8')
                self.inbytes += len(data)
                data = self.lastbuf + data
                while 1:
                    pos = data.find('\r\n')
                    if pos < 0:
                        break
                    self.recv(data[0:pos])
                    data = data[pos+2:]
                self.lastbuf = data

    def connect(self):
        """Tell client that connection is ok."""
        try:
            count = self.nickused
            while self.nick == '':
                self.read(0.1)
                if self.nick and count > 0:
                    self.send(':%s 433 * %s :Nickname is already in use.' % (NAME, self.nick))
                    self.nick = ''
                    count -= 1
            self.send(':%s 001 %s :Welcome to the WeeChat IRC server' % (NAME, self.nick))
            self.send(':%s 002 %s :Your host is %s, running version %s' % (NAME, self.nick, NAME, VERSION))
            self.send(':%s 003 %s :Are you solid like a rock?' % (NAME, self.nick))
            self.send(':%s 004 %s :Let\'s see!' % (NAME, self.nick))
        except KeyboardInterrupt:
            self.quit = True
            self.endmsg = 'interrupted'
            return

    def chan_randnick(self, channel):
        if len(self.channels[channel]) < 2:
            return None
        rnick = self.nick
        while rnick == self.nick:
            rnick = self.channels[channel][random.randint(0, len(self.channels[channel])-1)]
        return rnick

    def action_flood(self):
        """Yay, funny stuff here! Flood client!"""
        wait = int(getoption('wait'))
        if wait > 0:
            print('Wait %d seconds' % wait)
            time.sleep(wait)
        sys.stdout.write('Flooding client..')
        sys.stdout.flush()
        try:
            while not self.quit:
                self.read(self.sleep)
                # global actions
                action = random.randint(1, 2)
                if action == 1:
                    # join
                    if len(self.channels) < self.maxchans:
                        channel = '#%s' % strrand(1, 25)
                        if not channel in self.channels:
                            self.send(':%s!%s JOIN :%s' % (self.nick, self.addr[0], channel))
                            self.send(':%s 353 %s = %s :@%s' % (NAME, self.nick, channel, self.nick))
                            self.send(':%s 366 %s %s :End of /NAMES list.' % (NAME, self.nick, channel))
                            self.channels[channel] = [self.nick]
                elif action == 2 and self.usernotices:
                    # notice
                    self.send(':%s!%s@%s NOTICE %s :%s' % (
                            strrand(1, 10), strrand(1, 10), strrand(1, 10), self.nick, strrand(1, 400, True)))
                # actions for each channel
                for channel in self.channels:
                    action = random.randint(1, 50)
                    if action >= 1 and action <= 10:
                        # join
                        if len(self.channels[channel]) < self.maxnicks:
                            self.nicknumber += 1
                            newnick = '%s%d' % (strrand(1, 5), self.nicknumber)
                            self.send(':%s!%s@%s JOIN :%s' % (newnick, strrand(1, 10), strrand(1, 10), channel))
                            self.channels[channel].append(newnick)
                    elif action == 11:
                        # part/quit
                        if len(self.channels[channel]) > 0:
                            rnick = self.chan_randnick(channel)
                            if rnick:
                                command = 'QUIT :%s' % strrand(1, 30)
                                if random.randint(1, 2) == 1:
                                    command = 'PART %s' % channel
                                self.send(':%s!%s@%s %s' % (rnick, strrand(1, 10), strrand(1, 10), command))
                                self.channels[channel].remove(rnick)
                    elif action == 12:
                        # kick
                        if len(self.channels[channel]) > 0:
                            rnick1 = self.chan_randnick(channel)
                            rnick2 = self.chan_randnick(channel)
                            if rnick1 and rnick2 and rnick1 != rnick2:
                                self.send(':%s!%s@%s KICK %s %s :%s' % (rnick1, strrand(1, 10), strrand(1, 10), channel, rnick2, strrand (1, 50)))
                                self.channels[channel].remove(rnick2)
                    else:
                        # message
                        if len(self.channels[channel]) > 0:
                            rnick = self.chan_randnick(channel)
                            if rnick:
                                msg = strrand(1, 400, True)
                                if self.channotices and random.randint(1,100) == 100:
                                    # notice
                                    self.send(':%s!%s@%s NOTICE %s :%s' % (rnick, strrand(1, 10), strrand (1, 10), channel, msg))
                                else:
                                    # add random highlight
                                    if random.randint(1, 100) == 100:
                                        msg = '%s: %s' % (self.nick, msg)
                                    action2 = random.randint(1,50)
                                    if action2 == 1:
                                        # action (/me)
                                        msg = '\x01ACTION %s\x01' % msg
                                    elif action2 == 2:
                                        # version
                                        msg = '\x01VERSION\x01'
                                    self.send(':%s!%s@%s PRIVMSG %s :%s' % (rnick, strrand(1, 10), strrand(1, 10), channel, msg))
                # display progress
                if self.outcount % 1000 == 0:
                    sys.stdout.write('.')
                    sys.stdout.flush()
        except Exception as e:
            if self.quit:
                self.endmsg = 'quit received'
            else:
                self.endmsg = 'connection lost'
            self.endexcept = e
            return
        except KeyboardInterrupt:
            self.endmsg = 'interrupted'
            return
        else:
            self.endmsg = 'quit received'
            return

    def action_user(self):
        """User enters messages to send to client."""
        try:
            while 1:
                sys.stdout.write('Message to send to client: ')
                sys.stdout.flush()
                message = sys.stdin.readline()
                self.send(message)
        except Exception as e:
            self.endmsg = 'connection lost'
            self.endexcept = e
            return
        except KeyboardInterrupt:
            self.endmsg = 'interrupted'
            return

    def action_file(self):
        """Send messages from a file to client."""
        try:
            count = 0
            for line in self.file:
                if not line.startswith('//'):
                    self.read(self.sleep)
                    self.send(line.replace('${nick}', self.nick))
                    count += 1
            self.file.close()
            sys.stdout.write('%d messages sent, press Enter to restart server' % count)
            sys.stdout.flush()
            sys.stdin.readline()
        except IOError as e:
            self.endmsg = 'unable to open file "%s"' % self.filename
            self.endexcept = e
            return
        except Exception as e:
            self.endmsg = 'connection lost'
            self.endexcept = e
            return
        except KeyboardInterrupt:
            self.endmsg = 'interrupted'
            return

    def stats(self):
        msgexcept = ''
        if self.endexcept:
            msgexcept = '(%s)' % self.endexcept
        print(' %s %s' % (self.endmsg, msgexcept))
        elapsed = time.time() - self.starttime
        print('Elapsed: %.1fs - packets: in:%d, out:%d (%d/s) - bytes: in:%d, out: %d (%d/s)' % (
            elapsed, self.incount, self.outcount, self.outcount / elapsed,
            self.inbytes, self.outbytes, self.outbytes / elapsed))
        if self.endmsg == 'connection lost':
            print('Uh-oh! No quit received, client has crashed? Ahah \o/')

    def __del__(self):
        self.stats()
        print('Closing connection with %s' % str(self.addr))
        self.sock.close()

def main():
    if len(sys.argv) > 1 and (sys.argv[1] == '-h' or sys.argv[1] == '--help'):
        usage()
    readconfig('%s.conf' % NAME)
    for arg in sys.argv:
        setoption(arg)
    print('%s %s - WeeChat IRC server' % (NAME, VERSION))
    while 1:
        print('Options: %s' % getdictoptions())
        print('Listening on port %s (ctrl-C to exit)' % getoption('port'))
        servsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        servsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        servsock.bind((getoption('host'), int(getoption('port'))))
        servsock.listen(1)
        clientsock = None
        addr = None
        try:
            clientsock, addr = servsock.accept()
        except KeyboardInterrupt:
            servsock.close()
            return
        print('Connection from %s' % str(addr))
        client = Client(clientsock, addr)
        del client

if __name__ == '__main__':
    main()
