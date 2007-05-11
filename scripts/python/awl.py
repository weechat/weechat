"""
  This script shows buffer list in infobar. Nothing more.
  It's inspired by `awl.pl` irssi script, but is less advanced. :)

  The script is in the public domain.
  Leonid Evdokimov (weechat at darkk dot net dot ru)
  http://darkk.net.ru/weechat/awl.py

0.1 - initial commit
0.2 - added `show_servers` option
0.3 - infobar is actually redrawed only if that's necessary
"""

#######################################################################

import weechat
from itertools import ifilter

VERSION = "0.3"
NAME = "awl"
# how often to refresh infobar
timer_interval = 1
blist = ()

def cleanup():
    weechat.remove_infobar(-1)
    return weechat.PLUGIN_RC_OK

def cfg_boolean(key, default = False):
    map = {True: 'ON', False: 'OFF'}
    value = weechat.get_plugin_config(key).upper()
    if not value in map.values():
        if value:
            weechat.prnt("[%s]: invalid %s value (%s), resetting to %s" % (NAME, key, value, map[default]))
        weechat.set_plugin_config(key, map[default])
        value = default
    else:
        value = ifilter(lambda p: p[1] == value, map.iteritems()).next()[0]
    return value

def update_channels():
    global blist
    names = ()
    buffers = weechat.get_buffer_info()
    if buffers != None:
        for index, buffer in buffers.iteritems():
            #**** info for buffer no 8 ****
            # > log_filename, notify_level, server, num_displayed, type, channel
            if len(buffer['channel']):
                name = buffer['channel']
            elif len(buffer['server']):
                if cfg_boolean('show_servers'):
                    name = "[" + buffer['server'] + "]"
                else:
                    continue
            else:
                name = "?"
            names += ("%i:%s" % (index, name), )
    if (names != blist):
        the_string = " ".join(names)
        blist = names
        weechat.remove_infobar(-1)
        weechat.print_infobar(0, the_string);

def on_timer():
    update_channels()
    return weechat.PLUGIN_RC_OK

if weechat.register(NAME, VERSION, "cleanup", "bufferlist in infobar"):
    #message handlers are called __before__ buflist is changed, so we don't use them
    weechat.add_timer_handler(timer_interval, "on_timer")
    cfg_boolean('show_servers', False)
    update_channels()

# vim:set tabstop=4 softtabstop=4 shiftwidth=4:
# vim:set expandtab:
