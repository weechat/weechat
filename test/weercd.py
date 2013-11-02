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
# - if possible locally (ie server and client on same machine), to speed up
#   data exchange between server and client.
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

from __future__ import division, print_function

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
import traceback

NAME = 'weercd'
VERSION = '0.8'


class Client:

    def __init__(self, sock, addr, args, **kwargs):
        self.sock, self.addr = sock, addr
        self.args = args
        self.name = NAME
        self.version = VERSION
        self.nick = ''
        self.nicknumber = 0
        self.channels = {}
        self.lastbuf = ''
        self.incount, self.outcount, self.inbytes, self.outbytes = 0, 0, 0, 0
        self.quit, self.endmsg, self.endexcept = False, '', None
        self.starttime = time.time()
        self.connect()

    def run(self):
        """Execute the action asked for the client."""
        if self.quit:
            return

        # send commands from file (which can be stdin)
        if self.args.file:
            self.send_file()
            return

        # flood the client
        if self.args.wait > 0:
            print('Waiting', self.args.wait, 'seconds')
            time.sleep(self.args.wait)
        sys.stdout.write('Flooding client..')
        sys.stdout.flush()
        try:
            while not self.quit:
                self.flood()
        except Exception as e:
            if self.quit:
                self.endmsg = 'quit received'
            else:
                self.endmsg = 'connection lost'
            self.endexcept = e
        except KeyboardInterrupt:
            self.endmsg = 'interrupted'
        else:
            self.endmsg = 'quit received'

    def fuzzy_str(self, minlength=1, maxlength=50, spaces=False):
        """Return a fuzzy string (random length and content)."""
        length = random.randint(minlength, maxlength)
        strspace = ''
        if spaces:
            strspace = ' '
        return ''.join(random.choice(string.ascii_uppercase +
                                     string.ascii_lowercase +
                                     string.digits + strspace)
                       for x in range(length))

    def fuzzy_host(self):
        """Return a fuzzy host name."""
        return '{0}@{1}'.format(self.fuzzy_str(1, 10), self.fuzzy_str(1, 10))

    def fuzzy_nick(self, with_number=False):
        """Return a fuzzy nick name."""
        if with_number:
            self.nicknumber += 1
            return '{0}{1}'.format(self.fuzzy_str(1, 5), self.nicknumber)
        else:
            return self.fuzzy_str(1, 10)

    def fuzzy_chan(self):
        """Return a fuzzy channel name."""
        return '#{0}'.format(self.fuzzy_str(1, 25))

    def send(self, data):
        """Send one message to client."""
        if self.args.debug:
            print('<--', data)
        msg = data + '\r\n'
        self.outbytes += len(msg)
        self.sock.send(msg.encode('UTF-8'))
        self.outcount += 1

    def send_cmd(self, cmd, data, nick='{self.name}', host='',
                 target='{self.nick}'):
        """Send an IRC command to the client."""
        self.send(':{0}{1}{2} {3}{4}{5}{6}{7}'
                  ''.format(nick,
                            '!' if host else '',
                            host,
                            cmd,
                            ' ' if target else '',
                            target,
                            ' :' if data else '',
                            data).format(self=self))

    def recv(self, data):
        """Read one IRC message from client."""
        if self.args.debug:
            print('-->', data)
        if data.startswith('PING '):
            args = data[5:]
            if args[0] == ':':
                args = args[1:]
            self.send('PONG :{0}'.format(args))
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
        """Read raw data received from client."""
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
        """Inform the client that the connection is OK."""
        try:
            count = self.args.nickused
            while self.nick == '':
                self.read(0.1)
                if self.nick and count > 0:
                    self.send_cmd('433', 'Nickname is already in use.',
                                  target='* {self.nick}')
                    self.nick = ''
                    count -= 1
            self.send_cmd('001', 'Welcome to the WeeChat IRC server')
            self.send_cmd('002', 'Your host is {self.name}, running version '
                          '{self.version}')
            self.send_cmd('003', 'Are you solid like a rock?')
            self.send_cmd('004', 'Let\'s see!')
        except KeyboardInterrupt:
            self.quit = True
            self.endmsg = 'interrupted'
            return

    def channel_random_nick(self, channel):
        """Return a random nick of a channel."""
        if len(self.channels[channel]) < 2:
            return None
        rnick = self.nick
        while rnick == self.nick:
            rnick = self.channels[channel][
                random.randint(0, len(self.channels[channel]) - 1)]
        return rnick

    def flood_self_join(self):
        """Self join on a new channel."""
        channel = self.fuzzy_chan()
        if channel in self.channels:
            return
        self.send_cmd('JOIN', channel,
                      nick=self.nick, host=self.addr[0], target='')
        self.send_cmd('353', '@{self.nick}',
                      target='{0} = {1}'.format(self.nick, channel))
        self.send_cmd('366', 'End of /NAMES list.',
                      target='{0} {1}'.format(self.nick, channel))
        self.channels[channel] = [self.nick]

    def flood_user_notice(self):
        """Notice for the user."""
        self.send_cmd('NOTICE', self.fuzzy_str(1, 400, spaces=True),
                      nick=self.fuzzy_nick(), host=self.fuzzy_host())

    def flood_channel_join(self, channel):
        """Join of a user in a channel."""
        if len(self.channels[channel]) >= self.args.maxnicks:
            return
        newnick = self.fuzzy_nick(with_number=True)
        self.send_cmd('JOIN', channel,
                      nick=newnick, host=self.fuzzy_host(), target='')
        self.channels[channel].append(newnick)

    def flood_channel_part(self, channel):
        """Part or quit of a user in a channel."""
        if len(self.channels[channel]) == 0:
            return
        rnick = self.channel_random_nick(channel)
        if not rnick:
            return
        if random.randint(1, 2) == 1:
            self.send_cmd('PART', channel,
                          nick=rnick, host=self.fuzzy_host(), target='')
        else:
            self.send_cmd('QUIT', self.fuzzy_str(1, 30),
                          nick=rnick, host=self.fuzzy_host(), target='')
        self.channels[channel].remove(rnick)

    def flood_channel_kick(self, channel):
        """Kick of a user in a channel."""
        if len(self.channels[channel]) == 0:
            return
        rnick1 = self.channel_random_nick(channel)
        rnick2 = self.channel_random_nick(channel)
        if rnick1 and rnick2 and rnick1 != rnick2:
            self.send_cmd('KICK', self.fuzzy_str(1, 50),
                          nick=rnick1, host=self.fuzzy_host(),
                          target='{0} {1}'.format(channel, rnick2))
            self.channels[channel].remove(rnick2)

    def flood_channel_message(self, channel):
        """Message from a user in a channel."""
        if len(self.channels[channel]) == 0:
            return
        rnick = self.channel_random_nick(channel)
        if not rnick:
            return
        msg = self.fuzzy_str(1, 400, spaces=True)
        if 'channel' in self.args.notice and random.randint(1, 100) == 100:
            # notice for channel
            self.send_cmd('NOTICE', msg,
                          nick=rnick, host=self.fuzzy_host(), target=channel)
        else:
            # add random highlight
            if random.randint(1, 100) == 100:
                msg = '{0}: {1}'.format(self.nick, msg)
            action2 = random.randint(1, 50)
            if action2 == 1:
                # CTCP action (/me)
                msg = '\x01ACTION {0}\x01'.format(msg)
            elif action2 == 2:
                # CTCP version
                msg = '\x01VERSION\x01'
            self.send_cmd('PRIVMSG', msg,
                          nick=rnick, host=self.fuzzy_host(), target=channel)

    def flood(self):
        """Yay, funny stuff here! Flood the client!"""
        self.read(self.args.sleep)
        # global actions
        action = random.randint(1, 2)
        if action == 1 and len(self.channels) < self.args.maxchans:
            self.flood_self_join()
        elif action == 2 and 'user' in self.args.notice:
            self.flood_user_notice()
        # actions for each channel
        for channel in self.channels:
            action = random.randint(1, 50)
            if 1 <= action <= 10:
                self.flood_channel_join(channel)
            elif action == 11:
                self.flood_channel_part(channel)
            elif action == 12:
                self.flood_channel_kick(channel)
            else:
                self.flood_channel_message(channel)
        # display progress
        if self.outcount % 1000 == 0:
            sys.stdout.write('.')
            sys.stdout.flush()

    def send_file(self):
        """Send messages from a file to client."""
        stdin = self.args.file == sys.stdin
        count = 0
        self.read(0.2)
        try:
            while True:
                # display the prompt if we are reading in stdin
                if stdin:
                    sys.stdout.write('Message to send to client: ')
                    sys.stdout.flush()
                message = self.args.file.readline()
                if not message:
                    break
                if sys.version_info < (3,):
                    message = message.decode('UTF-8')
                message = message.rstrip('\n')
                if message and not message.startswith('//'):
                    self.send(message.format(self=self))
                    count += 1
                self.read(0.1 if stdin else self.args.sleep)
        except IOError as e:
            self.endmsg = 'unable to read file {0}'.format(self.args.file)
            self.endexcept = e
            return
        except Exception as e:
            traceback.print_exc()
            self.endmsg = 'connection lost'
            return
        except KeyboardInterrupt:
            self.endmsg = 'interrupted'
            return
        finally:
            sys.stdout.write('\n')
            sys.stdout.write('{0} messages sent from {1}, press Enter to exit'
                             .format(count, 'stdin' if stdin else 'file'))
            sys.stdout.flush()
            try:
                sys.stdin.readline()
            except:
                pass

    def stats(self):
        """Display some statistics about data exchanged with the client."""
        msgexcept = ''
        if self.endexcept:
            msgexcept = '({0})'.format(self.endexcept)
        print(self.endmsg, msgexcept)
        elapsed = time.time() - self.starttime
        countrate = self.outcount / elapsed
        bytesrate = self.outbytes / elapsed
        print('Elapsed: {elapsed:.1f}s - '
              'packets: in:{self.incount}, out:{self.outcount} '
              '({countrate:.0f}/s) - '
              'bytes: in:{self.inbytes}, out: {self.outbytes} '
              '({bytesrate:.0f}/s)'
              ''.format(self=self,
                        elapsed=elapsed,
                        countrate=countrate,
                        bytesrate=bytesrate))
        if self.endmsg == 'connection lost':
            print('Uh-oh! No quit received, client has crashed? Ahah \o/')

    def __del__(self):
        self.stats()
        print('Closing connection with', self.addr)
        self.sock.close()

if __name__ == "__main__":
    # parse command line arguments
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        fromfile_prefix_chars='@',
        description='The WeeChat IRC testing server.',
        epilog='Note: the environment variable "WEERCD_OPTIONS" can be '
        'set with default options. Argument "@file.txt" can be used to read '
        'default options in a file.')
    parser.add_argument('-H', '--host', help='host for socket bind')
    parser.add_argument('-p', '--port', type=int, default=7777,
                        help='port for socket bind')
    parser.add_argument('-f', '--file', type=argparse.FileType('r'),
                        help='send messages from file, instead of flooding '
                        'the client (use "-" for stdin)')
    parser.add_argument('-c', '--maxchans', type=int, default=5,
                        help='max number of channels to join')
    parser.add_argument('-n', '--maxnicks', type=int, default=100,
                        help='max number of nicks per channel')
    parser.add_argument('-u', '--nickused', type=int, default=0,
                        help='send 433 (nickname already in use) this number '
                        'of times before accepting nick')
    parser.add_argument('-N', '--notice', metavar='NOTICE_TYPE',
                        choices=['user', 'channel'],
                        default=['user', 'channel'], nargs='*',
                        help='notices to send: "user" (to user), "channel" '
                        '(to channel)')
    parser.add_argument('-s', '--sleep', type=float, default=0,
                        help='sleep for select: delay between 2 messages sent '
                        'to client (float, in seconds)')
    parser.add_argument('-w', '--wait', type=float, default=0,
                        help='time to wait before flooding client (float, '
                        'in seconds)')
    parser.add_argument('-d', '--debug', action='store_true',
                        help='debug output')
    parser.add_argument('-v', '--version', action='version', version=VERSION)
    args = parser.parse_args(shlex.split(os.getenv('WEERCD_OPTIONS') or '') +
                             sys.argv[1:])

    # welcome message, with options
    print(NAME, VERSION, '- WeeChat IRC testing server')
    print('Options:', vars(args))

    # main loop
    while True:
        servsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            servsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            servsock.bind((args.host or '', args.port))
            servsock.listen(1)
        except Exception as e:
            print('Socket error: {0}'.format(e))
            sys.exit(1)
        print('Listening on port', args.port, '(ctrl-C to exit)')
        clientsock = None
        addr = None
        try:
            clientsock, addr = servsock.accept()
        except KeyboardInterrupt:
            servsock.close()
            sys.exit(0)
        print('Connection from', addr)
        client = Client(clientsock, addr, args)
        client.run()
        del client
        # no loop if message were sent from a file
        if args.file:
            break
