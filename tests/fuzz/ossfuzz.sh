#!/bin/sh
#
# SPDX-FileCopyrightText: 2025 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# This script is meant to be run by OSS-Fuzz, see:
# https://github.com/google/oss-fuzz/blob/master/projects/weechat/Dockerfile

set -o errexit

src_dir="${SRC}"/weechat
build_dir="${WORK}"/build

cd "${src_dir}"

# apply patch for Ubuntu Focal (needed because CMake version is too old)
git apply tools/debian/patches/weechat_ubuntu_focal.patch

mkdir -p "${build_dir}"
cd "${build_dir}"
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_CJSON=OFF \
    -DENABLE_COVERAGE=ON \
    -DENABLE_FUZZ=ON \
    "${src_dir}"
make -j "$(nproc)"

cp tests/fuzz/weechat_*_fuzzer "${OUT}"/
cp "${src_dir}"/tests/fuzz/core/*.dict "${OUT}"/
