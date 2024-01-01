#!/bin/sh
#
# Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
# Build compressed tarballs for WeeChat using git-archive.
#
# Syntax:  makedist.sh [<version> [<tree-ish> [<path>]]]
#
# Optional arguments:
#
#   version   WeeChat version, for example 4.0.0 or 4.1.0-dev
#             defaults to current devel version (output of "version.sh devel-full")
#   tree-ish  git tree-ish, example: v4.0.0
#             defaults to "HEAD"
#   path      where to put packages
#             defaults to current directory
#

set -o errexit

error ()
{
    echo >&2 "ERROR: $*"
    exit 1
}

# check git repository
root_dir=$(git rev-parse --show-toplevel)
if [ -z "${root_dir}" ] || [ ! -d "${root_dir}/.git" ]; then
    error "this script must be run from WeeChat git repository."
fi
cd "${root_dir}"

# default values
version="$("${root_dir}/version.sh" devel-full)"
treeish="HEAD"
outpath="$(pwd)"

if [ $# -ge 1 ]; then
    version=$1
fi
if [ $# -ge 2 ]; then
    treeish=$2
fi
if [ $# -ge 3 ]; then
    outpath=$(cd "$3"; pwd)
fi

prefix="weechat-${version}/"
file="${outpath}/weechat-${version}.tar"

echo "Building package ${file}.gz"
git archive --prefix="${prefix}" "${treeish}" | gzip -c >"${file}.gz"

echo "Building package ${file}.xz"
git archive --prefix="${prefix}" "${treeish}" | xz -c >"${file}.xz"
