#!/bin/sh
#
# SPDX-FileCopyrightText: 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later
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

build_dir="build-tmp-$$"

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
mkdir "${build_dir}"
cd "${build_dir}"

run cmake .. -DENABLE_MAN=ON -DENABLE_DOC=ON -DENABLE_TESTS=ON "${BUILDARGS}"
if [ -f "build.ninja" ]; then
    ninja -v
    sudo ninja install
else
    make VERBOSE=1 -j "$(nproc)"
    sudo make install
fi
ctest -V
