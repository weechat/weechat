#!/bin/sh
#
# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

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

set -o errexit

error ()
{
    echo >&2 "ERROR: $*"
    exit 1
}

# Check git repository.
root_dir=$(git rev-parse --show-toplevel)
if [ -z "${root_dir}" ] || [ ! -e "${root_dir}/.git" ]; then
    error "this script must be run from WeeChat git repository."
fi
cd "${root_dir}"

# Default values
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
