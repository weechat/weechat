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
# Translate words and display result in infobar, local buffer or channel
#
# History:
# 2006-07-03, FlashCode <flashcode@flashtux.org>:
#     initial release
#

use strict;

# default values in setup file (~/.weechat/plugins.rc)
my $default_output_infobar = "on";
my $default_timeout = "2";

# script internal settings
my $languages = "de|el|en|es|fr|it|ja|ko|nl|pt|ru|zh|zt";

# init script
weechat::register("translate", "0.1", "", "Translation script");
weechat::set_plugin_config("output_infobar", $default_output_infobar) if (weechat::get_plugin_config("output_infobar") eq "");
weechat::set_plugin_config("timeout", $default_timeout) if (weechat::get_plugin_config("timeout") eq "");

# add command handlers for all languages
weechat::add_command_handler("translate", "translate", "translate text to other language",
                             "lang1 lang2 text [-o]",
                             "lang1: base language\n"
                             ."lang2: target language\n"
                             ." text: text to translate\n"
                             ."   -o: result is written on channel (visible by all)",
                             $languages." ".$languages);

# translate text with babelfish (altavista)
sub babelfish
{
    my $timeout = weechat::get_plugin_config("timeout");
    $timeout = $default_timeout if ($timeout eq "");
    
    if (`wget -q --timeout=$timeout --user-agent "Mozilla" --output-document=- "http://babelfish.altavista.com/babelfish/tr?lp=$_[0]_$_[1]&urltext=$_[2]"` =~ /.*<td bgcolor=white class=s><div style=padding:10px;>(.*)<\/div><\/td>/)
    {
        return $1;
    }
    return "";
}

# translate output
sub translate_output
{
    if ($_[1] == 1)
    {
        if (substr($_[0],0,1) eq "/")
        {
            weechat::command("/".$_[0], "", "");
        }
        else
        {
            weechat::command($_[0], "", "");
        }
    }
    else
    {
        my $output_infobar = weechat::get_plugin_config("output_infobar");
        $output_infobar = $default_output_infobar if ($output_infobar eq "");
        if ($output_infobar eq "on")
        {
            weechat::print_infobar(5, $_[0]);
        }
        else
        {
            weechat::print($_[0], "", "");
        }
    }
}

# /translate command
sub translate
{
    my $server = shift;
    my $args = shift;
    
    if ($args =~ /([a-zA-Z][a-zA-Z]) ([a-zA-Z][a-zA-Z]) (.*)/)
    {
        my $lang1 = $1;
        my $lang2 = $2;
        my $text = $3;
        
        # output on channel?
        my $output_chan = 0;
        if ($text =~ /(.*) -[oO]$/)
        {
            $output_chan = 1;
            $text = $1;
        }
        
        my $result = babelfish($lang1, $lang2, $text);
        if ($result eq "")
        {
            translate_output("Error: unable to translate (bad language or timeout)", 0);
        }
        else
        {
            translate_output($result, $output_chan);
        }
    }
    else
    {
        translate_output("Error: bad arguments", 0);
    }
    
    return weechat::PLUGIN_RC_OK;
}
