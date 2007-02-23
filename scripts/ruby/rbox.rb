# Publié sous license GNU GPL v2
# Metallines <metallines@gmail.com>

def weechat_init
       	Weechat.register("rbox", "0.1", "", "Affiche ce que rhythmbox joue.")
	Weechat.add_command_handler("rbox", "rbox")
	return Weechat::PLUGIN_RC_OK
end

def rbox(server, args)
	# Si un proocessus qui contient "rhythmbox" est en cours
	if `ps -C rhythmbox` =~ /rhythmbox/
		# On récupère les informations du morceau joué
		artiste = `rhythmbox-client --print-playing-format %ta`.chomp
		titre = `rhythmbox-client --print-playing-format %tt`.chomp
		album = `rhythmbox-client --print-playing-format %at`.chomp
		# On définit les couleurs pour l'affichage dans le salon
		couleur_artiste = "C05"
		couleur_titre = "C10"
		couleur_album = "C03"
		couleur_entre = "C14"
		# On execute la commande dans le salon
		Weechat.command("/me écoute" + " %" + couleur_titre + titre + " %" + couleur_entre + "par" + " %" + couleur_artiste + artiste + " %" + couleur_entre + "de l'album" + " %" + couleur_album + album)
	# Sinon on prévient l'utilisateur que Rhythbox ne tourne pas
	else 
		Weechat.print("Rhythmbox n'est pas lancé.")
	end
	return Weechat::PLUGIN_RC_OK
end
