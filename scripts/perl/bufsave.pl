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
# Save current buffer to a file.
#
# History:
#
# 2006-10-27, FlashCode <flashcode@flashtux.org>:
#     initial release
#

use strict;

my $version = "0.1";

weechat::register("bufsave", $version, "", "Save buffer content to a file");
weechat::add_command_handler("bufsave", "bufsave_cmd",
                             "save current buffer to a file",
                             "filename",
                             "filename: target file (must not exist)",
                             "");

sub bufsave_cmd
{
    if ($#_ == 1)
    {
        my $server = shift;
        my $filename = shift;
        
        return weechat::PLUGIN_RC_OK if (! $filename);
        
        if (-e $filename)
        { 
            weechat::print("Error: target file already exists!");
            return weechat::PLUGIN_RC_KO;
        }
        
        my $channel = weechat::get_info("channel");
        my @bc = weechat::get_buffer_data($server, $channel);
        if (@bc)
        {
            if (! open(FILE, ">$filename"))
            {
                weechat::print("Error writing to target file!");
                return weechat::PLUGIN_RC_KO;
            }
            foreach my $line (reverse(@bc))
            {
                my %l = %$line;
                print FILE "$l{date}  " if ($l{date});
                print FILE "\<$l{nick}\> " if ($l{nick});
                print FILE "$l{data}\n";
            }
            close(FILE);
        }
        else
        {
            weechat::print("Error: no buffer data");
            return weechat::PLUGIN_RC_KO;
        }
    }
    return weechat::PLUGIN_RC_OK;
}
