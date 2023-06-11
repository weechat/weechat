#!/bin/sh
#
# Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#   - BUILDARGS: arguments for cmake command
#
# Syntax to run the script with environment variables:
#   BUILDARGS="arguments" ./build_test.sh
#
# Syntax to run the script with arguments on command line:
#   ./build_test.sh [arguments]
#
# This script is used to build WeeChat in CI environment.
#

set -o errexit

BUILDDIR="build-tmp-$$"

if [ $# -ge 1 ]; then
    BUILDARGS="$*"
fi

run ()
{
    "$@"
}

# display commands
set -x

# create build directory
mkdir "$BUILDDIR"
cd "$BUILDDIR"

run cmake .. -DENABLE_MAN=ON -DENABLE_DOC=ON -DENABLE_TESTS=ON "${BUILDARGS}"
if [ -f "build.ninja" ]; then
    ninja -v
    ninja -v changelog
    ninja -v rn
    sudo ninja install
else
    make VERBOSE=1 --jobs="$(nproc)"
    make VERBOSE=1 changelog
    make VERBOSE=1 rn
    sudo make install
fi
ctest -V
