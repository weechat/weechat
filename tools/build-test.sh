#!/bin/sh
#
# Copyright (C) 2014-2020 Sébastien Helleu <flashcode@flashtux.org>
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

#
# Build WeeChat according to environment variables:
#   - BUILDTOOL: cmake or autotools
#   - BUILDARGS: arguments for cmake or configure commands
#
# Syntax to run the script with environment variables:
#   BUILDTOOL=cmake ./build-test.sh
#   BUILDTOOL=autotools ./build-test.sh
#   BUILDTOOL=cmake BUILDARGS="arguments" ./build-test.sh
#   BUILDTOOL=autotools BUILDARGS="arguments" ./build-test.sh
#
# Syntax to run the script with arguments on command line:
#   ./build-test.sh cmake [arguments]
#   ./build-test.sh autotools [arguments]
#
# This script is used to build WeeChat in CI environment.
#

run ()
{
    echo "Running \"$*\"..."
    if ! eval "$@"; then
        echo "ERROR"
        exit 1
    fi
}

BUILDDIR="build-tmp-$$"

if [ $# -ge 1 ]; then
    BUILDTOOL="$1"
    shift
fi

if [ $# -ge 1 ]; then
    BUILDARGS="$*"
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
    run "cmake .. -DENABLE_MAN=ON -DENABLE_DOC=ON -DENABLE_TESTS=ON ${BUILDARGS}"
    run "make VERBOSE=1 -j$(nproc)"
    run "sudo make install"
    run "ctest -V"
fi

if [ "$BUILDTOOL" = "autotools" ]; then
    # build with autotools
    run "../autogen.sh"
    run "../configure --enable-man --enable-doc --enable-tests ${BUILDARGS}"
    run "make -j$(nproc)"
    run "sudo make install"
    run "./tests/tests -v"
fi
