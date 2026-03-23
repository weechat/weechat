/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Functions for WeeChat version */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "config-git.h"

#include <stdio.h>


/*
 * Return package name ("weechat").
 */

const char *
version_get_name (void)
{
    return PACKAGE_NAME;
}

/*
 * Return the WeeChat version.
 *
 * Examples:
 *   0.3.9-dev
 *   0.3.9-rc1
 *   0.3.9
 */

const char *
version_get_version (void)
{
    return PACKAGE_VERSION;
}

/*
 * Return the package name ("weechat") + WeeChat version.
 *
 * Examples:
 *   weechat 0.3.9-dev
 *   weechat 0.3.9-rc1
 *   weechat 0.3.9
 */

const char *
version_get_name_version (void)
{
    return PACKAGE_STRING;
}

/*
 * Return the output of "git describe" (non-empty only for a devel version,
 * if compilation was made using the git repository, if git command was found).
 *
 * Example:
 *   v0.3.9-104-g7eb5cc
 */

const char *
version_get_git (void)
{
    return PACKAGE_VERSION_GIT;
}

/*
 * Return the WeeChat version + the git version (between brackets, and only if
 * it is not empty).
 *
 * Examples:
 *   0.3.9-dev (git: v0.3.9-104-g7eb5cc)
 *   0.3.9-dev
 *   0.3.9-rc1 (git: v0.3.9-rc1)
 *   0.3.9
 */

const char *
version_get_version_with_git (void)
{
    const char *git_version;
    static char version[256];

    git_version = version_get_git ();

    snprintf (version, sizeof (version), "%s%s%s%s",
              version_get_version (),
              (git_version && git_version[0]) ? " (git: " : "",
              (git_version && git_version[0]) ? git_version : "",
              (git_version && git_version[0]) ? ")" : "");

    return version;
}

/*
 * Return date of WeeChat compilation.
 *
 * Example:
 *   Dec 16 2012
 */

const char *
version_get_compilation_date (void)
{
    return __DATE__;
}

/*
 * Return time of WeeChat compilation.
 *
 * Example:
 *   18:10:22
 */

const char *
version_get_compilation_time (void)
{
    return __TIME__;
}

/*
 * Return date/time of WeeChat compilation.
 *
 * Example:
 *   Dec 16 2012 18:10:22
 */

const char *
version_get_compilation_date_time (void)
{
    return __DATE__ " " __TIME__;
}
