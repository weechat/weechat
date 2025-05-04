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
#   - RUN_TESTS: set to 0 to disable run of tests
#
# Syntax to run the script with environment variables:
#   RUN_TESTS=0 ./build_test.sh
#
# Syntax to run the script with arguments on command line:
#   ./build_test.sh [arguments]
#
# This script is used to build WeeChat in CI environment.
#

set -o errexit

# display commands
set -x

build_dir="build-tmp-$$"

# create build directory
mkdir "${build_dir}"
cd "${build_dir}"

cmake .. "$@"
if [ -f "build.ninja" ]; then
    ninja -v
    sudo ninja install
else
    make VERBOSE=1 -j "$(nproc)"
    sudo make install
fi
if [ "$RUN_TESTS" != "0" ]; then
    ctest -V
fi
