#
# Copyright (c) 2006 by FlashCode <flashcode@flashtux.org>
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

#
# Speaker beep on highlight/private msg.
#
# History:
#
# 2006-09-02, FlashCode <flashcode@flashtux.org>:
#     initial release
#

use strict;

my $version = "0.1";
my $beep_command = "echo -n \a";

# default values in setup file (~/.weechat/plugins.rc)
my $default_beep_highlight = "on";
my $default_beep_pv        = "on";

weechat::register("Beep", $version, "", "Speaker beep on highlight/private msg");
weechat::set_plugin_config("beep_highlight", $default_beep_highlight) if (weechat::get_plugin_config("beep_highlight") eq "");
weechat::set_plugin_config("beep_pv", $default_beep_pv) if (weechat::get_plugin_config("beep_pv") eq "");

weechat::add_message_handler("weechat_highlight", "highlight");
weechat::add_message_handler("weechat_pv", "pv");

sub highlight
{
    my $beep = weechat::get_plugin_config("beep_highlight");
    system($beep_command) if ($beep eq "on");
    return weechat::PLUGIN_RC_OK;
}

sub pv
{
    my $beep = weechat::get_plugin_config("beep_pv");
    system($beep_command) if ($beep eq "on");
    return weechat::PLUGIN_RC_OK;
}
