#
# Copyright (c) 2006 by Eric Gach <eric.gach@gmail.com>
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

import weechat
import os
import subprocess
import traceback

__desc__ = 'Amarok control and now playing script for Weechat.'
__version__ = '1.0.2'
__author__ = 'Eric Gach <eric.gach@gmail.com>'
# With changes by Leonid Evdokimov (weechat at darkk dot net another dot ru):
# http://darkk.net.ru/weechat/amarok.py
# v 1.0.1 - added %year%
# v 1.0.2 - fixed bug with dead zombie-childs 
#           fixed bug when loading second instance of the script
#           added better default for dcop_user setting

dcop = {}
debug = {}
infobar = {}
output = {}
ssh = {}

class amarokException(Exception):
    pass

def amarokCommand(server, args):
    try:
        args = args.split(' ')
        if args[0] == 'infobar':
            if infobar['enabled']:
                infobar['enabled'] = False
                weechat.set_plugin_config('infobar_enabled', '0')
                weechat.remove_timer_handler('amarokInfobarUpdate')
                weechat.remove_infobar(0)
                weechat.prnt('Infobar disabled')
            else:
                infobar['enabled'] = True
                weechat.set_plugin_config('infobar_enabled', '1')
                amarokInfobarUpdate()
                weechat.add_timer_handler(infobar['update'], 'amarokInfobarUpdate')
                weechat.prnt('Infobar enabled')
            return weechat.PLUGIN_RC_OK
        elif args[0] == 'next':
            __executeCommand('next')
            weechat.prnt('Amarok: Playing next song.')
            return weechat.PLUGIN_RC_OK
        elif args[0] == 'np':
            return amarokNowPlaying(server)
        elif args[0] == 'pause':
            __executeCommand('pause')
            weechat.prnt('Amarok: Song paused.')
            return weechat.PLUGIN_RC_OK
        elif args[0] == 'play':
            __executeCommand('play')
            weechat.prnt('Amarok: Started playing.')
            return weechat.PLUGIN_RC_OK
        elif args[0] == 'prev':
            __executeCommand('prev')
            weechat.prnt('Amarok: Playing previous song.')
            return weechat.PLUGIN_RC_OK
        elif args[0] == 'stop':
            __executeCommand('stop')
            weechat.prnt('Amarok: Stop playing.')
            return weechat.PLUGIN_RC_OK
        elif args[0] == '':
            return amarokDisplayHelp(server)
        else:
            weechat.prnt('Amarok: Unknown command %s' % (args[0]), '', server)
            return weechat.PLUGIN_RC_OK
    except amarokException, ex:
        return weechat.PLUGIN_RC_KO
    except:
        file = open(debug['file'], 'w')
        traceback.print_exc(None, file)
        weechat.prnt('Unknown Exception encountered. Stack dumped to %s' % (debug['file']), '', server)
        return weechat.PLUGIN_RC_KO

def amarokDisplayHelp(server):
    weechat.prnt('%s - Version: %s' % (__desc__, __version__), '', server)
    weechat.prnt('Author: %s' % (__author__), '', server)
    weechat.prnt('', '', server)
    weechat.prnt('Commands Available', '', server)
    weechat.prnt('  /amarok next    - Move to the next song in the playlist.', '', server)
    weechat.prnt('  /amarok np      - Display currently playing song.', '', server)
    weechat.prnt('  /amarok play    - Start playing music.', '', server)
    weechat.prnt('  /amarok pause   - Toggle between pause/playing.', '', server)
    weechat.prnt('  /amarok prev    - Move to the previous song in the playlist.', '', server)
    weechat.prnt('  /amarok stop    - Stop playing music.', '', server)
    weechat.prnt('  /amarok infobar - Toggle the infobar display.', '', server)
    weechat.prnt('', '', server)
    weechat.prnt('Formatting', '', server)
    weechat.prnt('  %artist%    - Replaced with the song artist.', '', server)
    weechat.prnt('  %title%     - Replaced with the song title.', '', server)
    weechat.prnt('  %album%     - Replaced with the song album.', '', server)
    weechat.prnt('  %year%      - Replaced with the song year tag.', '', server)
    weechat.prnt('  %cTime%     - Replaced with how long the song has been playing.', '', server)
    weechat.prnt('  %tTime%     - Replaced with the length of the song.', '', server)
    weechat.prnt('  %bitrate%   - Replaced with the bitrate of the song.', '', server)
    return weechat.PLUGIN_RC_OK

def amarokInfobarUpdate():
    __loadSettings()
    if infobar['enabled'] == False:
        return weechat.PLUGIN_RC_OK

    isPlaying = __executeCommands((__dcopCommand('isPlaying'),))
    if isPlaying.strip() == 'false':
        weechat.print_infobar(infobar['update'], 'Amarok is not currently playing')
        return weechat.PLUGIN_RC_OK
    else:
        song = __getSongInfo()
        format = __formatNP(infobar['format'], song)
        weechat.print_infobar(infobar['update'], format)
        return weechat.PLUGIN_RC_OK


def amarokNowPlaying(server):
    __loadSettings()
    isPlaying = __executeCommands((__dcopCommand('isPlaying'),))
    if isPlaying.strip() == 'false':
        weechat.prnt('Amarok is not playing.', '', server)
        return weechat.PLUGIN_RC_KO
    else:
        song = __getSongInfo()
        format = __formatNP(output['format'], song)
        weechat.command(format)
        return weechat.PLUGIN_RC_OK

def amarokUnload():
    """Unload the plugin from weechat"""
    if infobar['enabled']:
        weechat.remove_infobar(0)
        weechat.remove_timer_handler('amarokInfobarUpdate')
    return weechat.PLUGIN_RC_OK

def __formatNP(template, song):
    np = template.replace('%artist%', song['artist'])
    np = np.replace('%title%', song['title'])
    np = np.replace('%album%', song['album'])
    np = np.replace('%cTime%', song['cTime'])
    np = np.replace('%tTime%', song['tTime'])
    np = np.replace('%bitrate%', song['bitrate'])
    np = np.replace('%year%', song['year'])
    return np

def __dcopCommand(cmd):
    if dcop['user'] == ':':
        return 'dcop amarok player %s' % (cmd)
    else:
        return 'dcop --user %s amarok player %s' % (dcop['user'], cmd)

def __executeCommands(cmds):
    from subprocess import PIPE
    cmds = " && ".join(cmds)
    if ssh['enabled']:
        cmds = 'ssh -p %d %s@%s "%s"' % (ssh['port'], ssh['user'], ssh['host'], cmds)
    proc = subprocess.Popen(cmds, shell = True, stderr = PIPE, stdout = PIPE, close_fds = True)
    error = proc.stderr.read()
    if error != '':
        weechat.prnt(error)
    output = proc.stdout.read()
    proc.wait()
    return output

def __getSongInfo():
    """Get the song information from amarok"""
    song = {}
    songs = __executeCommands(
        (
            __dcopCommand('artist'),
            __dcopCommand('title'),
            __dcopCommand('album'),
            __dcopCommand('currentTime'),
            __dcopCommand('totalTime'),
            __dcopCommand('bitrate'),
            __dcopCommand('year')
        )
    )

    song['artist'], song['title'], song['album'], song['cTime'], song['tTime'], song['bitrate'], song['year'], empty = songs.split("\n")
    return song

def __loadSettings():
    dcop['user'] = __loadSetting('dcop_user', ':')
    debug['file'] = os.path.expanduser(__loadSetting('debug_file', '~/amarok_debug.txt'))
    infobar['enabled'] = __loadSetting('infobar_enabled', '0', 'bool')
    infobar['format'] = __loadSetting('infobar_format', 'Now Playing: %title% by %artist%')
    infobar['update'] = __loadSetting('infobar_update', '10', 'int')
    output['format'] = __loadSetting('output_format', '/me is listening to %C04%title%%C by %C03%artist%%C from %C12%album%%C [%cTime% of %tTime% @ %bitrate%kbps]')
    ssh['enabled'] = __loadSetting('ssh_enabled', '0', 'bool')
    ssh['host'] = __loadSetting('ssh_host', 'localhost')
    ssh['port'] = __loadSetting('ssh_port', '22', 'int')
    ssh['user'] = __loadSetting('ssh_user', 'user')

def __loadSetting(setting, default=None, type=None):
    value = weechat.get_plugin_config(setting)
    if value == '' and default != None:
        weechat.set_plugin_config(setting, default)
        value = default

    if type == 'int' or type == 'bool':
        value = int(value)
    if type == 'bool':
        value = bool(value)

    return value

if weechat.register('amarok', __version__, 'amarokUnload', __desc__):
    __loadSettings()
    if infobar['enabled']:
        amarokInfobarUpdate()
        weechat.add_timer_handler(infobar['update'], 'amarokInfobarUpdate')
    weechat.add_command_handler('amarok', 'amarokCommand', 'Manage amarok or display now playing information.', 'next|np|play|pause|prev|stop|infobar')

