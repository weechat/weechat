#!/bin/sh
#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
# Updates git version in config-git.h if the output of "git describe" has changed.
#
# Syntax:
#    set_git_version.sh <root_dir> <version> <header_file>
#
#       root_dir   : root directory with WeeChat files (to search .git/ directory)
#       version    : WeeChat version, for example 0.3.9 or 0.4.0-dev
#       header_file: file to update, for example config-git.h
#

if [ $# -lt 3 ]; then
    echo "Syntax: $0 <root_dir> <version> <header_file>"
    exit 1
fi

root_dir=$1
version=$2
header_file=$3

# debug:
#echo "pwd=$PWD, rootdir=${root_dir}, version=${version}, headerfile=${header_file}"

# read git version if we are in a devel/rc version and if we are in a repository
git_version=""
case ${version} in
*-*)
    # devel/rc version (like 0.4.0-dev or 0.4.0-rc1)
    if [ -e "${root_dir}/.git" ]; then
        git_version=$(cd "${root_dir}" && git describe 2>/dev/null)
    fi
    ;;
*)
    # stable version => no git version
    ;;
esac

# check if git version has changed
if [ ! -f "${header_file}" ]; then
    # header does not exist => create it
    echo "Creating file ${header_file} with git version: \"${git_version}\""
    echo "#define PACKAGE_VERSION_GIT \"${git_version}\"" >"${header_file}"
else
    if grep -q "#define PACKAGE_VERSION_GIT \"${git_version}\"" "${header_file}"; then
        # git version matches the file => NO update
        echo "File ${header_file} is up-to-date (git version: \"${git_version}\")"
    else
        # git version not found in file => update file with this git version
        echo "Updating file ${header_file} with git version: \"${git_version}\""
        sed "s/#define PACKAGE_VERSION_GIT \".*\"/#define PACKAGE_VERSION_GIT \"${git_version}\"/" "${header_file}" >"${header_file}.tmp"
        mv -f "${header_file}.tmp" "${header_file}"
    fi
fi
