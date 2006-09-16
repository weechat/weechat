# This script is a port from the hello.pl irssi script written by
# Cybertinus <cybertinus@cybertinus.nl>
# 
# Licensed under the GPL v2
#
# Author: Julien Louis <ptitlouis@sysif.net>

weechat::register("hello" ,"0.1", "", "Display greetings depending the time");

weechat::add_command_handler("hello", hello, "", "Send greetings to the current buffer");

weechat::set_plugin_config("evening_message", "good evening");
weechat::set_plugin_config("afternoon_message", "good afternoon");
weechat::set_plugin_config("morning_message", "good morning");
weechat::set_plugin_config("night_message", "good night");


sub hello {
	my ($server,$data) = @_;

	$time = (localtime(time))[2];
	if ($time >= 18) {
		$text = weechat::get_plugin_config("evening_message");
	} elsif ($time >= 12) {
		$text = weechat::get_plugin_config("afternoon_message");
	} elsif ($time >= 6) {
		$text = weechat::get_plugin_config("morning_message");
	} elsif ($time >= 0) {
		$text = weechat::get_plugin_config("night_message");
	}
	weechat::command("$text $data");
	return weechat::PLUGIN_RC_OK;
}

