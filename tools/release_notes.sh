#!/bin/sh
#
# SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Display the release notes of a WeeChat version, which is the content of the
# section for this version in file CHANGELOG.md.
#
# Syntax:  release_notes.sh <version>
#
#   version   WeeChat version, with or without the leading "v",
#             for example: v4.10.0, 4.10.0 or Unreleased

set -o errexit

if [ $# -lt 1 ]; then
    echo >&2 "usage: $0 <version>"
    exit 1
fi

# Remove the leading "v" of tag, to get the version as written in the changelog.
version=${1#v}

# Move to the repository root.
cd "$(dirname "$0")/.."

changelog="CHANGELOG.md"

# Display the section of the version, without the heading and the blank lines
# at the beginning and the end: it stops on the next version heading or on the
# block of links at the end of file.
awk -v heading="## [${version}]" '
    index($0, heading) == 1 { found = 1; next }
    found && (/^## / || /^\[[^]]+\]: /) { exit }
    found && $0 == "" { if (started) blank = blank "\n"; next }
    found { printf "%s%s\n", blank, $0; blank = ""; started = 1 }
    END { if (!found) exit 1 }
' "${changelog}" || {
    echo >&2 "ERROR: version \"${version}\" not found in ${changelog}"
    exit 1
}
