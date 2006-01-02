#!/usr/bin/perl
#
# Copyright (c) 2003-2006 FlashCode <flashcode@flashtux.org>
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
# Build some XML code for WeeChat doc
# with weechat-curses command
#

@all_lang      = ("fr_FR", "en_US");
%all_encodings = ("fr_FR" => "iso-8859-1",
                  "en_US" => "iso-8859-1");
%all_types     = ("fr_FR" => "type",
                  "en_US" => "type",
                  "es_ES" => "tipo");
%all_values    = ("fr_FR" => "valeurs",
                  "en_US" => "values",
                  "es_ES" => "valores");
%all_default   = ("fr_FR" => "valeur par défaut",
                  "en_US" => "default values",
                  "es_ES" => "valor por defecto");
%all_desc      = ("fr_FR" => "description",
                  "en_US" => "description",
                  "es_ES" => "descripción");

foreach $lng (@all_lang)
{
    create_commands ($lng, $all_encodings{$lng},
                     "weechat-curses -w | tail +3", "weechat_commands");
    create_commands ($lng, $all_encodings{$lng},
                     "weechat-curses -i | tail +3", "irc_commands");
    create_key_func ($lng, $all_encodings{$lng},
                     "weechat-curses -f | tail +3", "key_functions");
    create_config ($lng, $all_encodings{$lng},
                   "weechat-curses -c | tail +3", "config");
    print "\n";
}

sub create_commands
{
    $lang = $_[0];
    $lang2 = substr ($lang, 0, 2);
    $encoding = $_[1];
    $command = $_[2];
    $file = $_[3];
    print "Creating $lang2/$file.xml ($lang)...\n";
    open XML, ">$lang2/$file.xml" or die "Error: can't write file!";
    print XML "<?xml version=\"1.0\" encoding=\"$encoding\"?>\n";
    
    $started = 0;
    $ENV{"LANG"} = $lang;
    foreach (`$command`)
    {
        if (/\* (.*)/)
        {
            print XML "</programlisting>\n" if ($started == 1);
            $started = 1;
            print XML "<command>$1</command>\n";
            print XML "<programlisting>";
        }
        else
        {
            chomp ($_);
            print XML "$_\n";
        }
    }
    print XML "</programlisting>\n";
    close XML;
    iconv_file ($lang2."/".$file, $encoding);
}

sub create_key_func
{
    $lang = $_[0];
    $lang2 = substr ($lang, 0, 2);
    $encoding = $_[1];
    $command = $_[2];
    $file = $_[3];
    print "Creating $lang2/$file.xml ($lang)...\n";
    open XML, ">$lang2/$file.xml" or die "Error: can't write file!";
    print XML "<?xml version=\"1.0\" encoding=\"$encoding\"?>\n";
    
    $ENV{"LANG"} = $lang;
    foreach (`$command`)
    {
        if (/\* (.*): (.*)/)
        {
            print XML "<row>\n";
            print XML "  <entry><literal>$1</literal></entry>\n";
            print XML "  <entry>$2</entry>\n";
            print XML "</row>\n";
        }
    }
    close XML;
    iconv_file ($lang2."/".$file, $encoding);
}

sub create_config
{
    $lang = $_[0];
    $lang2 = substr ($lang, 0, 2);
    $encoding = $_[1];
    $command = $_[2];
    $file = $_[3];
    print "Creating $lang2/$file.xml ($lang)...\n";
    open XML, ">$lang2/$file.xml" or die "Error: can't write file!";
    print XML "<?xml version=\"1.0\" encoding=\"$encoding\"?>\n";
    $type = "";
    $values = "";
    $default = "";
    $desc = "";
    
    $ENV{"LANG"} = $lang;
    foreach (`weechat-curses -c`)
    {
        if (/\* (.*):/)
        {
            print XML "<row>\n";
            print XML "  <entry><option>$1</option></entry>\n";
        }
        elsif (/  \. $all_types{$lang}: (.*)/)
        {
            $type = $1;
        }
        elsif (/  \. $all_values{$lang}: (.*)/)
        {
            $values = $1;
        }
        elsif (/  \. $all_default{$lang}: (.*)/)
        {
            $default = $1;
        }
        elsif (/  \. $all_desc{$lang}: (.*)/)
        {
            $_ = $1;
            s/(.*)/\u$1/;
            $desc = $_;
            print XML "  <entry>".$type."</entry>\n";
            print XML "  <entry>".$values."</entry>\n";
            print XML "  <entry>".$default."</entry>\n";
            print XML "  <entry>".$desc."</entry>\n";
            print XML "</row>\n";
        }
    }
    close XML;
    iconv_file ($lang2."/".$file, $encoding);
}

sub iconv_file
{
    print "Converting $_[0].xml to $_[1]...\n";
    system ("iconv -t $encoding -o $_[0].xml.$_[1] $_[0].xml");
    system ("mv $_[0].xml.$_[1] $_[0].xml");
}
