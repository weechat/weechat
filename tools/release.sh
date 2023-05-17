#!/bin/sh
#
# Copyright (C) 2023 Sébastien Helleu <flashcode@flashtux.org>
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
    ROOT_DIR="$(git rev-parse --show-toplevel)"
    if [ -n "$(git status --porcelain)" ]; then
        release_error "working directory not clean"
    fi
    VERSION=$("${ROOT_DIR}/version.sh" devel)
    if git rev-parse "v${VERSION}" 2>/dev/null; then
        release_error "tag v${VERSION} already exists"
    fi
    MSG=$(git log -1 --pretty=%B | tr -d "\n")
    if [ "${MSG}" = "Version ${VERSION}" ]; then
        release_error "commit for version already exists"
    fi
    DATE=$(date +"%Y-%m-%d")
    BUILD_DIR="${ROOT_DIR}/release/${VERSION}"
    if [ -d "${BUILD_DIR}" ]; then
        release_error "directory ${BUILD_DIR} already exists"
    fi
    mkdir -p "${BUILD_DIR}"
    PKG_TAR="${BUILD_DIR}/weechat-${VERSION}.tar"
    CHGLOG="${BUILD_DIR}/doc/ChangeLog.html"
    RN="${BUILD_DIR}/doc/ReleaseNotes.html"
}

release_bump_version ()
{
    "${ROOT_DIR}/tools/bump_version.sh" stable
    sed -i \
        -e "s/^\(== Version ${VERSION}\) (under dev)$/\1 (${DATE})/" \
        "${ROOT_DIR}/ChangeLog.adoc" \
        "${ROOT_DIR}/ReleaseNotes.adoc"
}

release_commit_tag ()
{
    cd "${ROOT_DIR}"
    git commit -m "Version ${VERSION}" version.sh ChangeLog.adoc ReleaseNotes.adoc || release_error "git commit error, release already done?"
    git tag -a "v${VERSION}" -m "WeeChat ${VERSION}"
}

release_build ()
{
    cd "${BUILD_DIR}"
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${BUILD_DIR}/install" \
        -DWEECHAT_HOME="${BUILD_DIR}/home" \
        -DENABLE_DOC=ON \
        -DENABLE_MAN=ON \
        -DENABLE_TESTS=ON \
        "${ROOT_DIR}"
    make install
    make changelog
    make rn
    make test CTEST_OUTPUT_ON_FAILURE=TRUE
    make dist
    VERSION_WEECHAT=$("${BUILD_DIR}/install/bin/weechat" --version)
    if [ "${VERSION_WEECHAT}" != "${VERSION}" ]; then
        release_error "unexpected version \"${VERSION_WEECHAT}\" (expected: \"${VERSION}\")"
    fi
}

release_test_pkg ()
{
    cd "${BUILD_DIR}"
    tar axvf "weechat-${VERSION}.tar.xz"
    cd "weechat-${VERSION}"
    PKG_DIR="$(pwd)"
    SCRIPT_VERSION="${PKG_DIR}/version.sh"
    [ "$("${SCRIPT_VERSION}" stable)" = "${VERSION}" ] || release_error "wrong stable version in ${SCRIPT_VERSION}"
    [ "$("${SCRIPT_VERSION}" devel)" = "${VERSION}" ] || release_error "wrong devel version in ${SCRIPT_VERSION}"
    [ "$("${SCRIPT_VERSION}" devel-full)" = "${VERSION}" ] || release_error "wrong devel-full version in ${SCRIPT_VERSION}"
    mkdir build
    cd build
    PKG_BUILD_DIR="$(pwd)"
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${PKG_BUILD_DIR}/install" \
        -DWEECHAT_HOME="${PKG_BUILD_DIR}/home" \
        -DENABLE_DOC=ON \
        -DENABLE_MAN=ON \
        -DENABLE_TESTS=ON \
        "${PKG_DIR}"
    make install
    make test CTEST_OUTPUT_ON_FAILURE=TRUE
    VERSION_WEECHAT=$("${PKG_BUILD_DIR}/install/bin/weechat" --version)
    if [ "${VERSION_WEECHAT}" != "${VERSION}" ]; then
        release_error "unexpected version \"${VERSION_WEECHAT}\" (expected: \"${VERSION}\")"
    fi
}

release_end ()
{
    # display a report about the release made
    echo
    echo "========================= WeeChat release status ========================="
    echo "  version  : ${VERSION}"
    echo "  date     : ${DATE}"
    echo "  build dir: ${BUILD_DIR}"
    echo "  packages :"
    for PKG in "${PKG_TAR}".*; do
        echo "    $PKG"
    done
    echo "  chglog/rn:"
    echo "    $CHGLOG"
    echo "    $RN"
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
