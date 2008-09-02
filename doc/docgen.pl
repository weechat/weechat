#
# Copyright (c) 2008 by FlashCode <flashcode@flashtux.org>
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

# Documentation generator for WeeChat: build XML include files with commands
# and options for WeeChat core and plugins.
#
# Instructions to build config files yourself in WeeChat directories (replace
# all paths with your path to WeeChat):
#     1.  run WeeChat and load this script, with following command:
#           /perl load ~/src/weechat/doc/docgen.pl
#     2.  change path to build in your doc/ directory:
#           /set plugins.var.perl.docgen.path = "~/src/weechat/doc"
#     3.  run docgen command:
#           /docgen
# XML files should be in ~/src/weechat/doc/xx/autogen/ (where xx is language)
#
# Script written on 2008-08-22 by FlashCode <flashcode@flashtux.org>
#

use strict;

use Locale::gettext;
use POSIX;            # needed for setlocale()

my $version = "0.1";

# -------------------------------[ config ]-------------------------------------

# default path where doc XML files will be written (should be doc/ in sources
# package tree)
# path must have subdirectories with languages and autogen directory:
#      path
#       |-- en
#       |   |-- autogen
#       |-- fr
#       |   |-- autogen
#       ...
my $default_path = "~/src/weechat/doc";

# list of locales for which we want to build XML doc files to include
my @locale_list = qw(en_US fr_FR de_DE);

# all commands/options/.. of following plugins will produce a file
# non-listed plugins will be ignored
# value: "c" = plugin may have many commands
#        "o" = write config options for plugin
# if plugin is listed without "c", that means plugin has only one command
# /name (where "name" # is name of plugin)
# Note: we consider core is a plugin called "weechat"
my %plugin_list = ("weechat" => "co", "alias"   => "",
                   "charset" => "c",  "debug"   => "co",
                   "demo"    => "co", "fifo"    => "co",
                   "irc"     => "co", "logger"  => "co",
                   "notify"  => "co", "perl"    => "",
                   "python"  => "",   "ruby"    => "",
                   "lua"     => "",   "xfer"    => "co");

# options to ignore
my @ignore_options = ("weechat\\.bar\\..*",
                      "irc\\.server\\..*");

# --------------------------------[ init ]--------------------------------------

weechat::register("docgen", "FlashCode <flashcode\@flashtux.org>", $version,
                  "GPL", "Doc generator for WeeChat 0.2.7", "", "");
weechat::hook_command("docgen", "Doc generator", "", "", "", "docgen");
weechat::config_set_plugin("path", $default_path)
    if (weechat::config_get_plugin("path") eq "");

# ------------------------------------------------------------------------------

# get list of commands in a hash with 3 indexes: plugin, command, info_name
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

# get list of config options in a hash with 4 indexes: config, section, option, info_name
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
            if ($plugin_list{$config} =~ /o/)
            {
                $options{$config}{$section}{$option}{"type"} = weechat::infolist_string($infolist, "type");
                $options{$config}{$section}{$option}{"string_values"} = weechat::infolist_string($infolist, "string_values");
                $options{$config}{$section}{$option}{"default_value"} = weechat::infolist_string($infolist, "default_value");
                $options{$config}{$section}{$option}{"min"} = weechat::infolist_integer($infolist, "min");
                $options{$config}{$section}{$option}{"max"} = weechat::infolist_integer($infolist, "max");
                $options{$config}{$section}{$option}{"description"} = weechat::infolist_string($infolist, "description");
            }
        }
    }
    weechat::infolist_free($infolist);
    
    return %options;
}

# build XML doc files (command /docgen)
sub docgen
{
    my %plugin_commands = get_commands();
    my %plugin_options = get_options();
    
    # xml header (comment) for all files
    my $xml_header =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        ."<!--\n"
        ."     ********* WARNING! *********\n"
        ."\n"
        ."     This file is autogenerated with docgen.pl script. *** DO NOT EDIT! ***\n"
        ."     docgen.pl builds XML doc files to include in many languages\n"
        ."-->\n\n";
    
    # get path and replace ~ by home if needed
    my $path = weechat::config_get_plugin("path");
    $path =~ s/^~\//$ENV{"HOME"}\//;
    
    # write to doc files, by locale
    my $num_files_written = 0;
    foreach my $locale (@locale_list)
    {
        my $num_files_commands_written = 0;
        my $num_files_options_written = 0;
        
        setlocale(LC_MESSAGES, $locale.".UTF-8");
        my $d = Locale::gettext->domain_raw("weechat");
        $d->codeset("UTF-8");
        $d->dir(weechat::info_get("weechat_localedir", ""));
        
        my $dir = $path."/".substr($locale, 0, 2)."/autogen/";
        if (-d $dir)
        {
            # write commands
            foreach my $plugin (keys %plugin_commands)
            {
                $filename = $dir.$plugin."_commands.xml";
                if (open(FILE, ">".$filename))
                {
                    print FILE $xml_header;
                    foreach my $command (sort keys %{$plugin_commands{$plugin}})
                    {
                        my $args = $plugin_commands{$plugin}{$command}{"args"};
                        $args = $d->get($args) if ($args ne "");
                        my $description = $plugin_commands{$plugin}{$command}{"description"};
                        $description = $d->get($description) if ($description ne "");
                        my $args_description = $plugin_commands{$plugin}{$command}{"args_description"};
                        $args_description = $d->get($args_description) if ($args_description ne "");
                        
                        print FILE "<command>".$command;
                        print FILE "  ".$args if ($args ne "");
                        print FILE "</command>\n";
                        print FILE "<programlisting>\n";
                        print FILE $description."\n" if ($description ne "");
                        print FILE "\n".$args_description."\n" if ($args_description ne "");
                        print FILE "</programlisting>\n";
                    }
                    #weechat::print("", "docgen: file ok: '$filename'");
                    $num_files_written++;
                    $num_files_commands_written++;
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
                $filename = $dir.$config."_options.xml";
                if (open(FILE, ">".$filename))
                {
                    print FILE $xml_header;
                    foreach my $section (sort keys %{$plugin_options{$config}})
                    {
                        foreach my $option (sort keys %{$plugin_options{$config}{$section}})
                        {
                            my $type = $plugin_options{$config}{$section}{$option}{"type"};
                            my $string_values = $plugin_options{$config}{$section}{$option}{"string_values"};
                            my $default_value = $plugin_options{$config}{$section}{$option}{"default_value"};
                            my $min = $plugin_options{$config}{$section}{$option}{"min"};
                            my $max = $plugin_options{$config}{$section}{$option}{"max"};
                            my $description = $plugin_options{$config}{$section}{$option}{"description"};
                            $description = $d->get($description) if ($description ne "");
                            my $type_nls = $type;
                            $type_nls = $d->get($type_nls) if ($type_nls ne "");
                            
                            print FILE "<row>\n";
                            print FILE "  <entry><option>".$config.".".$section.".".$option."</option></entry>\n";
                            print FILE "  <entry>".$type_nls."</entry>\n";
                            print FILE "  <entry>";
                            if ($string_values eq "")
                            {
                                print FILE "on, off" if ($type eq "boolean");
                                print FILE $min." .. ".$max if ($type eq "integer");
                            }
                            else
                            {
                                $string_values =~ s/\|/, /g;
                                print FILE $string_values;
                            }
                            print FILE "</entry>\n";
                            print FILE "  <entry>".$default_value."</entry>\n";
                            print FILE "  <entry>".$description."</entry>\n";
                            print FILE "</row>\n";
                        }
                    }
                    #weechat::print("", "docgen: file ok: '$filename'");
                    $num_files_written++;
                    $num_files_options_written++;
                    close(FILE);
                }
                else
                {
                    weechat::print("", weechat::prefix("error")."docgen error: unable to write file '$filename'");
                }
            }
        }
        else
        {
            weechat::print("", weechat::prefix("error")."docgen error: directory '$dir' does not exist");
        }
        my $total = $num_files_commands_written + $num_files_options_written;
        weechat::print("", "docgen: ".$locale.": ".$total." files written (".$num_files_commands_written." commands, ".$num_files_options_written." options)");
    }
    weechat::print("", "docgen: total: ".$num_files_written." files written");
    setlocale(LC_MESSAGES, "");
}
