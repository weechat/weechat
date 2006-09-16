# --------------------------------------------------------------------
#
# Copyright (c) 2006 by Gwenn Gueguen <weechat@grumly.info>
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
# --------------------------------------------------------------------
# This script automatically sets away after a period of inactivity
# --------------------------------------------------------------------


import weechat


"""
/autoaway [time [message]]

/autoaway whithout any parameter disables autoaway

Script config:
  time:
    number of minutes of inactivity after which a user will be marked as away
  message:
    message that will be passed to the /away command
  enabled:
    is autoaway enabled ?
"""


SCRIPT_NAME="autoaway"
SCRIPT_VERSION="0.2"
SCRIPT_DESC="autoaway script for weechat"


# Names of the settings
CONFIG_TIME="time"
CONFIG_MESSAGE="message"
CONFIG_ENABLED="enabled"

# Name of the command
CMD_AUTOAWAY="autoaway"

# Default settings
DEFAULT_TIME="15"
DEFAULT_MESSAGE="idle"

# Interval (in seconds) between checks
TIMER_VALUE=15


def print_settings():
    weechat.prnt("AutoAway settings:")
    weechat.prnt("  time: %s minute(s)" % weechat.get_plugin_config(CONFIG_TIME))
    weechat.prnt("  message: %s" % weechat.get_plugin_config(CONFIG_MESSAGE))
    if weechat.get_plugin_config(CONFIG_ENABLED) == "true":
        weechat.prnt("  enabled")
    else:
        weechat.prnt("  disabled")


def autoaway(server, args):
    weechat.remove_timer_handler("timer_handler")

    params = args.split(None, 1)
    if len(params) == 0:
        weechat.set_plugin_config(CONFIG_ENABLED, "false")
    else:
        weechat.set_plugin_config(CONFIG_ENABLED, "true")
        weechat.set_plugin_config(CONFIG_TIME, params[0])
        if len(params) > 1:
            weechat.set_plugin_config(CONFIG_MESSAGE, params[1])

        previous_inactivity = int(weechat.get_info("inactivity"))
        weechat.add_timer_handler(TIMER_VALUE, "timer_handler")

    print_settings()

    return weechat.PLUGIN_RC_OK


def timer_handler():
    global previous_inactivity, previous_state

    # Get current away status
    away_flag = int(weechat.get_info("away"))

    # Get number of seconds of inactivity
    idle_time = int(weechat.get_info("inactivity"))

    if away_flag == previous_state:
        # away flag was not changed outside this script
        if away_flag and idle_time < previous_inactivity:
            # Inactivity was reset (or overflowed ?)
            weechat.command("/away -all")
        elif not away_flag and idle_time >= (60 * int(weechat.get_plugin_config(CONFIG_TIME))):
            # Time to go away
            weechat.command("/away -all %s" % weechat.get_plugin_config(CONFIG_MESSAGE))

    previous_state = int(weechat.get_info("away"))
    previous_inactivity = idle_time

    return weechat.PLUGIN_RC_OK


if weechat.register(SCRIPT_NAME, SCRIPT_VERSION, "", SCRIPT_DESC):

    try:
        previous_state = int(weechat.get_info("away"))
    except ValueError:
        previous_state = 0

    # Set config to default values if undefined
    try:
        idle_time = int(weechat.get_plugin_config(CONFIG_TIME))
    except ValueError:
        weechat.set_plugin_config(CONFIG_TIME, DEFAULT_TIME)
    
    if weechat.get_plugin_config(CONFIG_MESSAGE) == None:
        weechat.set_plugin_config(CONFIG_MESSAGE, DEFAULT_MESSAGE)
    
    if weechat.get_plugin_config(CONFIG_ENABLED) == None:
        weechat.set_plugin_config(CONFIG_ENABLED, "false")


    # Display a summary of the settings
    print_settings()


    # Start the timer if necessary
    if weechat.get_plugin_config(CONFIG_ENABLED) == "true":
        previous_inactivity = int(weechat.get_info("inactivity"))
        weechat.add_timer_handler(TIMER_VALUE, "timer_handler")


    weechat.add_command_handler(CMD_AUTOAWAY, "autoaway", "Set autoaway", 
      "[time [message]]",
      "time: number of minutes before being marked as away\n"
      + "message: away message\n"
      + "\n"
      + "whithout any argument, autoaway will be disabled\n")

