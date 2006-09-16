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
# Search for text in WeeChat disk log files.
#
# History:
# 2006-04-17, FlashCode <flashcode@flashtux.org>:
#     initial release
#

use strict;

# default values in setup file (~/.weechat/plugins.rc)
my $default_max          = "8";
my $default_server       = "off";
my $default_grep_options = "-i";

# init script
weechat::register("logsearch", "0.1", "", "Search for text in WeeChat disk log files");
weechat::set_plugin_config("max", $default_max) if (weechat::get_plugin_config("max") eq "");
weechat::set_plugin_config("server", $default_server) if (weechat::get_plugin_config("server") eq "");
weechat::set_plugin_config("grep_options", $default_grep_options) if (weechat::get_plugin_config("grep_options") eq "");

# add command handler /logsearch
weechat::add_command_handler("logsearch", "logsearch",
                             "search for text in WeeChat disk log files",
                             "[-n#] text",
                             "-n#: max number or lines to display\n"
                             ."text: regular expression (used by grep)\n\n"
                             ."Plugins options (set with /setp):\n"
                             ."  - perl.logsearch.max: max number of lines displayed by default\n"
                             ."  - perl.logsearch.server: display result on server "
                             ."buffer (if on), otherwise on current buffer\n"
                             ."  - perl.logsearch.grep_options: options to give to grep program",
                             "");

# /logsearch command
sub logsearch
{
    my $server = shift;
    my $args = shift;
    if ($args ne "")
    {
        # read settings
        my $conf_max = weechat::get_plugin_config("max");
        $conf_max = $default_max if ($conf_max eq "");
        my $conf_server = weechat::get_plugin_config("server");
        $conf_server = $default_server if ($conf_server eq "");
        my $output_server = "";
        $output_server = $server if (lc($conf_server) eq "on");
        my $grep_options = weechat::get_plugin_config("grep_options");
        
        # build log filename
        my $buffer = weechat::get_info("channel", "");
        $buffer = ".".$buffer if ($buffer ne "");
        my $log_path = weechat::get_config("log_path");
        $log_path =~ s/%h/~\/.weechat/g;
        my $file = $log_path.$server.$buffer.".weechatlog";
        
        # run grep in log file
        if ($args =~ /-n([0-9]+) (.*)/)
        {
            $conf_max = $1;
            $args = $2;
        }
        my $command = "grep ".$grep_options." '".$args."' ".$file." 2>/dev/null | tail -n".$conf_max;
        my $result = `$command`;
        
        # display result
        if ($result eq "")
        {
            weechat::print("Text not found in $file", "", $output_server);
            return weechat::PLUGIN_RC_OK;
        }
        my @result_array = split(/\n/, $result);
        weechat::print($_, "", $output_server) foreach(@result_array);
    }
    return weechat::PLUGIN_RC_OK;
}
