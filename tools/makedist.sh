#!/bin/sh
#
# Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Build gzip/bzip2/xz tarballs for WeeChat using git-archive.
#
# Syntax:  makedist.sh [<version> [<tree-ish> [<path>]]]
#
# Optional arguments:
#
#   version   WeeChat version, for example 1.6 or 1.7-dev
#             defaults to current devel version (output of "version.sh devel-full")
#   tree-ish  git tree-ish, example: v1.6
#             defaults to "HEAD"
#   path      where to put packages
#             defaults to current directory
#

# check git repository
ROOT_DIR=$(git rev-parse --show-toplevel)
if [ -z "${ROOT_DIR}" -o ! -d "${ROOT_DIR}/.git" ]; then
    echo "This script must be run from WeeChat git repository."
    exit 1
fi

# default values
VERSION="$(${ROOT_DIR}/version.sh devel-full)"
TREEISH="HEAD"
OUTPATH="$(pwd)"

if [ $# -ge 1 ]; then
    VERSION=$1
fi
if [ $# -ge 2 ]; then
    TREEISH=$2
fi
if [ $# -ge 3 ]; then
    OUTPATH=$(cd "$3"; pwd)
fi

cd "${ROOT_DIR}"

PREFIX="weechat-${VERSION}/"
FILE="${OUTPATH}/weechat-${VERSION}.tar"

echo "Building package ${FILE}.gz"
git archive --prefix=${PREFIX} ${TREEISH} | gzip -c >${FILE}.gz

echo "Building package ${FILE}.bz2"
git archive --prefix=${PREFIX} ${TREEISH} | bzip2 -c >${FILE}.bz2

echo "Building package ${FILE}.xz"
git archive --prefix=${PREFIX} ${TREEISH} | xz -c >${FILE}.xz
