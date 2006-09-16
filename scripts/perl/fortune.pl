# This script is a port from the original fortune.pl irssi script written by
# Ivo Marino <eim@cpan.org>. This script is in the public domain
# 
# Author: Julien Louis <ptitlouis@sysif.net>

weechat::register("fortune", "0.1", "", "Send a random fortune cookie to a specified nick");

weechat::add_command_handler ("fortune", fortune, "Send a random fortune cookie to a specified nick",
	"<nick> [lang]", 
	"<nick> The nickname to send the fortune cookie\n" .
	" [lang] The cookie language (Default: en)\n",
	"%n %-");

sub fortune {

    my ($server, $param) = @_;
    my $return = weechat::PLUGIN_RC_OK;
    my $cookie = '';

    if ($param) {

        if ($server) {
            (my $nick, my $lang) = split (' ', $param);
            $lang = 'en' unless ($lang eq 'de'|| $lang eq 'it' || $lang eq
'en' || $lang eq 'fr' );
            weechat::print ("Nick: " . $nick . ", Lang: \"" . $lang . "\"");

            if ($lang eq 'de') {
                $cookie = `fortune -x`;
            } elsif ($lang eq 'it') {
                $cookie = `fortune -a italia`;
            } else {
                $cookie = `fortune -a fortunes literature riddles`;
            }

            $cookie =~ s/\s*\n\s*/ /g;
            if ($cookie) {
		$channel = weechat::get_info("channel");
                if ($channel) {
                        weechat::command($nick . ": " . $cookie, $channel);
                }
            } else {
                weechat::print ("No cookie.");
                $return = weechat::PLUGIN_RC_KO;
            }
        } else {
            weechat::print ("Not connected to server");
            $return = weechat::PLUGIN_RC_KO;
        }
    } else {
        weechat::print ("Usage: /fortune <nick> [language]");
        $return = weechat::PLUGIN_RC_KO;
    }
    return $return;
}


