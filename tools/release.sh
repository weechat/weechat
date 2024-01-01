#!/bin/sh
#
# Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
# Make a new WeeChat release:
#   1. bump version
#   2. git commit + tag
#   3. compile, run, test, build changelog + rn + packages
#   4. test package: unpack, compile, run, test
#

set -o errexit

release_error ()
{
    [ $# -gt 0 ] && echo >&2 "ERROR: $*"
    exit 1
}

release_start ()
{
    root_dir="$(git rev-parse --show-toplevel)"
    if [ -n "$(git status --porcelain)" ]; then
        release_error "working directory not clean"
    fi
    version=$("${root_dir}/version.sh" devel)
    if git rev-parse "v${version}" 2>/dev/null; then
        release_error "tag v${version} already exists"
    fi
    msg=$(git log -1 --pretty=%B | tr -d "\n")
    if [ "${msg}" = "Version ${version}" ]; then
        release_error "commit for version already exists"
    fi
    date=$(date +"%Y-%m-%d")
    build_dir="${root_dir}/release/${version}"
    if [ -d "${build_dir}" ]; then
        release_error "directory ${build_dir} already exists"
    fi
    mkdir -p "${build_dir}"
    pkg_tar="${build_dir}/weechat-${version}.tar"
    chglog="${build_dir}/doc/ChangeLog.html"
    rn="${build_dir}/doc/ReleaseNotes.html"
}

release_bump_version ()
{
    "${root_dir}/tools/bump_version.sh" stable
    sed -i \
        -e "s/^\(== Version ${version}\) (under dev)$/\1 (${date})/" \
        "${root_dir}/ChangeLog.adoc" \
        "${root_dir}/ReleaseNotes.adoc"
}

release_commit_tag ()
{
    cd "${root_dir}"
    git commit -m "Version ${version}" version.sh ChangeLog.adoc ReleaseNotes.adoc || release_error "git commit error, release already done?"
    git tag -a "v${version}" -m "WeeChat ${version}"
}

release_build ()
{
    cd "${build_dir}"
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${build_dir}/install" \
        -DWEECHAT_HOME="${build_dir}/home" \
        -DENABLE_DOC=ON \
        -DENABLE_MAN=ON \
        -DENABLE_TESTS=ON \
        "${root_dir}"
    make install
    make changelog
    make rn
    make test CTEST_OUTPUT_ON_FAILURE=TRUE
    make dist
    version_weechat=$("${build_dir}/install/bin/weechat" --version)
    if [ "${version_weechat}" != "${version}" ]; then
        release_error "unexpected version \"${version_weechat}\" (expected: \"${version}\")"
    fi
}

release_test_pkg ()
{
    cd "${build_dir}"
    tar axvf "weechat-${version}.tar.xz"
    cd "weechat-${version}"
    pkg_dir="$(pwd)"
    script_version="${pkg_dir}/version.sh"
    [ "$("${script_version}" stable)" = "${version}" ] || release_error "wrong stable version in ${script_version}"
    [ "$("${script_version}" devel)" = "${version}" ] || release_error "wrong devel version in ${script_version}"
    [ "$("${script_version}" devel-full)" = "${version}" ] || release_error "wrong devel-full version in ${script_version}"
    mkdir build
    cd build
    pkg_build_dir="$(pwd)"
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${pkg_build_dir}/install" \
        -DWEECHAT_HOME="${pkg_build_dir}/home" \
        -DENABLE_DOC=ON \
        -DENABLE_MAN=ON \
        -DENABLE_TESTS=ON \
        "${pkg_dir}"
    make install
    make test CTEST_OUTPUT_ON_FAILURE=TRUE
    version_weechat=$("${pkg_build_dir}/install/bin/weechat" --version)
    if [ "${version_weechat}" != "${version}" ]; then
        release_error "unexpected version \"${version_weechat}\" (expected: \"${version}\")"
    fi
}

release_end ()
{
    # display a report about the release made
    echo
    echo "========================= WeeChat release status ========================="
    echo "  version  : ${version}"
    echo "  date     : ${date}"
    echo "  build dir: ${build_dir}"
    echo "  packages :"
    for pkg in "${pkg_tar}".*; do
        echo "    $pkg"
    done
    echo "  chglog/rn:"
    echo "    $chglog"
    echo "    $rn"
    echo "=========================================================================="
    echo
    echo "*** SUCCESS! ***"
    echo
}

release_start

release_bump_version
release_commit_tag
release_build
release_test_pkg

release_end

exit 0
