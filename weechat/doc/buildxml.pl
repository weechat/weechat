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

@all_lang      = ("fr_FR", "en_US", "de_DE");
%all_encodings = ("fr_FR" => "iso-8859-1",
                  "en_US" => "iso-8859-1",
                  "de_DE" => "iso-8859-1");
%all_types     = ("fr_FR" => "type",
                  "en_US" => "type",
                  "de_DE" => "Typ",
                  "es_ES" => "tipo");
%all_values    = ("fr_FR" => "valeurs",
                  "en_US" => "values",
                  "de_DE" => "Werte",
                  "es_ES" => "valores");
%all_default   = ("fr_FR" => "valeur par défaut",
                  "en_US" => "default value",
                  "de_DE" => "Standardwert",
                  "es_ES" => "valor por defecto");
%all_desc      = ("fr_FR" => "description",
                  "en_US" => "description",
                  "de_DE" => "Beschreibung",
                  "es_ES" => "descripción");
$warning_do_not_edit = "\n<!-- ********* WARNING! *********\n\n"
    ."     This file is automatically built with a Perl script. DO NOT EDIT!\n"
    ."-->\n\n";

foreach $lng (@all_lang)
{
    create_commands ($lng, $all_encodings{$lng},
                     "weechat-curses -w | sed 1,2d", "weechat_commands");
    create_commands ($lng, $all_encodings{$lng},
                     "weechat-curses -i | sed 1,2d", "irc_commands");
    create_key_func ($lng, $all_encodings{$lng},
                     "weechat-curses -f | sed 1,2d", "key_functions");
    create_config ($lng, $all_encodings{$lng},
                   "weechat-curses -c | sed 1,2d", "config");
    print "\n";
}

sub toxml
{
    $_ = $_[0];
    $_ =~ s/&/&amp;/g;
    $_ =~ s/</&lt;/g;
    $_ =~ s/>/&gt;/g;
    return $_;
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
    print XML $warning_do_not_edit;
    
    $started = 0;
    $ENV{"LANG"} = $lang;
    foreach (`$command`)
    {
        if (/\* (.*)/)
        {
            print XML "</programlisting>\n" if ($started == 1);
            $started = 1;
            print XML "<command>".toxml($1)."</command>\n";
            print XML "<programlisting>";
        }
        else
        {
            chomp ($_);
            print XML toxml($_)."\n";
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
    print XML $warning_do_not_edit;
    
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
    print XML $warning_do_not_edit;
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
            print XML "  <entry>".toxml($type)."</entry>\n";
            print XML "  <entry>".toxml($values)."</entry>\n";
            print XML "  <entry>".toxml($default)."</entry>\n";
            print XML "  <entry>".toxml($desc)."</entry>\n";
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
