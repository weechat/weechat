#!/bin/sh
#
# Copyright (C) 2015-2021 Sébastien Helleu <flashcode@flashtux.org>
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
# Build WeeChat Debian packages for a stable or development version.
#
# According to the version (stable/devel), a symbolic "debian" is made:
#        debian/ --> debian-stable/
#   or:  debian/ --> debian-devel/
#
# According to the distro version, a patch can be applied on debian files
# (patches are in directory tools/debian/patches).
#
# Script arguments:          Examples:
#
#   1. version               devel, devel-2, stable, stable-2, 1.9, 1.9-2
#   2. distro type/name      debian/sid, ubuntu/artful, raspbian/stretch
#
# The script can also just check that all Debian/Ubuntu patches apply fine
# with a single argument: "test-patches".
#
# Examples:
#
#   …/build-debian.sh devel debian/sid
#   …/build-debian.sh stable debian/stretch
#   …/build-debian.sh 1.9 ubuntu/artful
#   …/build-debian.sh 1.9-2 ubuntu/zesty
#
#   …/build-debian.sh test-patches
#
# Environment variables that can be used:
#
#   PACKAGER_NAME   Name of packager (for debian/changelog)
#   PACKAGER_EMAIL  E-mail of packager (for debian/changelog)
#   JOBS            Number of simultaneous jobs (for dpkg-buildpackage)
#                   (numeric or "auto" for dpkg >= 1.17.10)
#

# exit on any error
set -e

# default values for options from environment variables
DEFAULT_PACKAGER_NAME="Sébastien Helleu"
DEFAULT_PACKAGER_EMAIL="flashcode@flashtux.org"
DEFAULT_JOBS=""

usage ()
{
    RC=$1
    cat <<-EOF

Syntax: $0 devel|stable|<version> distro
        $0 test-patches

       version  version to build: stable, devel or specific version
                (debian package revision is allowed after name (default is 1),
                for example: devel-2, stable-2, 1.9-2)
        distro  the distro type/name (debian/sid, ubuntu/artful, raspbian/stretch, ...)

  test-patches  test that all Debian/Ubuntu patches apply fine (with git apply --check)

IMPORTANT: the current OS must match the distro, and the WeeChat sources
           must be checkouted in the appropriate version (this script
           does not checkout sources on a specific tag).

Examples:

  $0 devel debian/sid
  $0 stable debian/stretch
  $0 1.9 ubuntu/artful
  $0 1.9-2 ubuntu/zesty
  $0 test-patches

EOF
    exit "${RC}"
}

error ()
{
    echo >&2 "ERROR: $*"
    exit 1
}

error_usage ()
{
    echo >&2 "ERROR: $*"
    usage 1
}

test_patches ()
{
    set +e
    PATCHES_OK=0
    PATCHES_ERROR=0
    for file in "${ROOT_DIR}"/tools/debian/patches/*.patch; do
        echo "=== Testing patch ${file} ==="
        if git apply --check "${file}"; then
            PATCHES_OK=$((PATCHES_OK+1))
        else
            PATCHES_ERROR=$((PATCHES_ERROR+1))
        fi
    done
    echo "Patches: ${PATCHES_OK} OK, ${PATCHES_ERROR} in error."
    exit ${PATCHES_ERROR}
}

# ================================== START ==================================

# package name/email
[ -z "${PACKAGER_NAME}" ] && PACKAGER_NAME="${DEFAULT_PACKAGER_NAME}" && PACKAGER_EMAIL="${DEFAULT_PACKAGER_EMAIL}"
if [ -z "${PACKAGER_EMAIL}" ]; then
    echo >&2 "ERROR: PACKAGER_EMAIL must be set if PACKAGER_NAME is set."
    exit 1
fi

# simultaneous jobs for compilation (dpkg-buildpackage -jN)
[ -z "${JOBS}" ] && JOBS="${DEFAULT_JOBS}"

# check git repository
ROOT_DIR=$(git rev-parse --show-toplevel)
if [ -z "${ROOT_DIR}" ] || [ ! -d "${ROOT_DIR}/.git" ] || [ ! -d "${ROOT_DIR}/debian-stable" ]; then
    error "this script must be run from WeeChat git repository."
fi
cd "${ROOT_DIR}"

# check command line arguments
if [ $# -eq 0 ]; then
    usage 0
fi
if [ "$1" = "test-patches" ]; then
    test_patches
    exit 0
fi
if [ $# -lt 2 ]; then
    error_usage "missing arguments"
fi

# command line arguments
VERSION="$1"
DISTRO="$2"

# separate version and revision
# example: devel => devel / 1, stable-2 => stable / 2, 1.9-2 => 1.9 / 2
TMP_VERSION=$(expr "${VERSION}" : '\([^/]*\)-') || true
DEB_REVISION=""
if [ -n "${TMP_VERSION}" ]; then
    DEB_REVISION=$(expr "${VERSION}" : '[^-]*-\([^-]*\)') || true
    VERSION="${TMP_VERSION}"
fi
if [ -z "${DEB_REVISION}" ]; then
    DEB_REVISION="1"
fi

# convert version "stable" to its number
if [ "${VERSION}" = "stable" ]; then
    VERSION="$("${ROOT_DIR}/version.sh" stable)"
fi

if [ -z "${VERSION}" ]; then
    error_usage "unknown version"
fi

# extract distro type (debian, ubuntu, ...)
DISTRO_TYPE=$(expr "${DISTRO}" : '\([^/]*\)/') || true

# extract distro name (sid, jessie, wily, ...)
DISTRO_NAME=$(expr "${DISTRO}" : '[^/]*/\([a-z]*\)') || true

if [ -z "${DISTRO_TYPE}" ] || [ -z "${DISTRO_NAME}" ]; then
    error_usage "missing distro type/name"
fi

# set distro for dch
if [ "${DISTRO_TYPE}" = "ubuntu" ]; then
    # ubuntu
    if [ "${VERSION}" = "devel" ]; then
        DCH_DISTRO="UNRELEASED"
    else
        DCH_DISTRO="${DISTRO_NAME}"
    fi
else
    # debian/raspbian
    DCH_DISTRO="unstable"
fi

if [ "${VERSION}" = "devel" ]; then
    # devel packages: weechat-devel(-xxx)_X.Y-1~dev20150511_arch.deb
    DEB_DIR="debian-devel"
    DEB_NAME="weechat-devel"
    DEB_VERSION="$("${ROOT_DIR}/version.sh" devel)-1~dev$(date '+%Y%m%d')"
    if [ "${DEB_REVISION}" != "1" ]; then
        DEB_VERSION="${DEB_VERSION}-${DEB_REVISION}"
    fi
    DCH_CREATE="--create"
    DCH_URGENCY="low"
    DCH_CHANGELOG="Repository snapshot"
else
    # stable packages: weechat-(-xxx)_X.Y-1_arch.deb
    DEB_DIR="debian-stable"
    DEB_NAME="weechat"
    DEB_VERSION="${VERSION}-${DEB_REVISION}"
    DCH_CREATE=""
    DCH_URGENCY="medium"
    DCH_CHANGELOG="New upstream release"
fi

# display build info
echo "=== Building ${DEB_NAME}-${DEB_VERSION} on ${DISTRO_TYPE}/${DISTRO_NAME} (${DCH_DISTRO}) ==="

# ================================== BUILD ==================================

# apply patch (if needed, for old distros)
PATCH_FILE="${ROOT_DIR}/tools/debian/patches/weechat_${DISTRO_TYPE}_${DISTRO_NAME}.patch"
if [ -f "${PATCH_FILE}" ]; then
    echo " - Applying patch ${PATCH_FILE}"
    git apply "${PATCH_FILE}"
fi

# create a symlink "debian" -> "debian-{stable|devel}"
rm -f debian
ln -s -f "${DEB_DIR}" debian

# update debian changelog
if [ "${VERSION}" = "devel" ]; then
    rm -f "${DEB_DIR}/changelog"
fi

# create/update changelog
echo " - Updating changelog: ${DEB_NAME} ${DEB_VERSION} (${DCH_DISTRO}, ${DCH_URGENCY}), ${PACKAGER_NAME} <${PACKAGER_EMAIL}>: ${DCH_CHANGELOG}"
DEBFULLNAME="${PACKAGER_NAME}" DEBEMAIL="${PACKAGER_EMAIL}" dch "${DCH_CREATE}" --package "${DEB_NAME}" --newversion "${DEB_VERSION}" --distribution "${DCH_DISTRO}" --urgency "${DCH_URGENCY}" "${DCH_CHANGELOG}"

# build packages (without debug symbols)
DEB_BUILD_OPTIONS="noddebs" dpkg-buildpackage -us -uc -j${JOBS} --source-option="--tar-ignore=.git" --source-option="--tar-ignore=build*"

# all OK!
echo " - Build OK [${DEB_NAME}-${DEB_VERSION}]"
exit 0
