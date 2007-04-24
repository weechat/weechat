# Copyright (c) 2007 by Javier Valencia <jvalencia@log01.org>
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

def weechat_init
	Weechat.register("browse", "0.1", "", "Fast url browsing for Weechat.")
	
	Weechat.add_command_handler(
		"browse", 
		"do_browse", 
		"Use your preferred browser to navigate an url.", 
		"[number of url] (-1 to browse all) (leave empty to browse first one)")
	Weechat.add_command_handler(
		"webbrowser", 
		"set_browser", 
		"Get/Set browser command", 
		"[command] (leavy empty to get current command)")
	Weechat.add_command_handler(
		"urls", 
		"get_url_list", 
		"Get a list of urls in a channel.")
		
	Weechat.print("")
	Weechat.print("See help for 'browse', 'webbrowser' and 'urls' commands.")
	if Weechat.get_plugin_config("webbrowser").empty?
		Weechat.print("")
		Weechat.print("WARNING: you must define a web browser with the command /webbrowser.")
	end
	return Weechat::PLUGIN_RC_OK
end

def set_browser(server, args)
	if args.empty?
		Weechat.print("current web browser -> #{Weechat.get_plugin_config('webbrowser')}")
	else
		Weechat.set_plugin_config("webbrowser", args)
		Weechat.print("web browser set to -> #{args}")
	end
	return Weechat::PLUGIN_RC_OK
end

def get_url_array(buffer_data)
	urls = Array.new
	buffer_data.each do |line|
		line["data"].split.each do |word|
			if word =~ /^http\:\/\/.*/
				urls << word
			end
		end
	end
	return urls.uniq.reverse
end

def get_url_list(server,args)
	channel = Weechat.get_info("channel")
	if channel.empty?
		Weechat.print("You must be inside a channel.")
		return Weechat::PLUGIN_RC_KO
	end
	buffer = Weechat.get_buffer_data(server, channel)
	unless buffer
		Weechat.print("Error retrieving window buffer, may be a bug?.")
		return Weechat::PLUGIN_RC_KO
	end
	Weechat.print("")
	Weechat.print("URL listing:")
	pos = 0
	get_url_array(buffer).each do |url|
		Weechat.print("#{pos}) #{url}")
		pos += 1
	end
	return Weechat::PLUGIN_RC_OK
end

def do_browse(server, args)
	num = args.to_i
	channel = Weechat.get_info("channel")
	if channel.empty?
		Weechat.print("You must be inside a channel.")
		return Weechat::PLUGIN_RC_KO
	end
	buffer = Weechat.get_buffer_data(server, channel)
	unless buffer
		Weechat.print("Error retrieving window buffer, may be a bug?.")
		return Weechat::PLUGIN_RC_KO
	end
	browser = Weechat.get_plugin_config("webbrowser")
	if browser.empty?
		Weechat.print("")
		Weechat.print("WARNING: you must define a web browser with the command /webbrowser.")
		return Weechat::PLUGIN_RC_KO
	end
	urls = get_url_array(buffer)
	if urls.empty?
		Weechat.print("No urls in current channel.")
	else
		if num == -1
			Weechat.print("Starting browser...")
			fork do	# only for unix/posix
				exec("#{browser} #{urls.join(' ')} &> /dev/null")
			end
		else
			if urls[num]
				Weechat.print("Starting browser...")
				fork do	# only for unix/posix
					exec("#{browser} #{urls[num]} &> /dev/null")
				end
			else
				Weechat.print("Invalid url number.")
			end
		end
	end
	return Weechat::PLUGIN_RC_OK
end
