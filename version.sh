#!/bin/sh
#
# Copyright (C) 2015-2023 Sébastien Helleu <flashcode@flashtux.org>
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

WEECHAT_STABLE="4.0.1"
WEECHAT_DEVEL="4.0.1"

STABLE_MAJOR=$(echo "${WEECHAT_STABLE}" | cut -d"." -f1)
STABLE_MINOR=$(echo "${WEECHAT_STABLE}" | cut -d"." -f2)
STABLE_PATCH=$(echo "${WEECHAT_STABLE}" | cut -d"." -f3-)
STABLE_PATCH_DIGITS=$(echo "${WEECHAT_STABLE}" | cut -d"." -f3- | cut -d"-" -f1)

DEVEL_MAJOR=$(echo "${WEECHAT_DEVEL}" | cut -d"." -f1)
DEVEL_MINOR=$(echo "${WEECHAT_DEVEL}" | cut -d"." -f2)
DEVEL_PATCH=$(echo "${WEECHAT_DEVEL}" | cut -d"." -f3-)
DEVEL_PATCH_DIGITS=$(echo "${WEECHAT_DEVEL}" | cut -d"." -f3- | cut -d"-" -f1)

if [ $# -lt 1 ]; then
    echo >&2 "Syntax: $0 <name>"
    echo >&2 "name: stable, stable-major, stable-minor, stable-patch, stable-number,"
    echo >&2 "      devel, devel-full, devel-major, devel-minor, devel-patch, devel-number"
    exit 1
fi

case $1 in
    # stable
    stable ) echo "${WEECHAT_STABLE}" ;;
    stable-major ) echo "${STABLE_MAJOR}" ;;
    stable-minor ) echo "${STABLE_MINOR}" ;;
    stable-patch ) echo "${STABLE_PATCH}" ;;
    stable-number ) printf "0x%02d%02d%02d00\n" "${STABLE_MAJOR}" "${STABLE_MINOR}" "${STABLE_PATCH_DIGITS}" ;;
    # devel
    devel ) echo "${WEECHAT_DEVEL}" | cut -d"-" -f1 ;;
    devel-full ) echo "${WEECHAT_DEVEL}" ;;
    devel-major ) echo "${DEVEL_MAJOR}" ;;
    devel-minor ) echo "${DEVEL_MINOR}" ;;
    devel-patch ) echo "${DEVEL_PATCH}" ;;
    devel-number ) printf "0x%02d%02d%02d00\n" "${DEVEL_MAJOR}" "${DEVEL_MINOR}" "${DEVEL_PATCH_DIGITS}" ;;
    # error
    * ) echo >&2 "ERROR: unknown version."
        exit 1 ;;
esac
