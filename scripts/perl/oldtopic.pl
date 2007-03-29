#
# Copyright (c) 2007 by FlashCode <flashcode@flashtux.org>
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
# Display old topics for a channel.
#
# History:
# 2007-03-29, FlashCode <flashcode@flashtux.org>:
#     initial release
#

use strict;

my $version = "0.1";

my $plugin_help_options = "Plugins options (set with /setp):\n"
    ."  - perl.oldtopic.log_server: if on, displays all topic changes on server buffer\n"
    ."  - perl.oldtopic.max_topics: number of topics to keep for each channel "
    ."(0 = do not keep any topics)";

# default values in setup file (~/.weechat/plugins.rc)
my $default_log_server = "off";
my $default_max_topics = "10";

# init script
weechat::register("oldtopic", $version, "", "Display old topics for a channel");
weechat::set_plugin_config("log_server", $default_log_server) if (weechat::get_plugin_config("log_server") eq "");
weechat::set_plugin_config("max_topics", $default_max_topics) if (weechat::get_plugin_config("max_topics") eq "");

# message and command handlers
weechat::add_message_handler("topic", "topic_msg");
weechat::add_message_handler("332", "topic_332_msg");
weechat::add_message_handler("333", "topic_333_msg");
weechat::add_command_handler ("lasttopic", "lasttopic_cmd",
                              "Display last topic for a channel (topic before current one)",
                              "[channel]",
                              "channel: channel name (default: current channel)\n\n"
                              .$plugin_help_options,
                              "%C");
weechat::add_command_handler ("oldtopic", "oldtopic_cmd",
                              "Display old topics for a channel",
                              "[channel]",
                              "channel: channel name (default: current channel)\n\n"
                              .$plugin_help_options,
                              "%C");

my %oldtopic;
undef %oldtopic;

my %msg_332_topic;
undef %msg_332_topic;

sub get_config
{
    my $conf_log_server = weechat::get_plugin_config("log_server");
    $conf_log_server = $default_log_server if ($conf_log_server eq "");
    
    my $conf_max = weechat::get_plugin_config("max_topics");
    $conf_max = $default_max_topics if ($conf_max eq "");
    $conf_max = 0 if ($conf_max < 0);
    
    return ($conf_log_server, $conf_max);
}

sub add_topic
{
    my ($server, $channel, $nick, $topic, $topicdate, $conf_max) =
        ($_[0], $_[1], $_[2], $_[3], $_[4], $_[5]);
    
    my $time = $topicdate;
    $time = time() if ($time eq "");
    
    $oldtopic{$server}{$channel}{$time}{"nick"} = $nick;
    $oldtopic{$server}{$channel}{$time}{"topic"} = $topic;
    my $numchan = scalar(keys(%{$oldtopic{$server}{$channel}}));
    if ($numchan > $conf_max)
    {
        my $diff = $numchan - $conf_max;
        my $count = 0;
        foreach my $date (sort keys %{$oldtopic{$server}{$channel}})
        {
            delete($oldtopic{$server}{$channel}{$date});
            $count++;
            last if ($count >= $diff);
        }
    }
}

# received when someone changes topic on a channel
sub topic_msg
{
    my $server = $_[0];
    my ($conf_log_server, $conf_max) = get_config();
    
    if ($_[1] =~ /(.*) TOPIC (.*)/)
    {
        my $nick = $1;
        my $args = $2;
        my $nick = $1 if ($nick =~ /:?([^!]+)(!?.*)/);
        if ($args =~ /([\#\&\+\!][^ ]*) (.*)/)
        {
            my $channel = $1;
            my $topic = $2;
            $topic = $1 if ($topic =~ /:(.*)/);
            
            if ($conf_log_server eq "on")
            {
                weechat::print("Topic on $channel changed by ${nick} to: \"${topic}\x0F\"",
                               "", $server);
            }
            
            if ($conf_max > 0)
            {
                add_topic($server, $channel, $nick, $topic, "", $conf_max);
            }
            else
            {
                delete($oldtopic{$server}{$channel});
            }
        }
    }
    return weechat::PLUGIN_RC_OK;
}

# topic for a channel (when joining channel)
sub topic_332_msg
{
    my $server = $_[0];
    
    if ($_[1] =~ /(.*) 332 (.*)/)
    {
        my $args = $2;
        if ($args =~ /([\#\&\+\!][^ ]*) (.*)/)
        {
            my $channel = $1;
            my $topic = $2;
            $topic = $1 if ($topic =~ /:(.*)/);
            $msg_332_topic{$server}{$channel} = $topic;
        }
    }
    return weechat::PLUGIN_RC_OK;
}

# nick/date for topic (when joining channel, received after 332)
sub topic_333_msg
{
    my $server = $_[0];
    
    if ($_[1] =~ /.* 333 .* ([\#\&\+\!][^ ]*) (.*) ([0-9]+)/)
    {
        my $channel = $1;
        my $nick = $2;
        my $date = $3;
        my ($conf_log_server, $conf_max) = get_config();
        add_topic($server, $channel, $nick,
                  $msg_332_topic{$server}{$channel},
                  $date, $conf_max);
    }
    return weechat::PLUGIN_RC_OK;
}

sub lasttopic_cmd
{
    my $server = weechat::get_info("server");
    my $channel = weechat::get_info("channel");
    $channel = $_[1] if ($_[1] ne "");
    
    if ($channel ne "")
    {
        my ($conf_log_server, $conf_max) = get_config();
        
        delete($oldtopic{$server}{$channel}) if ($conf_max == 0);
        
        my $count = 0;
        my $found = 0;
        foreach my $date (sort { $b <=> $a } keys %{$oldtopic{$server}{$channel}})
        {
            $count++;
            if ($count > 1)
            {
                $found = 1;
                weechat::print("Last topic for ${server}/${channel}: on ".localtime($date)." by ".
                               $oldtopic{$server}{$channel}{$date}{"nick"}.": \"".
                               $oldtopic{$server}{$channel}{$date}{"topic"}."\x0F\"");
                last;
            }
        }
        weechat::print("Last topic not found for channel ${server}/${channel}.") if ($found == 0);
    }
    else
    {
        weechat::print("Error: this buffer is not a channel.");
    }
    
    return weechat::PLUGIN_RC_OK;
}

sub oldtopic_cmd
{
    my $server = weechat::get_info("server");
    my $channel = weechat::get_info("channel");
    $channel = $_[1] if ($_[1] ne "");
    
    if ($channel ne "")
    {
        my ($conf_log_server, $conf_max) = get_config();
        
        delete($oldtopic{$server}{$channel}) if ($conf_max == 0);
        
        my $found = 0;
        foreach my $date (sort keys %{$oldtopic{$server}{$channel}})
        {
            $found = 1;
            weechat::print("Topic for ${server}/${channel} on ".localtime($date)." by ".
                           $oldtopic{$server}{$channel}{$date}{"nick"}.": \"".
                           $oldtopic{$server}{$channel}{$date}{"topic"}."\x0F\"");
        }
        weechat::print("No old topic for channel ${server}/${channel}.") if ($found == 0);
    }
    else
    {
        weechat::print("Error: this buffer is not a channel.");
    }
    
    return weechat::PLUGIN_RC_OK;
}
