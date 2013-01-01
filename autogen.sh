#!/bin/sh
#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2005 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2005-2006 Emmanuel Bouthenot <kolter@openics.org>
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

###
### common stuff
###

AUTOGEN_LOG=autogen.log

err ()
{
    echo "-------"
    echo "Error :"
    echo "---8<-----------------------------------"
    cat $AUTOGEN_LOG
    echo "----------------------------------->8---"
    exit 1
}

run ()
{
    echo -n "Running \"$@\"..."
    eval $@ >$AUTOGEN_LOG 2>&1
    if [ $? = 0 ] ; then
        echo " OK"
    else
        echo " FAILED"
        err
    fi
}

###
### cleanning part
###
#  remove autotools stuff
run "rm -rf config"
run "rm -f config.h.in"
run "rm -f aclocal.m4 configure config.log config.status"
run "rm -rf autom4te*.cache"
# remove libtool stuff
run "rm -f libtool"
# remove gettext stuff
run "rm -f ABOUT-NLS"
run "rm -rf intl"

###
### configuration part
###
# create the config directory
run "mkdir -p config/m4"
run "mkdir intl"

# execute autotools cmds
run "autopoint -f"
run "libtoolize --automake --force --copy"
run "aclocal --force -I config/m4"
run "autoheader"
run "autoconf"
run "automake --add-missing --copy --gnu"

# ending
rm -f $AUTOGEN_LOG
