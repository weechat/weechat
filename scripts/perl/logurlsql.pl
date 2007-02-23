# perl
# Logs URLs to a p5-DBI-MySQL database.
# This script is BSD licensed.
# Hacked together by Karlis "Hagabard" Loen in Feb 2007
# hagabard AT gmail DOT com
# Table format: (ircurls is default table name in script)
#+-----------+---------------+------+-----+---------+-------+
#| Field     | Type          | Null | Key | Default | Extra |
#+-----------+---------------+------+-----+---------+-------+
#| timestamp | timestamp(14) | YES  |     | NULL    |       |
#| hostmask  | text(255)     | YES  |     | NULL    |       |
#| target    | text(255)     | YES  |     | NULL    |       |
#| url       | text(255)     | YES  |     | NULL    |       |
#+-----------+---------------+------+-----+---------+-------+
# In the future I hope to get around to adding stuff like:
#+-----------+---------------+------+-----+---------+-------+
#| httptitle | text(255)     | YES  |     | NULL    |       |
#| httpdinfo | text(255)     | YES  |     | NULL    |       |
#| tinyurl   | text(255)     | YES  |     | NULL    |       |
#+-----------+---------------+------+-----+---------+-------+
# Test other DBI types, maybe /setp perl.logurlsql.dbtype="Pg" works?

# Tested with p5-DBI-Mysql, others untested
use DBI;

weechat::register("logurlsql", "0.2", "", "WeeChat p5-DBI script to log URLs to a SQL Database.");

weechat::add_message_handler("*", "cmd_logurl");

# Setup Database connection, now with /setp for your configuration pleasure
weechat::set_plugin_config("dbtype", "mysql") if (weechat::get_plugin_config("dbtype") eq "");
weechat::set_plugin_config("dbhost", "localhost") if (weechat::get_plugin_config("dbhost") eq "");
weechat::set_plugin_config("dbname", "weechat") if (weechat::get_plugin_config("dbname") eq "");
weechat::set_plugin_config("dbtable", "ircurls") if (weechat::get_plugin_config("dbtable") eq "");
weechat::set_plugin_config("dbuser", "irc") if (weechat::get_plugin_config("dbuser") eq "");
weechat::set_plugin_config("dbpass", "forever") if (weechat::get_plugin_config("dbpass") eq "");

# Regexps
sub URL_SCHEME_REGEX()                  { '(http|ftp|https|news|irc)' }
sub URL_GUESS_REGEX()                   { '(www|ftp)' }
sub URL_BASE_REGEX()                    { '[a-z0-9_\-+\\/:?%.&!~;,=\#<]' }

# This sub borrowed from the BSD licensed urlplot.pl irssi script by bwolf.
sub scan_url {
        my $rawtext = shift;
        return $1 if $rawtext =~ m|(@{[ URL_SCHEME_REGEX ]}://@{[ URL_BASE_REGEX ]}+)|io;
        # The URL misses a scheme, try to be smart
        if ($rawtext =~ m|@{[ URL_GUESS_REGEX ]}\.@{[ URL_BASE_REGEX ]}+|io) {
                my $preserve = $&;
                return "http://$preserve" if $1 =~ /^www/;
                return "ftp://$preserve"  if $1 =~ /^ftp/;
        }
        return undef;
}

sub cmd_logurl {
        ($null, $target, $data) = split ":",$_[1],3;
        ($hostmask, $null, $target) = split " ", $target;
        if (defined($url = scan_url($data))) {
                #weechat::print("hostmask=$hostmask, target=$target, url=$url");
                db_insert($hostmask, $target, $url);
        }
        return weechat::PLUGIN_RC_OK;
}

sub db_insert {
        my ($hostmask, $target, $url)=@_;
        my $dbh = DBI->connect( "DBI:". weechat::get_plugin_config(dbtype) .":".  weechat::get_plugin_config(dbname) .":". weechat::get_plugin_config(dbhost), weechat::get_plugin_config(dbuser), weechat::get_plugin_config(dbpass));
        my $sql="insert into ". weechat::get_plugin_config(dbtable) ." (timestamp, hostmask, target, url) values (NOW()".",". $dbh->quote($hostmask) ."," . $dbh->quote($target) ."," . $dbh->quote($url) .")";
        my $sth = $dbh->do($sql);
        $dbh->disconnect();
}
