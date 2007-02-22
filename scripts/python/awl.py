"""
  This script shows buffer list in infobar. Nothing more.
  It's inspired by `awl.pl` irssi script, but is less advanced. :)

  The script is in the public domain.
  Leonid Evdokimov (weechat at darkk dot net dot ru)
  http://darkk.net.ru/weechat/awl.py
"""

#######################################################################

import weechat

version = "0.1"
# how often to refresh infobar
timer_interval = 1

def cleanup():
    weechat.remove_infobar(-1)
    return weechat.PLUGIN_RC_OK

def update_channels():
    the_string = [];
    buffers = weechat.get_buffer_info()
    if buffers != None:
        for index, buffer in buffers.iteritems():
            #**** info for buffer no 8 ****
            # > log_filename, notify_level, server, num_displayed, type, channel
            if len(buffer['channel']):
                name = buffer['channel']
            elif len(buffer['server']):
                name = "[" + buffer['server'] + "]"
            else:
                name = "?"
            the_string.append("%i:%s" % (index, name))
    the_string = " ".join(the_string)
    weechat.remove_infobar(-1)
    weechat.print_infobar(0, the_string);

def on_timer():
    update_channels()
    return weechat.PLUGIN_RC_OK

if weechat.register("awl", version, "cleanup", "bufferlist in infobar"):
    #message handlers are called __before__ buflist is changed, so we don't use them
    weechat.add_timer_handler(timer_interval, "on_timer")
    update_channels()
