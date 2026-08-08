#!/bin/sh
#
# SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Check shell and Python scripts in WeeChat git repository using these tools:
#  - shell scripts: shellcheck
#  - Python scripts: ruff

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
    ruff check "${root_dir}/$script"
done
