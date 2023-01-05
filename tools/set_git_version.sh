#!/bin/sh
#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#    set_git_version.sh <rootdir> <version> <headerfile>
#
#       rootdir   : root directory with WeeChat files (to search .git/ directory)
#       version   : WeeChat version, for example 0.3.9 or 0.4.0-dev
#       headerfile: file to update, for example config-git.h
#

if [ $# -lt 3 ]; then
    echo "Syntax: $0 <rootdir> <version> <headerfile>"
    exit 1
fi

ROOTDIR=$1
VERSION=$2
HEADERFILE=$3

# debug:
#echo "pwd=$PWD, rootdir=$ROOTDIR, version=$VERSION, headerfile=$HEADERFILE"

# read git version if we are in a devel/rc version and if we are in a repository
GIT_VERSION=""
case ${VERSION} in
*-*)
    # devel/rc version (like 0.4.0-dev or 0.4.0-rc1)
    if [ -d "${ROOTDIR}/.git" ]; then
        GIT_VERSION=$(cd "${ROOTDIR}" && git describe 2>/dev/null)
    fi
    ;;
*)
    # stable version => no git version
    ;;
esac

# check if git version has changed
if [ ! -f "${HEADERFILE}" ]; then
    # header does not exist => create it
    echo "Creating file ${HEADERFILE} with git version: \"${GIT_VERSION}\""
    echo "#define PACKAGE_VERSION_GIT \"${GIT_VERSION}\"" >"${HEADERFILE}"
else
    if grep -q "#define PACKAGE_VERSION_GIT \"${GIT_VERSION}\"" "${HEADERFILE}"; then
        # git version matches the file => NO update
        echo "File ${HEADERFILE} is up-to-date (git version: \"${GIT_VERSION}\")"
    else
        # git version not found in file => update file with this git version
        echo "Updating file ${HEADERFILE} with git version: \"${GIT_VERSION}\""
        sed "s/#define PACKAGE_VERSION_GIT \".*\"/#define PACKAGE_VERSION_GIT \"${GIT_VERSION}\"/" "${HEADERFILE}" >"${HEADERFILE}.tmp"
        mv -f "${HEADERFILE}.tmp" "${HEADERFILE}"
    fi
fi
