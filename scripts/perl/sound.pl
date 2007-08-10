#
# Copyright (c) 2006-2007 by FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Play a sound when highlighted/private msg, or for ctcp sound event.
#
# History:
#
# 2007-08-10, FlashCode <flashcode@flashtux.org>:
#     version 0.4: upgraded licence to GPL 3
# 2006-05-30, FlashCode <flashcode@flashtux.org>:
#     added plugin options for commands
# 2004-10-01, FlashCode <flashcode@flashtux.org>:
#     initial release
#

use strict;

my $version = "0.4";
my $command_suffix = " >/dev/null 2>&1 &";

# default values in setup file (~/.weechat/plugins.rc)
my $default_cmd_highlight = "alsaplay -i text ~/sound_highlight.wav";
my $default_cmd_pv        = "alsaplay -i text ~/sound_pv.wav";
my $default_cmd_ctcp      = "alsaplay -i text \$filename";

weechat::register("Sound", $version, "", "Sound for highlights/privates & CTCP sound events");
weechat::set_plugin_config("cmd_highlight", $default_cmd_highlight) if (weechat::get_plugin_config("cmd_highlight") eq "");
weechat::set_plugin_config("cmd_pv", $default_cmd_pv) if (weechat::get_plugin_config("cmd_pv") eq "");
weechat::set_plugin_config("cmd_ctcp", $default_cmd_ctcp) if (weechat::get_plugin_config("cmd_ctcp") eq "");

weechat::add_message_handler("PRIVMSG", "sound");
weechat::add_message_handler("weechat_highlight", "highlight");
weechat::add_message_handler("weechat_pv", "pv");
weechat::add_command_handler("sound", "sound_cmd");

sub sound
{
    my $server = $_[0];
    if ($_[1] =~ /(.*) PRIVMSG (.*)/)
    {
        my ($host, $msg) = ($1, $2);
	if ($host ne "localhost")
	{
            if ($msg =~ /\001SOUND ([^ ]*)\001/)
            {
                my $filename = $1;
                my $command = weechat::get_plugin_config("cmd_ctcp");
                $command =~ s/(\$\w+)/$1/gee;
                system($command.$command_suffix);
            }
	}
    }
    return weechat::PLUGIN_RC_OK;
}

sub highlight
{
    my $command = weechat::get_plugin_config("cmd_highlight");
    system($command.$command_suffix);
    return weechat::PLUGIN_RC_OK;
}

sub pv
{
    my $command = weechat::get_plugin_config("cmd_pv");
    system($command.$command_suffix);
    return weechat::PLUGIN_RC_OK;
}

sub sound_cmd
{
    if ($#_ == 1)
    {
        my $filename = $_[1].".wav";
        my $command = weechat::get_plugin_config("cmd_ctcp");
	$command =~ s/(\$\w+)/$1/gee;
	system($command.$command_suffix);
        weechat::command("/quote PRIVMSG ".weechat::get_info("channel")." :\001SOUND $filename\001") if (@_);
    }
    return weechat::PLUGIN_RC_OK;
}
