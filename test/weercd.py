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
# weercd - the WeeChat IRC testing server
#
# It can be used with any IRC client (not only WeeChat).
#
# In the "flood" mode, various IRC commands are sent in a short time (privmsg,
# notice, join/quit, ..) to test client resistance and memory usage (to quickly
# detect memory leaks, for example with client scripts).
#
# This script works with Python 2.x (>= 2.7) and 3.x.
#
# It is *STRONGLY RECOMMENDED* to connect this server with a client in a test
# environment:
# - for WeeChat, another home with: `weechat --dir /tmp/weechat`
# - on a test machine, because CPU will be used a lot by client to display
#   messages from weercd
# - if possible locally (ie server and client on same machine), to speed up data
#   exchange between server and client.
#
# Instructions to use this server with WeeChat:
#   1. open a terminal and run server:
#        python weercd.py
#   2. open another terminal and run WeeChat with home in /tmp:
#        weechat --dir /tmp/weechat
#   3. optional: install script(s) (/script install ...)
#   4. add server and connect to it:
#        /server add weercd 127.0.0.1/7777
#        /connect weercd
#   5. wait some months.....
#      WeeChat still not crashed and does not use 200 TB of RAM ?
#      Yeah, it's stable \o/
#

import argparse
import os
import random
import re
import select
import shlex
import socket
import string
import sys
import time

NAME = 'weercd'
VERSION = '0.7'
DESCRIPTION = 'The WeeChat IRC testing server.'


class Client:
    def __init__(self, sock, addr, args, **kwargs):
        self.sock, self.addr = sock, addr
        self.args = args
        self.nick = ''
        self.nicknumber = 0
        self.channels = {}
        self.lastbuf = ''
        self.incount, self.outcount, self.inbytes, self.outbytes = 0, 0, 0, 0
        self.quit, self.endmsg, self.endexcept = False, '', None
        self.starttime = time.time()
        self.connect()
        if not self.quit:
            if self.args.file:
                self.send_from_file()
            else:
                self.flood()

    def strrand(self, minlength=1, maxlength=50, spaces=False):
        """Return string with random length and content."""
        length = random.randint(minlength, maxlength)
        strspace = ''
        if spaces:
            strspace = ' '
        return ''.join(random.choice(string.ascii_uppercase +
                                     string.ascii_lowercase +
                                     string.digits + strspace) for x in range(length))

    def send(self, data):
        """Send one message to client."""
        if self.args.debug:
            print('<-- %s' % data)
        msg = '%s\r\n' % data
        self.outbytes += len(msg)
        self.sock.send(msg.encode('UTF-8'))
        self.outcount += 1

    def recv(self, data):
        """Read one message from client."""
        if self.args.debug:
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
                while True:
                    pos = data.find('\r\n')
                    if pos < 0:
                        break
                    self.recv(data[0:pos])
                    data = data[pos + 2:]
                self.lastbuf = data

    def connect(self):
        """Tell client that connection is ok."""
        try:
            count = self.args.nickused
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
            rnick = self.channels[channel][random.randint(0, len(self.channels[channel]) - 1)]
        return rnick

    def flood(self):
        """Yay, funny stuff here! Flood client!"""
        if self.args.wait > 0:
            print('Wait %f seconds' % self.args.wait)
            time.sleep(self.args.wait)
        sys.stdout.write('Flooding client..')
        sys.stdout.flush()
        try:
            while not self.quit:
                self.read(self.args.sleep)
                # global actions
                action = random.randint(1, 2)
                if action == 1:
                    # join
                    if len(self.channels) < self.args.maxchans:
                        channel = '#%s' % self.strrand(1, 25)
                        if not channel in self.channels:
                            self.send(':%s!%s JOIN :%s' % (self.nick, self.addr[0], channel))
                            self.send(':%s 353 %s = %s :@%s' % (NAME, self.nick, channel, self.nick))
                            self.send(':%s 366 %s %s :End of /NAMES list.' % (NAME, self.nick, channel))
                            self.channels[channel] = [self.nick]
                elif action == 2 and 'user' in self.args.notice:
                    # notice for user
                    self.send(':%s!%s@%s NOTICE %s :%s' % (self.strrand(1, 10), self.strrand(1, 10),
                                                           self.strrand(1, 10), self.nick,
                                                           self.strrand(1, 400, True)))
                # actions for each channel
                for channel in self.channels:
                    action = random.randint(1, 50)
                    if action >= 1 and action <= 10:
                        # join
                        if len(self.channels[channel]) < self.args.maxnicks:
                            self.nicknumber += 1
                            newnick = '%s%d' % (self.strrand(1, 5), self.nicknumber)
                            self.send(':%s!%s@%s JOIN :%s' % (newnick, self.strrand(1, 10),
                                                              self.strrand(1, 10), channel))
                            self.channels[channel].append(newnick)
                    elif action == 11:
                        # part/quit
                        if len(self.channels[channel]) > 0:
                            rnick = self.chan_randnick(channel)
                            if rnick:
                                command = 'QUIT :%s' % self.strrand(1, 30)
                                if random.randint(1, 2) == 1:
                                    command = 'PART %s' % channel
                                self.send(':%s!%s@%s %s' % (rnick, self.strrand(1, 10),
                                                            self.strrand(1, 10), command))
                                self.channels[channel].remove(rnick)
                    elif action == 12:
                        # kick
                        if len(self.channels[channel]) > 0:
                            rnick1 = self.chan_randnick(channel)
                            rnick2 = self.chan_randnick(channel)
                            if rnick1 and rnick2 and rnick1 != rnick2:
                                self.send(':%s!%s@%s KICK %s %s :%s' % (rnick1, self.strrand(1, 10),
                                                                        self.strrand(1, 10), channel, rnick2,
                                                                        self.strrand(1, 50)))
                                self.channels[channel].remove(rnick2)
                    else:
                        # message
                        if len(self.channels[channel]) > 0:
                            rnick = self.chan_randnick(channel)
                            if rnick:
                                msg = self.strrand(1, 400, True)
                                if 'channel' in self.args.notice and random.randint(1, 100) == 100:
                                    # notice for channel
                                    self.send(':%s!%s@%s NOTICE %s :%s' % (rnick, self.strrand(1, 10),
                                                                           self.strrand(1, 10), channel, msg))
                                else:
                                    # add random highlight
                                    if random.randint(1, 100) == 100:
                                        msg = '%s: %s' % (self.nick, msg)
                                    action2 = random.randint(1, 50)
                                    if action2 == 1:
                                        # action (/me)
                                        msg = '\x01ACTION %s\x01' % msg
                                    elif action2 == 2:
                                        # version
                                        msg = '\x01VERSION\x01'
                                    self.send(':%s!%s@%s PRIVMSG %s :%s' % (rnick, self.strrand(1, 10),
                                                                            self.strrand(1, 10), channel, msg))
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

    def send_from_file(self):
        """Send messages from a file to client."""
        stdin = self.args.file == sys.stdin
        count = 0
        try:
            while True:
                if stdin:
                    sys.stdout.write('Message to send to client: ')
                    sys.stdout.flush()
                message = self.args.file.readline()
                if not message:
                    break
                message = message.rstrip('\n')
                if sys.version_info < (3,):
                    message = message.decode('UTF-8')
                if not message.startswith('//'):
                    if not stdin:
                        # sleep, only if commands come from a file
                        self.read(self.args.sleep)
                    self.send(message.replace('${nick}', self.nick))
                    count += 1
        except IOError as e:
            self.endmsg = 'unable to read file %s' % self.args.file
            self.endexcept = e
            return
        except Exception as e:
            self.endmsg = 'connection lost'
            self.endexcept = e
            return
        except KeyboardInterrupt:
            self.endmsg = 'interrupted'
            return
        finally:
            sys.stdout.write('\n')
            sys.stdout.write('%d messages sent from %s, press Enter to exit'
                             % (count, 'stdin' if stdin else 'file'))
            sys.stdout.flush()
            try:
                sys.stdin.readline()
            except:
                pass

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

# parse command line arguments
parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                 fromfile_prefix_chars='@',
                                 description=DESCRIPTION,
                                 epilog='Note: the environment variable "WEERCD_OPTIONS" can be '
                                 'set with some default options, and argument "@file.txt" can be '
                                 'used to read some default options in a file.')
parser.add_argument('-H', '--host', help='host for socket bind')
parser.add_argument('-p', '--port', type=int, default=7777, help='port for socket bind')
parser.add_argument('-f', '--file', type=argparse.FileType('r'),
                    help='send messages from file, instead of flooding the client (use "-" for stdin)')
parser.add_argument('-c', '--maxchans', type=int, default=5, help='max number of channels to join')
parser.add_argument('-n', '--maxnicks', type=int, default=100, help='max number of nicks per channel')
parser.add_argument('-u', '--nickused', type=int, default=0,
                    help='send 433 (nickname already in use) this number of times before accepting nick')
parser.add_argument('-N', '--notice', metavar='NOTICE_TYPE', choices=['user', 'channel'],
                    default=['user', 'channel'], nargs='*',
                    help='notices to send: "user" (to user), "channel" (to channel)')
parser.add_argument('-s', '--sleep', type=float, default=0,
                    help='sleep for select: delay between 2 messages sent to client (float, in seconds)')
parser.add_argument('-w', '--wait', type=float, default=0,
                    help='time to wait before flooding client (float, in seconds)')
parser.add_argument('-d', '--debug', action='store_true', help='debug output')
parser.add_argument('-v', '--version', action='version', version=VERSION)
args = parser.parse_args(shlex.split(os.getenv('WEERCD_OPTIONS') or '') + sys.argv[1:])

print('%s %s - WeeChat IRC testing server' % (NAME, VERSION))
print('Options: %s' % vars(args))

while True:
    servsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        servsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        servsock.bind((args.host or '', args.port))
        servsock.listen(1)
    except Exception as e:
        print('Socket error: %s' % e)
        sys.exit(1)
    print('Listening on port %s (ctrl-C to exit)' % args.port)
    clientsock = None
    addr = None
    try:
        clientsock, addr = servsock.accept()
    except KeyboardInterrupt:
        servsock.close()
        sys.exit(0)
    print('Connection from %s' % str(addr))
    client = Client(clientsock, addr, args)
    del client
    if args.file:
        break
