#!/bin/sh
#
# Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
# Bumps WeeChat version in file version.sh.
#
# Syntax:
#   bump_version.sh stable|major|minor|patch
#
#     stable       bump to the next stable: set stable and devel to
#                  `devel` value, with only digits and dots
#     major        bump the devel major version, set minor + patch to 0
#     minor        bump the devel minor version, set patch to 0
#     patch        bump the devel patch version
#

set -o errexit

if [ $# -lt 1 ]; then
    echo >&2 "usage: $0 stable|major|minor|patch"
    exit 1
fi

root_dir=$(git rev-parse --show-toplevel)

new_stable=$("${root_dir}/version.sh" stable)
new_devel=$("${root_dir}/version.sh" devel)
new_devel_full=$("${root_dir}/version.sh" devel-full)

case "$1" in
    stable ) new_stable="${new_devel}"
             new_devel_full="${new_devel}" ;;
    major ) new_devel=$(echo "${new_devel}" | awk -F. '{$(NF-2) = $(NF-2) + 1; $(NF-1) = 0; $NF = 0; print}' OFS=.)
            new_devel_full="${new_devel}-dev" ;;
    minor ) new_devel=$(echo "$new_devel" | awk -F. '{$(NF-1) = $(NF-1) + 1; $NF = 0; print}' OFS=.)
            new_devel_full="${new_devel}-dev" ;;
    patch ) new_devel=$(echo "$new_devel" | awk -F. '{$NF = $NF + 1; print} ' OFS=.)
            new_devel_full="${new_devel}-dev" ;;
    * ) echo >&2 "ERROR: unknown version."
        exit 1 ;;
esac

sed -i \
    -e "s/^\(weechat_stable\)=.*/\1=\"${new_stable}\"/" \
    -e "s/^\(weechat_devel\)=.*/\1=\"${new_devel_full}\"/" \
    "${root_dir}/version.sh"
