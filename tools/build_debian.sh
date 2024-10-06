#!/bin/sh
#
# Copyright (C) 2015-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#   2. distro type/name      debian/sid, ubuntu/noble, raspbian/bookworm
#
# The script can also just check that all Debian/Ubuntu patches apply fine
# with a single argument: "test-patches".
#
# Examples:
#
#   …/build_debian.sh devel debian/sid
#   …/build_debian.sh stable debian/bookworm
#   …/build_debian.sh 4.3.0 ubuntu/noble
#   …/build_debian.sh 4.3.0-2 ubuntu/noble
#
#   …/build_debian.sh test-patches
#
# Environment variables that can be used:
#
#   PACKAGER_NAME   Name of packager (for debian/changelog)
#   PACKAGER_EMAIL  E-mail of packager (for debian/changelog)
#   JOBS            Number of simultaneous jobs (for dpkg-buildpackage)
#                   (numeric or "auto" for dpkg >= 1.17.10)
#   RETRY_BUILD     Retry build with `make` multiple times in case of failure
#                   (default is 0: do not retry and exit with error);
#                   when set, this patches the file debian/rules to run
#                   dh_auto_build multiple times in case of failure
#

set -o errexit

# default values for options from environment variables
default_packager_name="Sébastien Helleu"
default_packager_email="flashcode@flashtux.org"
default_jobs=""
default_retry_build="0"

usage ()
{
    rc=$1
    cat <<-EOF

Syntax: $0 devel|stable|<version> distro
        $0 test-patches

       version  version to build: stable, devel or specific version
                (debian package revision is allowed after name (default is 1),
                for example: devel-2, stable-2, 1.9-2)
        distro  the distro type/name (debian/sid, ubuntu/noble, raspbian/bookworm, ...)

  test-patches  test that all Debian/Ubuntu patches apply fine (with git apply --check)

IMPORTANT: the current OS must match the distro, and the WeeChat sources
           must be checked out in the appropriate version (this script
           does not checkout sources on a specific tag).

Examples:

  $0 devel debian/sid
  $0 stable debian/bookworm
  $0 4.3.0 ubuntu/noble
  $0 4.3.0-2 ubuntu/noble
  $0 test-patches

EOF
    exit "${rc}"
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
    patches_ok=0
    patches_error=0
    for file in "${root_dir}"/tools/debian/patches/*.patch; do
        if [ -f "${file}" ]; then
            echo "=== Testing patch ${file} ==="
            if git apply --check "${file}"; then
                patches_ok=$((patches_ok+1))
            else
                patches_error=$((patches_error+1))
            fi
        fi
    done
    echo "Patches: ${patches_ok} OK, ${patches_error} in error."
    exit ${patches_error}
}

# ================================== START ==================================

# package name/email
[ -z "${PACKAGER_NAME}" ] && PACKAGER_NAME="${default_packager_name}" && PACKAGER_EMAIL="${default_packager_email}"
if [ -z "${PACKAGER_EMAIL}" ]; then
    echo >&2 "ERROR: PACKAGER_EMAIL must be set if PACKAGER_NAME is set."
    exit 1
fi

# simultaneous jobs for compilation (dpkg-buildpackage -jN)
[ -z "${JOBS}" ] && JOBS="${default_jobs}"

# retry build
[ -z "${RETRY_BUILD}" ] && RETRY_BUILD="${default_retry_build}"

# check git repository
root_dir=$(git rev-parse --show-toplevel)
if [ -z "${root_dir}" ] || [ ! -d "${root_dir}/.git" ] || [ ! -d "${root_dir}/debian-stable" ]; then
    error "this script must be run from WeeChat git repository."
fi
cd "${root_dir}"

# check command line arguments
if [ $# -eq 0 ]; then
    usage 0
fi
if [ "$1" = "test-patches" ]; then
    test_patches
fi
if [ $# -lt 2 ]; then
    error_usage "missing arguments"
fi

# command line arguments
version="$1"
distro="$2"

# separate version and revision
# example: devel => devel / 1, stable-2 => stable / 2, 1.9-2 => 1.9 / 2
tmp_version=$(expr "${version}" : '\([^/]*\)-') || true
DEB_REVISION=""
if [ -n "${tmp_version}" ]; then
    DEB_REVISION=$(expr "${version}" : '[^-]*-\([^-]*\)') || true
    version="${tmp_version}"
fi
if [ -z "${DEB_REVISION}" ]; then
    DEB_REVISION="1"
fi

# convert version "stable" to its number
if [ "${version}" = "stable" ]; then
    version="$("${root_dir}/version.sh" stable)"
fi

if [ -z "${version}" ]; then
    error_usage "unknown version"
fi

# extract distro type (debian, ubuntu, ...)
distro_type=$(expr "${distro}" : '\([^/]*\)/') || true

# extract distro name (sid, jessie, wily, ...)
distro_name=$(expr "${distro}" : '[^/]*/\([a-z]*\)') || true

if [ -z "${distro_type}" ] || [ -z "${distro_name}" ]; then
    error_usage "missing distro type/name"
fi

# set distro for dch
if [ "${distro_type}" = "ubuntu" ]; then
    # ubuntu
    if [ "${version}" = "devel" ]; then
        DCH_DISTRO="UNRELEASED"
    else
        DCH_DISTRO="${distro_name}"
    fi
else
    # debian/raspbian
    DCH_DISTRO="unstable"
fi

if [ "${version}" = "devel" ]; then
    # devel packages: weechat-devel(-xxx)_X.Y-1~dev20150511_arch.deb
    DEB_DIR="debian-devel"
    DEB_NAME="weechat-devel"
    DEB_VERSION="$("${root_dir}/version.sh" devel)-1~dev$(date '+%Y%m%d')"
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
    DEB_VERSION="${version}-${DEB_REVISION}"
    DCH_CREATE=""
    DCH_URGENCY="medium"
    DCH_CHANGELOG="New upstream release"
fi

# display build info
echo "=== Building ${DEB_NAME}-${DEB_VERSION} on ${distro_type}/${distro_name} (${DCH_DISTRO}) ==="

# ================================== BUILD ==================================

# apply patch (if needed, for old distros)
patch_file="${root_dir}/tools/debian/patches/weechat_${distro_type}_${distro_name}.patch"
if [ -f "${patch_file}" ]; then
    echo " - Applying patch ${patch_file}"
    git apply "${patch_file}"
fi

# create a symlink "debian" -> "debian-{stable|devel}"
rm -f debian
ln -s -f "${DEB_DIR}" debian

# update debian changelog
if [ "${version}" = "devel" ]; then
    rm -f "${DEB_DIR}/changelog"
fi

# create/update changelog
echo " - Updating changelog: ${DEB_NAME} ${DEB_VERSION} (${DCH_DISTRO}, ${DCH_URGENCY}), ${PACKAGER_NAME} <${PACKAGER_EMAIL}>: ${DCH_CHANGELOG}"
DEBFULLNAME="${PACKAGER_NAME}" DEBEMAIL="${PACKAGER_EMAIL}" dch "${DCH_CREATE}" --package "${DEB_NAME}" --newversion "${DEB_VERSION}" --distribution "${DCH_DISTRO}" --urgency "${DCH_URGENCY}" "${DCH_CHANGELOG}"

# if retry build is enabled, patch debian/rules to run dh_auto_build
# multiple times if needed
if [ "${RETRY_BUILD}" != "0" ]; then
    cat <<EOF >> debian/rules

override_dh_auto_build:
	dh_auto_build || dh_auto_build || dh_auto_build || dh_auto_build
EOF
fi

# build packages (without debug symbols)
DEB_BUILD_OPTIONS="noddebs" dpkg-buildpackage -us -uc --jobs="${JOBS}"

# all OK!
echo " - Build OK [${DEB_NAME}-${DEB_VERSION}]"
exit 0
