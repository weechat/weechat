#!/bin/sh
#
# SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Build WeeChat according to environment variables:
#   - JOBS: number of parallel jobs
#   - RUN_TESTS: set to 0 to disable run of tests
#
# Syntax to run the script with environment variables:
#   RUN_TESTS=0 JOBS=2 ./build_test.sh
#
# Syntax to run the script with arguments on command line:
#   ./build_test.sh [arguments]
#
# This script is used to build WeeChat in CI environment.

set -o errexit

# display commands
set -x

build_dir="build-tmp-$$"

# create build directory
mkdir "${build_dir}"
cd "${build_dir}"

if [ -z "${JOBS}" ]; then
    JOBS="$(nproc)"
fi

cmake .. "$@"
if [ -f "build.ninja" ]; then
    ninja -v
    sudo ninja install
else
    make VERBOSE=1 -j "${JOBS}"
    sudo make install
fi
if [ "$RUN_TESTS" != "0" ]; then
    ctest -V
fi
