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
# Returns current stable or devel version of WeeChat.
#
# Syntax:
#   version.sh <name>
#
#   name is one of:
#
#     stable         the current stable (e.g. "4.0.2")
#     stable-major   the major version of stable ("4" for "4.0.2")
#     stable-minor   the minor version of stable ("0" for "4.0.2")
#     stable-patch   the patch version of stable ("2" for "4.0.2")
#     stable-number  the stable version as hex number ("0x04000200" for "4.0.2")
#     devel          the devel with only digits/dots (e.g. "4.1.0")
#     devel-full     the full devel (e.g. "4.1.0-dev")
#     devel-major    the major version of devel ("4" for "4.1.0-dev")
#     devel-minor    the minor version of devel ("1" for "4.1.0-dev")
#     devel-patch    the patch version of devel ("0-dev" for "4.1.0-dev")
#     devel-number   the devel version as hex number ("0x04010000" for "4.1.0-dev")
#

weechat_stable="4.3.1"
weechat_devel="4.3.1"

stable_major=$(echo "${weechat_stable}" | cut -d"." -f1)
stable_minor=$(echo "${weechat_stable}" | cut -d"." -f2)
stable_patch=$(echo "${weechat_stable}" | cut -d"." -f3-)
stable_patch_digits=$(echo "${weechat_stable}" | cut -d"." -f3- | cut -d"-" -f1)

devel_major=$(echo "${weechat_devel}" | cut -d"." -f1)
devel_minor=$(echo "${weechat_devel}" | cut -d"." -f2)
devel_patch=$(echo "${weechat_devel}" | cut -d"." -f3-)
devel_patch_digits=$(echo "${weechat_devel}" | cut -d"." -f3- | cut -d"-" -f1)

if [ $# -lt 1 ]; then
    echo >&2 "Syntax: $0 <name>"
    echo >&2 "name: stable, stable-major, stable-minor, stable-patch, stable-number,"
    echo >&2 "      devel, devel-full, devel-major, devel-minor, devel-patch, devel-number"
    exit 1
fi

case $1 in
    # stable
    stable ) echo "${weechat_stable}" ;;
    stable-major ) echo "${stable_major}" ;;
    stable-minor ) echo "${stable_minor}" ;;
    stable-patch ) echo "${stable_patch}" ;;
    stable-number ) printf "0x%02d%02d%02d00\n" "${stable_major}" "${stable_minor}" "${stable_patch_digits}" ;;
    # devel
    devel ) echo "${weechat_devel}" | cut -d"-" -f1 ;;
    devel-full ) echo "${weechat_devel}" ;;
    devel-major ) echo "${devel_major}" ;;
    devel-minor ) echo "${devel_minor}" ;;
    devel-patch ) echo "${devel_patch}" ;;
    devel-number ) printf "0x%02d%02d%02d00\n" "${devel_major}" "${devel_minor}" "${devel_patch_digits}" ;;
    # error
    * ) echo >&2 "ERROR: unknown version."
        exit 1 ;;
esac
