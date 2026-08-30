#!/bin/sh
#
# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Update git version in config-git.h if the output of "git describe" has changed.
#
# Syntax:
#    set_git_version.sh <root_dir> <version> <header_file>
#
#       root_dir   : root directory with WeeChat files (to search .git/ directory)
#       version    : WeeChat version, for example 0.3.9 or 0.4.0-dev
#       header_file: file to update, for example config-git.h

if [ $# -lt 3 ]; then
    echo "Syntax: $0 <root_dir> <version> <header_file>"
    exit 1
fi

root_dir=$1
version=$2
header_file=$3

# Debug:
#echo "pwd=$PWD, rootdir=${root_dir}, version=${version}, headerfile=${header_file}"

# Read git version if we are in a devel/rc version and if we are in a
# repository.
git_version=""
case ${version} in
*-*)
    # Devel/rc version (like 0.4.0-dev or 0.4.0-rc1)
    if [ -e "${root_dir}/.git" ]; then
        git_version=$(cd "${root_dir}" && git describe 2>/dev/null)
    fi
    ;;
*)
    # Stable version => no git version
    ;;
esac

# Check if git version has changed.
if [ ! -f "${header_file}" ]; then
    # Header does not exist => create it.
    echo "Creating file ${header_file} with git version: \"${git_version}\""
    echo "#define PACKAGE_VERSION_GIT \"${git_version}\"" >"${header_file}"
else
    if grep -q "#define PACKAGE_VERSION_GIT \"${git_version}\"" "${header_file}"; then
        # Git version matches the file => NO update.
        echo "File ${header_file} is up-to-date (git version: \"${git_version}\")"
    else
        # Git version not found in file => update file with this git version.
        echo "Updating file ${header_file} with git version: \"${git_version}\""
        sed "s/#define PACKAGE_VERSION_GIT \".*\"/#define PACKAGE_VERSION_GIT \"${git_version}\"/" "${header_file}" >"${header_file}.tmp"
        mv -f "${header_file}.tmp" "${header_file}"
    fi
fi
