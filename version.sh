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
#   version.sh stable|devel|devel-full|devel-major|devel-minor|devel-patch
#
#     stable       the current stable (e.g. 1.3)
#     devel        the current devel (e.g. 1.4)
#     devel-full   the full name of devel (e.g. 1.4-dev or 1.4-rc1)
#     devel-major  the major version of devel (e.g. 1)
#     devel-minor  the minor version of devel (e.g. 4-dev)
#     devel-patch  the patch version of devel (e.g. 2 for version 1.4.2)
#

WEECHAT_STABLE=3.8
WEECHAT_DEVEL=3.8
WEECHAT_DEVEL_FULL=3.8

if [ $# -lt 1 ]; then
    echo >&2 "Syntax: $0 stable|devel|devel-full|devel-major|devel-minor|devel-patch"
    exit 1
fi

case $1 in
    stable ) echo "$WEECHAT_STABLE" ;;
    devel ) echo "$WEECHAT_DEVEL" ;;
    devel-full ) echo "$WEECHAT_DEVEL_FULL" ;;
    devel-major ) echo "$WEECHAT_DEVEL_FULL" | cut -d'.' -f1 ;;
    devel-minor ) echo "$WEECHAT_DEVEL_FULL" | cut -d'.' -f2 ;;
    devel-patch ) echo "$WEECHAT_DEVEL_FULL" | cut -d'.' -f3- ;;
    * ) echo >&2 "ERROR: unknown version."
        exit 1 ;;
esac
