#!/bin/sh
#
# SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Build WeeChat Debian packages for a stable or development version.
#
# Packaging files are in directory "debian", with the names of stable
# packages (weechat, weechat-curses, ...); for a devel version, packages are
# renamed on the fly (weechat-devel, weechat-devel-curses, ...).
#
# According to the distro version, a patch can be applied on debian files
# (patches are in directory tools/debian/patches).
#
# IMPORTANT: this script modifies files in directory "debian" (patch, devel
# packages, changelog); to restore them, run:
#   git checkout -- debian && git clean -fd debian
#
# Script arguments:          Examples:
#
#   1. version               devel, devel-2, stable, stable-2, 1.9, 1.9-2
#   2. distro type/name      debian/sid, ubuntu/resolute
#
# The script can also just check that all Debian/Ubuntu patches apply fine
# with a single argument: "test-patches".
#
# Examples:
#
#   …/build_debian.sh devel debian/sid
#   …/build_debian.sh stable debian/trixie
#   …/build_debian.sh 4.3.0 ubuntu/resolute
#   …/build_debian.sh 4.3.0-2 ubuntu/resolute
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

set -o errexit

# Default values for options from environment variables
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
        distro  the distro type/name (debian/sid, ubuntu/resolute, ...)

  test-patches  test that all Debian/Ubuntu patches apply fine (with git apply --check)

IMPORTANT: the current OS must match the distro, and the WeeChat sources
           must be checked out in the appropriate version (this script
           does not checkout sources on a specific tag).

Examples:

  $0 devel debian/sid
  $0 stable debian/trixie
  $0 4.3.0 ubuntu/resolute
  $0 4.3.0-2 ubuntu/resolute
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

# Convert files in directory "debian" to build devel packages:
# weechat-xxx -> weechat-devel-xxx.

convert_to_devel ()
{
    # Rename package files: weechat-xxx.* -> weechat-devel-xxx.*
    for file in debian/weechat-*; do
        if [ -f "${file}" ]; then
            mv "${file}" "debian/weechat-devel-${file#debian/weechat-}"
        fi
    done

    # Rename source and binary packages in debian/control, set maintainer
    # and make each devel package conflict with its stable package
    # (only binary packages built from this source are renamed).
    awk -v maintainer="${PACKAGER_NAME} <${PACKAGER_EMAIL}>" '
        function rename_pkgs(line,    result, name, before, after) {
            result = ""
            while (match(line, /weechat[-a-z0-9]*/)) {
                name = substr(line, RSTART, RLENGTH)
                before = (RSTART > 1) ? substr(line, RSTART - 1, 1) : " "
                after = substr(line, RSTART + RLENGTH, 1)
                result = result substr(line, 1, RSTART - 1)
                if ((name in pkgs) && before ~ /[ ,|]/ && after ~ /^([ ,|]|)$/) {
                    sub(/^weechat/, "weechat-devel", name)
                }
                result = result name
                line = substr(line, RSTART + RLENGTH)
            }
            return result line
        }
        NR == FNR {
            if ($1 == "Package:") pkgs[$2] = 1
            next
        }
        /^[^ ]/ {
            relation = ($1 ~ /^(Pre-Depends|Depends|Recommends|Suggests|Enhances|Breaks|Conflicts|Replaces|Provides):$/)
        }
        /^Source: / { $0 = "Source: weechat-devel" }
        /^Maintainer: / { $0 = "Maintainer: " maintainer }
        /^Package: / { stable_pkg = $2; $0 = "Package: weechat-devel" substr($2, 8) }
        /^Description: / && stable_pkg != "" {
            print "Conflicts: " stable_pkg
            stable_pkg = ""
        }
        relation { $0 = rename_pkgs($0) }
        { print }
    ' debian/control debian/control > debian/control.devel
    mv debian/control.devel debian/control

    # Doc directories and menu entry.
    sed -i \
        -e 's|share/doc/weechat-doc/|share/doc/weechat-devel-doc/|g' \
        -e 's|share/doc/weechat/html|share/doc/weechat-devel/html|g' \
        debian/weechat-devel-doc.*
    sed -i 's|?package(weechat-curses)|?package(weechat-devel-curses)|' debian/weechat-devel-curses.menu

    # Devel packages have their own changelog (created with dch).
    rm -f debian/changelog
}

# ================================== START ==================================

# Package name/email
[ -z "${PACKAGER_NAME}" ] && PACKAGER_NAME="${default_packager_name}" && PACKAGER_EMAIL="${default_packager_email}"
if [ -z "${PACKAGER_EMAIL}" ]; then
    echo >&2 "ERROR: PACKAGER_EMAIL must be set if PACKAGER_NAME is set."
    exit 1
fi

# Simultaneous jobs for compilation (dpkg-buildpackage -jN)
[ -z "${JOBS}" ] && JOBS="${default_jobs}"

# Retry build
[ -z "${RETRY_BUILD}" ] && RETRY_BUILD="${default_retry_build}"

# Check git repository.
root_dir=$(git rev-parse --show-toplevel)
if [ -z "${root_dir}" ] || [ ! -e "${root_dir}/.git" ] || [ ! -d "${root_dir}/debian" ]; then
    error "this script must be run from WeeChat git repository."
fi
cd "${root_dir}"

# Check command line arguments.
if [ $# -eq 0 ]; then
    usage 0
fi
if [ "$1" = "test-patches" ]; then
    test_patches
fi
if [ $# -lt 2 ]; then
    error_usage "missing arguments"
fi

# Command line arguments
version="$1"
distro="$2"

# Separate version and revision.
# Example: devel => devel / 1, stable-2 => stable / 2, 1.9-2 => 1.9 / 2
tmp_version=$(expr "${version}" : '\([^/]*\)-') || true
DEB_REVISION=""
if [ -n "${tmp_version}" ]; then
    DEB_REVISION=$(expr "${version}" : '[^-]*-\([^-]*\)') || true
    version="${tmp_version}"
fi
if [ -z "${DEB_REVISION}" ]; then
    DEB_REVISION="1"
fi

# Convert version "stable" to its number.
if [ "${version}" = "stable" ]; then
    version="$("${root_dir}/version.sh" stable)"
fi

if [ -z "${version}" ]; then
    error_usage "unknown version"
fi

# Extract distro type (debian, ubuntu, ...).
distro_type=$(expr "${distro}" : '\([^/]*\)/') || true

# Extract distro name (sid, jessie, wily, ...).
distro_name=$(expr "${distro}" : '[^/]*/\([a-z]*\)') || true

if [ -z "${distro_type}" ] || [ -z "${distro_name}" ]; then
    error_usage "missing distro type/name"
fi

# Set distro for dch.
if [ "${distro_type}" = "ubuntu" ]; then
    # ubuntu
    if [ "${version}" = "devel" ]; then
        DCH_DISTRO="UNRELEASED"
    else
        DCH_DISTRO="${distro_name}"
    fi
else
    # debian
    DCH_DISTRO="unstable"
fi

if [ "${version}" = "devel" ]; then
    # Devel packages: weechat-devel(-xxx)_X.Y-1~dev20150511_arch.deb
    DEB_NAME="weechat-devel"
    DEB_VERSION="$("${root_dir}/version.sh" devel)-1~dev$(date '+%Y%m%d')"
    if [ "${DEB_REVISION}" != "1" ]; then
        DEB_VERSION="${DEB_VERSION}-${DEB_REVISION}"
    fi
    DCH_CREATE="--create"
    DCH_URGENCY="low"
    DCH_CHANGELOG="Repository snapshot"
else
    # Stable packages: weechat-(-xxx)_X.Y-1_arch.deb
    DEB_NAME="weechat"
    DEB_VERSION="${version}-${DEB_REVISION}"
    DCH_CREATE=""
    DCH_URGENCY="medium"
    DCH_CHANGELOG="New upstream release"
fi

# Display build info.
echo "=== Building ${DEB_NAME}-${DEB_VERSION} on ${distro_type}/${distro_name} (${DCH_DISTRO}) ==="

# ================================== BUILD ==================================

# Apply patch (if needed, for old distros).
patch_file="${root_dir}/tools/debian/patches/weechat_${distro_type}_${distro_name}.patch"
if [ -f "${patch_file}" ]; then
    echo " - Applying patch ${patch_file}"
    git apply "${patch_file}"
fi

# Rename packages for devel version (weechat-xxx -> weechat-devel-xxx).
if [ "${version}" = "devel" ]; then
    echo " - Converting packages to devel"
    convert_to_devel
fi

# Create/update changelog.
echo " - Updating changelog: ${DEB_NAME} ${DEB_VERSION} (${DCH_DISTRO}, ${DCH_URGENCY}), ${PACKAGER_NAME} <${PACKAGER_EMAIL}>: ${DCH_CHANGELOG}"
DEBFULLNAME="${PACKAGER_NAME}" DEBEMAIL="${PACKAGER_EMAIL}" dch "${DCH_CREATE}" --package "${DEB_NAME}" --newversion "${DEB_VERSION}" --distribution "${DCH_DISTRO}" --urgency "${DCH_URGENCY}" "${DCH_CHANGELOG}"

# If retry build is enabled, patch debian/rules to run dh_auto_build
# multiple times if needed.
if [ "${RETRY_BUILD}" != "0" ]; then
    cat <<EOF >> debian/rules

override_dh_auto_build:
	dh_auto_build || dh_auto_build || dh_auto_build || dh_auto_build || dh_auto_build || dh_auto_build
EOF
fi

# Build packages (without debug symbols).
DEB_BUILD_OPTIONS="noddebs" dpkg-buildpackage -us -uc --jobs="${JOBS}"

# All OK!
echo " - Build OK [${DEB_NAME}-${DEB_VERSION}]"
exit 0
