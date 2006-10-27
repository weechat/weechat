#
# Copyright (c) 2006 by DeltaS4 <deltas4@gmail.com>
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
# Changelog:
#  0.1: first version
#  0.2: now you don't need the Song Change plugin enabled because
#  	it uses audtool --current-song to get the music name as nenolod
#  	suggested (http://boards.nenolod.net/index.php?showtopic=147). :)
#

#
# /setp perl.audacious.msg = "<msg that u want before the song>"
# example: "is playing"		* YourNick is playing Artist - Title
# 	   "now playing:" 	* YourNick now playing: Artist - Title
# 	   "at" 		* YourNick at Artist - Title
#

weechat::register ("audacious", "0.2", "", "audacious-weechat current song script (usage: /music)");
weechat::add_command_handler ("music", audtool);

$msg = weechat::get_plugin_config("msg");
if ($msg eq $null) { weechat::set_plugin_config("msg", "is playing"); }

sub audtool {
	$cs = `audtool --current-song`; chomp($cs);
	$msg = weechat::get_plugin_config("msg");
	
	weechat::command("/me $msg $cs"); 
}
