#
# Copyright (C) 2008-2011 Sebastien Helleu <flashcode@flashtux.org>
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Documentation generator for WeeChat: build include files with commands,
# options, infos and completions for WeeChat core and plugins.
#
# Instructions to build config files yourself in WeeChat directories (replace
# all paths with your path to WeeChat):
#     1.  run WeeChat and load this script, with following command:
#           /perl load ~/src/weechat/doc/docgen.pl
#     2.  change path to build in your doc/ directory:
#           /set plugins.var.perl.docgen.path "~/src/weechat/doc"
#     3.  run docgen command:
#           /docgen
# Files should be in ~/src/weechat/doc/xx/autogen/ (where xx is language)
#

use strict;

use POSIX;            # needed for setlocale()
use Locale::gettext;
use File::Basename;

my $version = "0.1";

# -------------------------------[ config ]------------------------------------

# default path where doc files will be written (should be doc/ in sources
# package tree)
# path must have subdirectories with languages and autogen directory:
#      path
#       |-- en
#       |   |-- autogen
#       |-- fr
#       |   |-- autogen
#       ...
my $default_path = "~/src/weechat/doc";

# list of locales for which we want to build doc files to include
my @all_locale_list = qw(en_US fr_FR it_IT de_DE);

# all commands/options/.. of following plugins will produce a file
# non-listed plugins will be ignored
# value: "c" = plugin may have many commands
#        "o" = write config options for plugin
# if plugin is listed without "c", that means plugin has only one command
# /name (where "name" # is name of plugin)
# Note: we consider core is a plugin called "weechat"
my %plugin_list = ("weechat"   => "co", "alias"     => "",
                   "aspell"    => "o",  "charset"   => "co",
                   "demo"      => "co", "fifo"      => "co",
                   "irc"       => "co", "logger"    => "co",
                   "relay"     => "co", "rmodifier" => "co",
                   "perl"      => "",   "python"    => "",
                   "ruby"      => "",   "lua"       => "",
                   "tcl"       => "",   "xfer"      => "co");

# options to ignore
my @ignore_options = ("aspell\\.dict\\..*",
                      "aspell\\.option\\..*",
                      "charset\\.decode\\..*",
                      "charset\\.encode\\..*",
                      "irc\\.msgbuffer\\..*",
                      "irc\\.ctcp\\..*",
                      "irc\\.ignore\\..*",
                      "irc\\.server\\..*",
                      "jabber\\.server\\..*",
                      "logger\\.level\\..*",
                      "logger\\.mask\\..*",
                      "relay\\.port\\..*",
                      "rmodifier\\.modifier\\..*",
                      "weechat\\.palette\\..*",
                      "weechat\\.proxy\\..*",
                      "weechat\\.bar\\..*",
                      "weechat\\.debug\\..*",
                      "weechat\\.notify\\..*");

# infos to ignore
my @ignore_infos_plugins = ();

# infos (hashtable) to ignore
my @ignore_infos_hashtable_plugins = ();

# infolists to ignore
my @ignore_infolists_plugins = ();

# hdata to ignore
my @ignore_hdata_plugins = ();

# completions to ignore
my @ignore_completions_plugins = ();
my @ignore_completions_items = ("docgen.*",
                                "jabber.*",
                                "weeget.*");

# for gettext
my $d;

# -------------------------------[ init ]--------------------------------------

weechat::register("docgen", "Sebastien Helleu <flashcode\@flashtux.org>", $version,
                  "GPL3", "Doc generator for WeeChat 0.3.x", "", "");
weechat::hook_command("docgen", "Doc generator",
                      "[locales]",
                      "locales: list of locales to build (by default build all locales)",
                      "%(docgen_locales)|%*", "docgen", "");
weechat::hook_completion("docgen_locales", "locales for docgen", "docgen_completion", "");
weechat::config_set_plugin("path", $default_path)
    if (weechat::config_get_plugin("path") eq "");

# -----------------------------------------------------------------------------

# gettext
sub weechat_gettext
{
    return $d->get($_[0]);
}

# get list of commands in a hash with 3 indexes: plugin, command, xxx
sub get_commands
{
    my %commands;
    
    my $infolist = weechat::infolist_get("hook", "", "command");
    while (weechat::infolist_next($infolist))
    {
        my $command = weechat::infolist_string($infolist, "command");
        my $plugin = weechat::infolist_string($infolist, "plugin_name");
        $plugin = "weechat" if ($plugin eq "");
        if (exists($plugin_list{$plugin}))
        {
            if (($command eq $plugin) || ($plugin_list{$plugin} =~ /c/))
            {
                $commands{$plugin}{$command}{"description"} = weechat::infolist_string($infolist, "description");
                $commands{$plugin}{$command}{"args"} = weechat::infolist_string($infolist, "args");
                $commands{$plugin}{$command}{"args_description"} = weechat::infolist_string($infolist, "args_description");
                $commands{$plugin}{$command}{"completion"} = weechat::infolist_string($infolist, "completion");
            }
        }
    }
    weechat::infolist_free($infolist);
    
    return %commands;
}

# get list of config options in a hash with 4 indexes: config, section, option, xxx
sub get_options
{
    my %options;
    
    my $infolist = weechat::infolist_get("option", "", "");
    while (weechat::infolist_next($infolist))
    {
        my $full_name = weechat::infolist_string($infolist, "full_name");
        
        # check if option is ignored or not
        my $ignore = 0;
        foreach my $mask (@ignore_options)
        {
            $ignore = 1 if ($full_name =~ /${mask}/);
        }
        
        if ($ignore ne 1)
        {
            my $config = weechat::infolist_string($infolist, "config_name");
            my $section = weechat::infolist_string($infolist, "section_name");
            my $option = weechat::infolist_string($infolist, "option_name");
            if (defined $plugin_list{$config} && ($plugin_list{$config} =~ /o/))
            {
                $options{$config}{$section}{$option}{"type"} = weechat::infolist_string($infolist, "type");
                $options{$config}{$section}{$option}{"string_values"} = weechat::infolist_string($infolist, "string_values");
                $options{$config}{$section}{$option}{"default_value"} = weechat::infolist_string($infolist, "default_value");
                $options{$config}{$section}{$option}{"min"} = weechat::infolist_integer($infolist, "min");
                $options{$config}{$section}{$option}{"max"} = weechat::infolist_integer($infolist, "max");
                $options{$config}{$section}{$option}{"null_value_allowed"} = weechat::infolist_integer($infolist, "null_value_allowed");
                $options{$config}{$section}{$option}{"description"} = weechat::infolist_string($infolist, "description");
            }
        }
    }
    weechat::infolist_free($infolist);
    
    return %options;
}

# get list of infos hooked by plugins in a hash with 3 indexes: plugin, name, xxx
sub get_infos
{
    my %infos;
    
    # get infos hooked
    my $infolist = weechat::infolist_get("hook", "", "info");
    while (weechat::infolist_next($infolist))
    {
        my $info_name = weechat::infolist_string($infolist, "info_name");
        my $plugin = weechat::infolist_string($infolist, "plugin_name");
        $plugin = "weechat" if ($plugin eq "");
        
        # check if info is ignored or not
        my $ignore = 0;
        foreach my $mask (@ignore_infos_plugins)
        {
            $ignore = 1 if ($plugin =~ /${mask}/);
        }
        
        if ($ignore ne 1)
        {
            $infos{$plugin}{$info_name}{"description"} = weechat::infolist_string($infolist, "description");
            $infos{$plugin}{$info_name}{"args_description"} = weechat::infolist_string($infolist, "args_description");
        }
    }
    weechat::infolist_free($infolist);
    
    return %infos;
}

# get list of infos (hashtable) hooked by plugins in a hash with 3 indexes: plugin, name, xxx
sub get_infos_hashtable
{
    my %infos_hashtable;
    
    # get infos hooked
    my $infolist = weechat::infolist_get("hook", "", "info_hashtable");
    while (weechat::infolist_next($infolist))
    {
        my $info_name = weechat::infolist_string($infolist, "info_name");
        my $plugin = weechat::infolist_string($infolist, "plugin_name");
        $plugin = "weechat" if ($plugin eq "");
        
        # check if info_hashtable is ignored or not
        my $ignore = 0;
        foreach my $mask (@ignore_infos_hashtable_plugins)
        {
            $ignore = 1 if ($plugin =~ /${mask}/);
        }
        
        if ($ignore ne 1)
        {
            $infos_hashtable{$plugin}{$info_name}{"description"} = weechat::infolist_string($infolist, "description");
            $infos_hashtable{$plugin}{$info_name}{"args_description"} = weechat::infolist_string($infolist, "args_description");
            $infos_hashtable{$plugin}{$info_name}{"output_description"} = weechat::infolist_string($infolist, "output_description");
        }
    }
    weechat::infolist_free($infolist);
    
    return %infos_hashtable;
}

# get list of infolists hooked by plugins in a hash with 3 indexes: plugin, name, xxx
sub get_infolists
{
    my %infolists;
    
    # get infolists hooked
    my $infolist = weechat::infolist_get("hook", "", "infolist");
    while (weechat::infolist_next($infolist))
    {
        my $infolist_name = weechat::infolist_string($infolist, "infolist_name");
        my $plugin = weechat::infolist_string($infolist, "plugin_name");
        $plugin = "weechat" if ($plugin eq "");
        
        # check if infolist is ignored or not
        my $ignore = 0;
        foreach my $mask (@ignore_infolists_plugins)
        {
            $ignore = 1 if ($plugin =~ /${mask}/);
        }
        
        if ($ignore ne 1)
        {
            $infolists{$plugin}{$infolist_name}{"description"} = weechat::infolist_string($infolist, "description");
            $infolists{$plugin}{$infolist_name}{"pointer_description"} = weechat::infolist_string($infolist, "pointer_description");
            $infolists{$plugin}{$infolist_name}{"args_description"} = weechat::infolist_string($infolist, "args_description");
        }
    }
    weechat::infolist_free($infolist);
    
    return %infolists;
}

# get list of hdata hooked by plugins in a hash with 3 indexes: plugin, name, xxx
sub get_hdata
{
    my %hdata;
    
    # get hdata hooked
    my $infolist = weechat::infolist_get("hook", "", "hdata");
    while (weechat::infolist_next($infolist))
    {
        my $hdata_name = weechat::infolist_string($infolist, "hdata_name");
        my $plugin = weechat::infolist_string($infolist, "plugin_name");
        $plugin = "weechat" if ($plugin eq "");
        
        # check if hdata is ignored or not
        my $ignore = 0;
        foreach my $mask (@ignore_hdata_plugins)
        {
            $ignore = 1 if ($plugin =~ /${mask}/);
        }
        
        if ($ignore ne 1)
        {
            $hdata{$plugin}{$hdata_name}{"description"} = weechat::infolist_string($infolist, "description");
            
            my $vars = "";
            my $lists = "";
            my $ptr_hdata = weechat::hdata_get($hdata_name);
            if ($ptr_hdata ne "")
            {
                my $str = weechat::hdata_get_string($ptr_hdata, "var_keys_values");
                my @items = split(/,/, $str);
                my %hdata2;
                foreach my $item (@items)
                {
                    my ($key, $value) = split(/:/, $item);
                    my $type = int($value) >> 16;
                    my $offset = int($value) & 0xFFFF;
                    my $stroffset = sprintf("%08d", $offset);
                    my $var_hdata = weechat::hdata_get_var_hdata($ptr_hdata, $key);
                    $var_hdata = ", hdata: '".$var_hdata."'" if ($var_hdata ne "");
                    $hdata2{$stroffset} = "'".$key."' (".weechat::hdata_get_var_type_string($ptr_hdata, $key).$var_hdata.")";
                }
                foreach my $offset (sort keys %hdata2)
                {
                    $vars .= " +\n" if ($vars ne "");
                    $vars .= "  ".$hdata2{$offset};
                }
                $hdata{$plugin}{$hdata_name}{"vars"} = "\n".$vars;
                
                $str = weechat::hdata_get_string($ptr_hdata, "list_keys");
                if ($str ne "")
                {
                    my @items = split(/,/, $str);
                    @items = sort(@items);
                    foreach my $item (@items)
                    {
                        $lists .= " +\n" if ($lists ne "");
                        $lists .= "  '".$item."'";
                    }
                    $lists = "\n".$lists;
                }
                else
                {
                    $lists = "\n  -";
                }
                $hdata{$plugin}{$hdata_name}{"lists"} = $lists;
            }
        }
    }
    weechat::infolist_free($infolist);
    
    return %hdata;
}

# get list of completions hooked by plugins in a hash with 3 indexes: plugin, item, xxx
sub get_completions
{
    my %completions;
    
    # get completions hooked
    my $infolist = weechat::infolist_get("hook", "", "completion");
    while (weechat::infolist_next($infolist))
    {
        my $completion_item = weechat::infolist_string($infolist, "completion_item");
        my $plugin = weechat::infolist_string($infolist, "plugin_name");
        $plugin = "weechat" if ($plugin eq "");
        
        # check if completion item is ignored or not
        my $ignore = 0;
        foreach my $mask (@ignore_completions_plugins)
        {
            $ignore = 1 if ($plugin =~ /${mask}/);
        }
        foreach my $mask (@ignore_completions_items)
        {
            $ignore = 1 if ($completion_item =~ /${mask}/);
        }
        
        if (($ignore ne 1) && ($completion_item ne ""))
        {
            $completions{$plugin}{$completion_item}{"description"} = weechat::infolist_string($infolist, "description");
        }
    }
    weechat::infolist_free($infolist);
    
    return %completions;
}

sub escape_string
{
    my $str = $_[0];
    $str =~ s/"/\\"/g;
    return $str;
}

sub escape_table
{
    my $str = $_[0];
    $str =~ s/\|/\\|/g;
    return $str;
}

# build doc files (command /docgen)
sub docgen
{
    my ($data, $buffer, $args) = ($_[0], $_[1], $_[2]);
    
    my @locale_list = @all_locale_list;
    @locale_list = split(/ /, $args) if ($args ne "");
    
    my %plugin_commands = get_commands();
    my %plugin_options = get_options();
    my %plugin_infos = get_infos();
    my %plugin_infos_hashtable = get_infos_hashtable();
    my %plugin_infolists = get_infolists();
    my %plugin_hdata = get_hdata();
    my %plugin_completions = get_completions();
    
    # get path and replace ~ by home if needed
    my $path = weechat::config_get_plugin("path");
    $path =~ s/^~\//$ENV{"HOME"}\//;
    
    my $old_locale = setlocale(LC_MESSAGES);
    
    # write to doc files, by locale
    my $num_files = 0;
    my $num_files_updated = 0;
    my $filename = "";
    
    foreach my $locale (@locale_list)
    {
        my $num_files_commands = 0;
        my $num_files_commands_updated = 0;
        my $num_files_options = 0;
        my $num_files_options_updated = 0;
        my $num_files_infos = 0;
        my $num_files_infos_updated = 0;
        my $num_files_infos_hashtable = 0;
        my $num_files_infos_hashtable_updated = 0;
        my $num_files_infolists = 0;
        my $num_files_infolists_updated = 0;
        my $num_files_hdata = 0;
        my $num_files_hdata_updated = 0;
        my $num_files_completions = 0;
        my $num_files_completions_updated = 0;
        
        setlocale(LC_MESSAGES, $locale.".UTF-8");
        $d = Locale::gettext->domain_raw("weechat");
        $d->codeset("UTF-8");
        $d->dir(weechat::info_get("weechat_localedir", ""));
        
        my $dir = $path."/".substr($locale, 0, 2)."/autogen/";
        if (-d $dir)
        {
            # write commands
            foreach my $plugin (keys %plugin_commands)
            {
                $filename = $dir."user/".$plugin."_commands.txt";
                if (open(FILE, ">".$filename.".tmp"))
                {
                    foreach my $command (sort keys %{$plugin_commands{$plugin}})
                    {
                        my $args = $plugin_commands{$plugin}{$command}{"args"};
                        $args = $d->get($args) if ($args ne "");
                        my @args_formats = split(/ \|\| /, $args);
                        my $description = $plugin_commands{$plugin}{$command}{"description"};
                        $description = $d->get($description) if ($description ne "");
                        my $args_description = $plugin_commands{$plugin}{$command}{"args_description"};
                        $args_description = $d->get($args_description) if ($args_description ne "");
                        
                        print FILE "[command]*`".$command."`* ".$description."::\n";
                        print FILE "........................................\n";
                        my $prefix = "/".$command."  ";
                        foreach my $format (@args_formats)
                        {
                            print FILE $prefix.$format."\n";
                            $prefix = " " x length($prefix);
                        }
                        if ($args_description ne "")
                        {
                            print FILE "\n";
                            my @lines = split(/\n/, $args_description);
                            foreach my $line (@lines)
                            {
                                print FILE $line."\n";
                            }
                        }
                        print FILE "........................................\n\n";
                        
                    }
                    #weechat::print("", "docgen: file ok: '$filename'");
                    my $rc = system("diff ".$filename." ".$filename.".tmp >/dev/null 2>&1");
                    if ($rc != 0)
                    {
                        system("mv -f ".$filename.".tmp ".$filename);
                        $num_files_updated++;
                        $num_files_commands_updated++;
                    }
                    else
                    {
                        system("rm ".$filename.".tmp");
                    }
                    $num_files++;
                    $num_files_commands++;
                    close(FILE);
                }
                else
                {
                    weechat::print("", weechat::prefix("error")."docgen error: unable to write file '$filename'");
                }
            }
            
            # write config options
            foreach my $config (keys %plugin_options)
            {
                $filename = $dir."user/".$config."_options.txt";
                if (open(FILE, ">".$filename.".tmp"))
                {
                    foreach my $section (sort keys %{$plugin_options{$config}})
                    {
                        foreach my $option (sort keys %{$plugin_options{$config}{$section}})
                        {
                            my $type = $plugin_options{$config}{$section}{$option}{"type"};
                            my $string_values = $plugin_options{$config}{$section}{$option}{"string_values"};
                            my $default_value = $plugin_options{$config}{$section}{$option}{"default_value"};
                            my $min = $plugin_options{$config}{$section}{$option}{"min"};
                            my $max = $plugin_options{$config}{$section}{$option}{"max"};
                            my $null_value_allowed = $plugin_options{$config}{$section}{$option}{"null_value_allowed"};
                            my $description = $plugin_options{$config}{$section}{$option}{"description"};
                            $description = $d->get($description) if ($description ne "");
                            my $type_nls = $type;
                            $type_nls = $d->get($type_nls) if ($type_nls ne "");
                            my $values = "";
                            if ($type eq "boolean")
                            {
                                $values = "on, off";
                            }
                            if ($type eq "integer")
                            {
                                if ($string_values ne "")
                                {
                                    $string_values =~ s/\|/, /g;
                                    $values = $string_values;
                                }
                                else
                                {
                                    $values = $min." .. ".$max;
                                }
                            }
                            if ($type eq "string")
                            {
                                $values = weechat_gettext("any string") if ($max <= 0);
                                $values = weechat_gettext("any char") if ($max == 1);
                                $values = weechat_gettext("any string")." (".weechat_gettext("max chars").": ".$max.")" if ($max > 1);
                                $default_value = "\"".escape_string($default_value)."\"";
                            }
                            if ($type eq "color")
                            {
                                $values = weechat_gettext("a WeeChat color name (default, black, "
                                                          ."(dark)gray, white, (light)red, (light)green, "
                                                          ."brown, yellow, (light)blue, (light)magenta, "
                                                          ."(light)cyan), a terminal color number or "
                                                          ."an alias; attributes are allowed before "
                                                          ."color (for text color only, not "
                                                          ."background): \"*\" for bold, \"!\" for "
                                                          ."reverse, \"_\" for underline");
                            }
                            
                            print FILE "* *".$config.".".$section.".".$option."*\n";
                            print FILE "** ".weechat_gettext("description").": `".$description."`\n";
                            print FILE "** ".weechat_gettext("type").": ".$type_nls."\n";
                            print FILE "** ".weechat_gettext("values").": ".$values." "
                                ."(".weechat_gettext("default value").": `".$default_value."`)\n";
                            if ($null_value_allowed eq 1)
                            {
                                print FILE "** ".weechat_gettext("undefined value allowed (null)")."\n";
                            }
                            print FILE "\n";
                        }
                    }
                    #weechat::print("", "docgen: file ok: '$filename'");
                    my $rc = system("diff ".$filename." ".$filename.".tmp >/dev/null 2>&1");
                    if ($rc != 0)
                    {
                        system("mv -f ".$filename.".tmp ".$filename);
                        $num_files_updated++;
                        $num_files_options_updated++;
                    }
                    else
                    {
                        system("rm ".$filename.".tmp");
                    }
                    $num_files++;
                    $num_files_options++;
                    close(FILE);
                }
                else
                {
                    weechat::print("", weechat::prefix("error")."docgen error: unable to write file '$filename'");
                }
            }
            
            # write infos hooked
            $filename = $dir."plugin_api/infos.txt";
            if (open(FILE, ">".$filename.".tmp"))
            {
                print FILE "[width=\"100%\",cols=\"^1,^2,6,6\",options=\"header\"]\n";
                print FILE "|========================================\n";
                print FILE "| ".weechat_gettext("Plugin")." | ".weechat_gettext("Name")
                    ." | ".weechat_gettext("Description")." | ".weechat_gettext("Arguments")."\n\n";
                foreach my $plugin (sort keys %plugin_infos)
                {
                    foreach my $info (sort keys %{$plugin_infos{$plugin}})
                    {
                        my $description = $plugin_infos{$plugin}{$info}{"description"};
                        $description = $d->get($description) if ($description ne "");
                        my $args_description = $plugin_infos{$plugin}{$info}{"args_description"};
                        $args_description = $d->get($args_description) if ($args_description ne "");
                        $args_description = "-" if ($args_description eq "");
                        
                        print FILE "| ".escape_table($plugin)." | ".escape_table($info)
                            ." | ".escape_table($description)." | ".escape_table($args_description)."\n\n";
                    }
                }
                print FILE "|========================================\n";
                #weechat::print("", "docgen: file ok: '$filename'");
                my $rc = system("diff ".$filename." ".$filename.".tmp >/dev/null 2>&1");
                if ($rc != 0)
                {
                    system("mv -f ".$filename.".tmp ".$filename);
                    $num_files_updated++;
                    $num_files_infos_updated++;
                }
                else
                {
                    system("rm ".$filename.".tmp");
                }
                $num_files++;
                $num_files_infos++;
                close(FILE);
            }
            else
            {
                weechat::print("", weechat::prefix("error")."docgen error: unable to write file '$filename'");
            }
            
            # write infos (hashtable) hooked
            $filename = $dir."plugin_api/infos_hashtable.txt";
            if (open(FILE, ">".$filename.".tmp"))
            {
                print FILE "[width=\"100%\",cols=\"^1,^2,6,6,6\",options=\"header\"]\n";
                print FILE "|========================================\n";
                print FILE "| ".weechat_gettext("Plugin")." | ".weechat_gettext("Name")
                    ." | ".weechat_gettext("Description")." | ".weechat_gettext("Hashtable (input)")
                    ." | ".weechat_gettext("Hashtable (output)")."\n\n";
                foreach my $plugin (sort keys %plugin_infos_hashtable)
                {
                    foreach my $info (sort keys %{$plugin_infos_hashtable{$plugin}})
                    {
                        my $description = $plugin_infos_hashtable{$plugin}{$info}{"description"};
                        $description = $d->get($description) if ($description ne "");
                        my $args_description = $plugin_infos_hashtable{$plugin}{$info}{"args_description"};
                        $args_description = $d->get($args_description) if ($args_description ne "");
                        $args_description = "-" if ($args_description eq "");
                        my $output_description = $plugin_infos_hashtable{$plugin}{$info}{"output_description"};
                        $output_description = $d->get($output_description) if ($output_description ne "");
                        $output_description = "-" if ($output_description eq "");
                        
                        print FILE "| ".escape_table($plugin)." | ".escape_table($info)
                            ." | ".escape_table($description)." | ".escape_table($args_description)
                            ." | ".escape_table($output_description)."\n\n";
                    }
                }
                print FILE "|========================================\n";
                #weechat::print("", "docgen: file ok: '$filename'");
                my $rc = system("diff ".$filename." ".$filename.".tmp >/dev/null 2>&1");
                if ($rc != 0)
                {
                    system("mv -f ".$filename.".tmp ".$filename);
                    $num_files_updated++;
                    $num_files_infos_hashtable_updated++;
                }
                else
                {
                    system("rm ".$filename.".tmp");
                }
                $num_files++;
                $num_files_infos_hashtable++;
                close(FILE);
            }
            else
            {
                weechat::print("", weechat::prefix("error")."docgen error: unable to write file '$filename'");
            }
            
            # write infolists hooked
            $filename = $dir."plugin_api/infolists.txt";
            if (open(FILE, ">".$filename.".tmp"))
            {
                print FILE "[width=\"100%\",cols=\"^1,^2,5,5,5\",options=\"header\"]\n";
                print FILE "|========================================\n";
                print FILE "| ".weechat_gettext("Plugin")." | ".weechat_gettext("Name")
                    ." | ".weechat_gettext("Description")." | ".weechat_gettext("Pointer")
                    ." | ".weechat_gettext("Arguments")."\n\n";
                foreach my $plugin (sort keys %plugin_infolists)
                {
                    foreach my $infolist (sort keys %{$plugin_infolists{$plugin}})
                    {
                        my $description = $plugin_infolists{$plugin}{$infolist}{"description"};
                        $description = $d->get($description) if ($description ne "");
                        my $pointer_description = $plugin_infolists{$plugin}{$infolist}{"pointer_description"};
                        $pointer_description = $d->get($pointer_description) if ($pointer_description ne "");
                        $pointer_description = "-" if ($pointer_description eq "");
                        my $args_description = $plugin_infolists{$plugin}{$infolist}{"args_description"};
                        $args_description = $d->get($args_description) if ($args_description ne "");
                        $args_description = "-" if ($args_description eq "");
                        
                        print FILE "| ".escape_table($plugin)." | ".escape_table($infolist)
                            ." | ".escape_table($description)." | ".escape_table($pointer_description)
                            ." | ".escape_table($args_description)."\n\n";
                    }
                }
                print FILE "|========================================\n";
                #weechat::print("", "docgen: file ok: '$filename'");
                my $rc = system("diff ".$filename." ".$filename.".tmp >/dev/null 2>&1");
                if ($rc != 0)
                {
                    system("mv -f ".$filename.".tmp ".$filename);
                    $num_files_updated++;
                    $num_files_infolists_updated++;
                }
                else
                {
                    system("rm ".$filename.".tmp");
                }
                $num_files++;
                $num_files_infolists++;
                close(FILE);
            }
            else
            {
                weechat::print("", weechat::prefix("error")."docgen error: unable to write file '$filename'");
            }
            
            # write hdata hooked
            $filename = $dir."plugin_api/hdata.txt";
            if (open(FILE, ">".$filename.".tmp"))
            {
                print FILE "[width=\"100%\",cols=\"^1,^2,5,5,5\",options=\"header\"]\n";
                print FILE "|========================================\n";
                print FILE "| ".weechat_gettext("Plugin")." | ".weechat_gettext("Name")
                    ." | ".weechat_gettext("Description")." | ".weechat_gettext("Variables")
                    ." | ".weechat_gettext("Lists")."\n\n";
                foreach my $plugin (sort keys %plugin_hdata)
                {
                    foreach my $hdata (sort keys %{$plugin_hdata{$plugin}})
                    {
                        my $description = $plugin_hdata{$plugin}{$hdata}{"description"};
                        $description = $d->get($description) if ($description ne "");
                        my $vars = $plugin_hdata{$plugin}{$hdata}{"vars"};
                        my $lists = $plugin_hdata{$plugin}{$hdata}{"lists"};
                        print FILE "| ".escape_table($plugin)." | ".escape_table($hdata)
                            ." | ".escape_table($description)." |".escape_table($vars)
                            ." |".escape_table($lists)."\n\n";
                    }
                }
                print FILE "|========================================\n";
                #weechat::print("", "docgen: file ok: '$filename'");
                my $rc = system("diff ".$filename." ".$filename.".tmp >/dev/null 2>&1");
                if ($rc != 0)
                {
                    system("mv -f ".$filename.".tmp ".$filename);
                    $num_files_updated++;
                    $num_files_hdata_updated++;
                }
                else
                {
                    system("rm ".$filename.".tmp");
                }
                $num_files++;
                $num_files_hdata++;
                close(FILE);
            }
            else
            {
                weechat::print("", weechat::prefix("error")."docgen error: unable to write file '$filename'");
            }
            
            # write completions hooked
            $filename = $dir."plugin_api/completions.txt";
            if (open(FILE, ">".$filename.".tmp"))
            {
                print FILE "[width=\"65%\",cols=\"^1,^2,8\",options=\"header\"]\n";
                print FILE "|========================================\n";
                print FILE "| ".weechat_gettext("Plugin")." | ".weechat_gettext("Name")
                    ." | ".weechat_gettext("Description")."\n\n";
                foreach my $plugin (sort keys %plugin_completions)
                {
                    foreach my $completion_item (sort keys %{$plugin_completions{$plugin}})
                    {
                        my $description = $plugin_completions{$plugin}{$completion_item}{"description"};
                        $description = $d->get($description) if ($description ne "");
                        
                        print FILE "| ".escape_table($plugin)." | ".escape_table($completion_item)
                            ." | ".escape_table($description)."\n\n";
                    }
                }
                print FILE "|========================================\n";
                #weechat::print("", "docgen: file ok: '$filename'");
                my $rc = system("diff ".$filename." ".$filename.".tmp >/dev/null 2>&1");
                if ($rc != 0)
                {
                    system("mv -f ".$filename.".tmp ".$filename);
                    $num_files_updated++;
                    $num_files_completions_updated++;
                }
                else
                {
                    system("rm ".$filename.".tmp");
                }
                $num_files++;
                $num_files_completions++;
                close(FILE);
            }
            else
            {
                weechat::print("", weechat::prefix("error")."docgen error: unable to write file '$filename'");
            }
        }
        else
        {
            weechat::print("", weechat::prefix("error")."docgen error: directory '$dir' does not exist");
        }
        my $total_files = $num_files_commands + $num_files_options
            + $num_files_infos + $num_files_infos_hashtable
            + $num_files_infolists + $num_files_hdata + $num_files_completions;
        my $total_files_updated = $num_files_commands_updated
            + $num_files_options_updated + $num_files_infos_updated
            + $num_files_infos_hashtable_updated + $num_files_infolists_updated
            + $num_files_hdata_updated + $num_files_completions_updated;
        weechat::print("",
                       sprintf ("docgen: %s: %3d files   (%2d cmd, %2d opt, %2d infos, "
                                ."%2d infos_hash, %2d infolists, %2d hdata, "
                                ."%2d complt)",
                                $locale, $total_files, $num_files_commands,
                                $num_files_options, $num_files_infos, $num_files_infos,
                                $num_files_infolists, $num_files_hdata,
                                $num_files_completions));
        weechat::print("",
                       sprintf ("               %3d updated (%2d cmd, %2d opt, %2d infos, "
                                ."%2d infos_hash, %2d infolists, %2d hdata, "
                                ."%2d complt)",
                                $total_files_updated, $num_files_commands_updated,
                                $num_files_options_updated, $num_files_infos_updated,
                                $num_files_infos_updated, $num_files_infolists_updated,
                                $num_files_hdata_updated, $num_files_completions_updated));
    }
    weechat::print("",
                   sprintf ("docgen: total: %d files, %d updated",
                            $num_files, $num_files_updated));
    
    setlocale(LC_MESSAGES, $old_locale);
    
    return weechat::WEECHAT_RC_OK;
}

sub docgen_completion
{
    my ($data, $completion_item, $buffer, $completion) = ($_[0], $_[1], $_[2], $_[3]);
    
    foreach my $locale (@all_locale_list)
    {
        weechat::hook_completion_list_add($completion, $locale, 0, weechat::WEECHAT_LIST_POS_SORT);
    }
    return weechat::WEECHAT_RC_OK;
}
