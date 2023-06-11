#!/bin/sh
#
# Copyright (C) 2023 Sébastien Helleu <flashcode@flashtux.org>
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

ROOT_DIR=$(git rev-parse --show-toplevel)

NEW_STABLE=$("${ROOT_DIR}/version.sh" stable)
NEW_DEVEL=$("${ROOT_DIR}/version.sh" devel)
NEW_DEVEL_FULL=$("${ROOT_DIR}/version.sh" devel-full)

case "$1" in
    stable ) NEW_STABLE="${NEW_DEVEL}"
             NEW_DEVEL_FULL="${NEW_DEVEL}" ;;
    major ) NEW_DEVEL=$(echo "${NEW_DEVEL}" | awk -F. '{$(NF-2) = $(NF-2) + 1; $(NF-1) = 0; $NF = 0; print}' OFS=.)
            NEW_DEVEL_FULL="${NEW_DEVEL}-dev" ;;
    minor ) NEW_DEVEL=$(echo "$NEW_DEVEL" | awk -F. '{$(NF-1) = $(NF-1) + 1; $NF = 0; print}' OFS=.)
            NEW_DEVEL_FULL="${NEW_DEVEL}-dev" ;;
    patch ) NEW_DEVEL=$(echo "$NEW_DEVEL" | awk -F. '{$NF = $NF + 1; print} ' OFS=.)
            NEW_DEVEL_FULL="${NEW_DEVEL}-dev" ;;
    * ) echo >&2 "ERROR: unknown version."
        exit 1 ;;
esac

sed -i \
    -e "s/^\(WEECHAT_STABLE\)=.*/\1=\"${NEW_STABLE}\"/" \
    -e "s/^\(WEECHAT_DEVEL\)=.*/\1=\"${NEW_DEVEL_FULL}\"/" \
    "${ROOT_DIR}/version.sh"
