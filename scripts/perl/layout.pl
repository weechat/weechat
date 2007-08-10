#
# Copyright (c) 2007 by FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Save/restore buffers layout.
#
# History:
#
# 2007-08-10, FlashCode <flashcode@flashtux.org>:
#     version 0.1, initial release
#

use strict;

# default values in setup file (~/.weechat/plugins.rc)
my $default_auto_save = "on";

weechat::register("layout", "0.1", "unload_layout", "Save/restore windows/buffers layout");
weechat::set_plugin_config("auto_save", $default_auto_save) if (weechat::get_plugin_config("auto_save") eq "");

weechat::add_command_handler("layout", "layout",
                             "save/restore windows/buffers layout",
                             "[save]",
                             "save: save buffers order",
                             "save");
weechat::add_event_handler("buffer_open", "buffer_open");


# get buffer name, depending on type of buffer

sub get_buffer_name
{
    # DCC type ?
    return "<DCC>" if ($_[0] eq "1");
    
    # RAW IRC type ?
    return "<RAW>" if ($_[0] eq "2");
    
    # no channel ?
    if ($_[2] eq "")
    {
        return "<servers>" if (weechat::get_config("look_one_server_buffer") eq "on");
        return "<empty>" if ($_[1] eq "");
        return $_[1];
    }
    
    # return server/channel
    return $_[1]."/".$_[2];
}

# get buffer list and build a hash:  number => name

sub get_layout_hash
{
    my %buflist;
    my $bufinfo = weechat::get_buffer_info();
    if ($bufinfo)
    {
        while (my ($nobuf, $binfos) = each %$bufinfo)
        {
            $buflist{$nobuf}{"server"} = $$binfos{"server"};
            $buflist{$nobuf}{"channel"} = $$binfos{"channel"};
            $buflist{$nobuf}{"name"} = get_buffer_name($$binfos{"type"},
                                                       $$binfos{"server"},
                                                       $$binfos{"channel"});
        }
    }
    return %buflist;
}

# get buffet list and build a hash:  name => number

sub get_layout_hash_by_name
{
    my %buflist;
    my $bufinfo = weechat::get_buffer_info();
    if ($bufinfo)
    {
        while (my ($nobuf, $binfos) = each %$bufinfo)
        {
            my $name = get_buffer_name($$binfos{"type"},
                                       $$binfos{"server"},
                                       $$binfos{"channel"});
            $buflist{$name} = $nobuf;
        }
    }
    return %buflist;
}

# build a string with buffer list hash

sub get_layout_string
{
    my $layout = "";
    my %buflist = get_layout_hash();
    my @keys = sort { $a <=> $b } (keys %buflist);
    foreach my $key (@keys)
    {
        $layout .= ",".$key.":".$buflist{$key}{"name"};
    }
    $layout = substr($layout, 1);
    return $layout;
}

# build a hash (name => number) from config string

sub get_hash_from_string
{
    my %buflist;
    my @fields = split(/,/, $_[0]);
    foreach my $field (@fields)
    {
        $buflist{$2} = $1 if ($field =~ /([0-9]+):(.*)/);
    }
    return %buflist;
}

# save layout to plugin option

sub save_layout
{
    my $layout = get_layout_string();
    if ($layout ne "")
    {
        weechat::print_server("Layout: saving buffers order: $layout");
        weechat::set_plugin_config("buffers", $layout);
    }
}

# the /layout command

sub layout
{
    if ($_[1] eq "save")
    {
        save_layout();
    }
    else
    {
        weechat::command("/help layout");
    }
    return weechat::PLUGIN_RC_OK;
}

# event handler called when a buffer is open

sub buffer_open
{
    # retrieve current layout and config layout
    my %buflist = get_layout_hash();
    my %buflist_name = get_layout_hash_by_name();
    my %config = get_hash_from_string(weechat::get_plugin_config("buffers"));
    
    my $name = $buflist{$_[0]}{"name"};
    
    # scan open buffers and look for correct position
    my $pos = $config{$name};
    if ($pos ne "")
    {
        my @keys = sort { $b <=> $a } (keys %buflist);
        my $nbbuf = $#keys + 1;
        
        # find best position (move to last known buffer + 1)
        my ($best_pos, $best_pos2) = (-1, -1);
        my $config_seen = 0;
        foreach my $key (@keys)
        {
            $config_seen = 1 if (($name ne $buflist{$key}{"name"})
                                 && ($config{$buflist{$key}{"name"}} ne ""));
            $best_pos = scalar($key) if ($config_seen == 0);
        }
        foreach my $key (@keys)
        {
            $best_pos2 = scalar($key) if (($best_pos2 == -1)
                                          && ($name ne $buflist{$key}{"name"})
                                          && ($config{$buflist{$key}{"name"}} ne "")
                                          && (scalar($config{$name}) < scalar($config{$buflist{$key}{"name"}})));
        }
        $best_pos = $best_pos2 if (($best_pos2 != -1) && ($best_pos2 < $best_pos));
        
        if (($best_pos != -1) && ($best_pos != scalar($buflist_name{$name})))
        {
            weechat::command("/buffer move $best_pos",
                             $buflist{$_[0]}{"channel"},
                             $buflist{$_[0]}{"server"});
        }
    }
    
    return weechat::PLUGIN_RC_OK;
}

# function called when unloading script (save layout if auto_save is on)

sub unload_layout
{
    my $auto_save = weechat::get_plugin_config("auto_save");
    save_layout() if (lc($auto_save) eq "on");
}
