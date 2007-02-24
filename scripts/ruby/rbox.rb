# Rbox plugin (Rhythmnox control and now playing plugin.)
# Version 0.2
# Released under GNU GPL v2
# Metallines <metallines@gmail.com>
# /rbox-help for help

def weechat_init
       	Weechat.register("rbox", "0.2", "", "Rhythmbox control and now playing plugin.")
	Weechat.add_command_handler("rbox-print", "rbox_print")
	Weechat.add_command_handler("rbox-play", "rbox_play")
	Weechat.add_command_handler("rbox-pause", "rbox_pause")
	Weechat.add_command_handler("rbox-previous", "rbox_previous")
	Weechat.add_command_handler("rbox-next", "rbox_next")
	Weechat.add_command_handler("rbox-help", "rbox_help")
	return Weechat::PLUGIN_RC_OK
end

def rbox_print(server, args)
	if `ps -C rhythmbox` =~ /rhythmbox/
		artiste = `rhythmbox-client --print-playing-format %ta`.chomp
		titre = `rhythmbox-client --print-playing-format %tt`.chomp
		album = `rhythmbox-client --print-playing-format %at`.chomp
		
		couleur_artiste = "C05"
		couleur_titre = "C10"
		couleur_album = "C03"
		couleur_entre = "C14"
		
		Weechat.command("/me écoute" + " %" + couleur_titre + titre + " %" + couleur_entre + "par" + " %" + couleur_artiste + artiste + " %" + couleur_entre + "de l'album" + " %" + couleur_album + album)
	else 
		Weechat.print("Rhythmbox isn't running.")
	end
	return Weechat::PLUGIN_RC_OK
end

def rbox_play(server, args)
	if `ps -C rhythmbox` =~ /rhythmbox/
		`rhythmbox-client --play`
		Weechat.print("Start playback.")
	else
		Weechat.print("Rhythmbox isn't running.")
	end
	return Weechat::PLUGIN_RC_OK
end

def rbox_pause(server, args)
	if `ps -C rhythmbox` =~ /rhythmbox/
		`rhythmbox-client --pause`
		Weechat.print("Pause playback.")
	else
		Weechat.print("Rhythmbox isn't running.")
	end
	return Weechat::PLUGIN_RC_OK
end

def rbox_previous(server, args)
	if `ps -C rhythmbox` =~ /rhythmbox/
		`rhythmbox-client --previous`
		Weechat.print("Skip to the previous track.")
	else
		Weechat.print("Rhythmbox isn't running.")
	end
	return Weechat::PLUGIN_RC_OK
end

def rbox_next(server, args)
	if `ps -C rhythmbox` =~ /rhythmbox/
		`rhythmbox-client --next`
		Weechat.print("Skip to the next track.")
	else
		Weechat.print("Rhythmbox isn't running.")
	end
	return Weechat::PLUGIN_RC_OK
end

def rbox_help(server, args)
	Weechat.print("/rbox-play -> Start playback")
        Weechat.print("/rbox-pause -> Pause playback")
	Weechat.print("/rbox-previous -> Skip to previous track")
	Weechat.print("/rbox-next -> Skip to next track")
	Weechat.print("/rbox-print -> Print which song rhythmbox is playing in current channel")
	Weechat.print("/robix-help -> Display this help")
	return Weechat::PLUGIN_RC_OK
end
