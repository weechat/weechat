#!/bin/sh
#
# SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Run the installed WeeChat with a few command line options to check that
# it starts and quits properly.
#
# Syntax:
#   ./run_weechat_smoke.sh
#
# This script is used to run WeeChat in CI environment, after the install.

set -o errexit

# Display commands.
set -x

weechat --help
weechat-curses --help
weechat --version
weechat --build-info
weechat --colors
weechat --license
weechat --run-command "/debug dirs;/debug libs" --run-command "/quit"
