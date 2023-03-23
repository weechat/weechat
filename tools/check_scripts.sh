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
# Check shell and Python scripts in WeeChat git repository using these tools:
#  - shell scripts: shellcheck
#  - Python scripts: flake8 + pylint + bandit
#

set -o errexit

# check git repository
ROOT_DIR=$(git rev-parse --show-toplevel)
cd "${ROOT_DIR}"

SHELL_SCRIPTS=$(git ls-files "*.sh")
PYTHON_SCRIPTS=$(git ls-files "*.py")

# display commands
set -x

# check shell scripts
for script in $SHELL_SCRIPTS; do
    shellcheck "${ROOT_DIR}/$script"
done

# check Python scripts
for script in $PYTHON_SCRIPTS; do
    flake8 --max-line-length=100 --builtins=_ "${ROOT_DIR}/$script"
    pylint --additional-builtins=_ "${ROOT_DIR}/$script"
    bandit "${ROOT_DIR}/$script"
done
