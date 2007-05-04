#   Forcenick v1.0 - forces your original nickname
#   Copyright (C) 2007  Pavel Shevchuk <stlwrt@gmail.com>

#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

def weechat_init
	Weechat.register("forcenick", "1.0", "", "Forces your original nickname, wipes out ghosts and nick stealers")
	Weechat.add_command_handler("forcenick", "forcenick", "Forces server_nick1, identifies to nickserv using server_password. Useful to call from server_command if your connection is unstable")
end

def forcenick(server, args)
	server_info = Weechat.get_server_info[server]
	cur_nick = server_info['nick']
	forced_nick = server_info['nick1']
	password = server_info['password']
	if cur_nick != forced_nick
		Weechat.command("/msg nickserv ghost #{forced_nick} #{password}", "", server)
		Weechat.command("/nick #{forced_nick}", "", server)
		Weechat.command("/msg nickserv identify #{password}", "", server)
	end
	return Weechat::PLUGIN_RC_OK
end

