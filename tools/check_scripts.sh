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
# Check shell and Python scripts in WeeChat git repository using these tools:
#  - shell scripts: shellcheck
#  - Python scripts: flake8 + pylint + bandit
#

set -o errexit

# check git repository
root_dir=$(git rev-parse --show-toplevel)
cd "${root_dir}"

shell_scripts=$(git ls-files "*.sh")
python_scripts=$(git ls-files "*.py")

# display commands
set -x

# check shell scripts
for script in ${shell_scripts}; do
    shellcheck "${root_dir}/$script"
done

# check Python scripts
for script in ${python_scripts}; do
    flake8 --max-line-length=100 --builtins=_ "${root_dir}/$script"
    pylint --additional-builtins=_ "${root_dir}/$script"
    bandit "${root_dir}/$script"
done
