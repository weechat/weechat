#!/bin/sh
#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

###
### common stuff
###

DIR=$(cd "$(dirname "$0")" || exit 1; pwd)
cd "$DIR" || exit 1

AUTOGEN_LOG=autogen.log

err ()
{
    echo "-------"
    echo "Error :"
    echo "---8<-----------------------------------"
    cat "$AUTOGEN_LOG"
    echo "----------------------------------->8---"
    exit 1
}

run ()
{
    printf "Running \"%s\"... " "$*"
    if "$@" >"$AUTOGEN_LOG" 2>&1 ; then
        echo "OK"
    else
        echo "FAILED"
        err
    fi
}

# remove autotools stuff
run rm -f config.h.in
run rm -f aclocal.m4 configure config.log config.status
run rm -rf "autom4te*.cache"

# remove libtool stuff
run rm -f libtool

# remove gettext stuff
run rm -f ABOUT-NLS
run rm -rf intl

# execute autoreconf cmds
run autoreconf -vi

# ending
rm -f "$AUTOGEN_LOG"
