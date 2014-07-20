#!/bin/sh
#
# Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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
# Build WeeChat with CMake or autotools, according to environment variable
# $BUILDTOOL or first script argument (if given).
#
# This script is used to build WeeChat in Travis CI environment.
#

run ()
{
    echo "Running \"$@\"..."
    eval $@
    if [ $? -ne 0 ]; then
        echo "ERROR"
        exit 1
    fi
}

BUILDDIR="build-tmp-$$"

if [ $# -ge 1 ]; then
    BUILDTOOL=$1
fi

if [ -z "$BUILDTOOL" ]; then
    echo "Syntax: $0 cmake|autotools"
    exit 1
fi

# create build directory
run "mkdir $BUILDDIR"
run "cd $BUILDDIR"

if [ "$BUILDTOOL" = "cmake" ]; then
    # build with CMake
    run "cmake .. -DENABLE_MAN=ON -DENABLE_DOC=ON"
    run "make VERBOSE=1"
    run "sudo make install"
fi

if [ "$BUILDTOOL" = "autotools" ]; then
    # build with autotools
    run "../autogen.sh"
    run "../configure --enable-man --enable-doc"
    run "make"
    run "sudo make install"
fi
