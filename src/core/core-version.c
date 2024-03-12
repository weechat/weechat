/*
 * core-version.c - functions for WeeChat version
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "config-git.h"

#include <stdio.h>


/*
 * Returns package name ("weechat").
 */

const char *
version_get_name ()
{
    return PACKAGE_NAME;
}

/*
 * Returns the WeeChat version.
 *
 * Examples:
 *   0.3.9-dev
 *   0.3.9-rc1
 *   0.3.9
 */

const char *
version_get_version ()
{
    return PACKAGE_VERSION;
}

/*
 * Returns the package name ("weechat") + WeeChat version.
 *
 * Examples:
 *   weechat 0.3.9-dev
 *   weechat 0.3.9-rc1
 *   weechat 0.3.9
 */

const char *
version_get_name_version ()
{
    return PACKAGE_STRING;
}

/*
 * Returns the output of "git describe" (non-empty only for a devel version,
 * if compilation was made using the git repository, if git command was found).
 *
 * Example:
 *   v0.3.9-104-g7eb5cc
 */

const char *
version_get_git ()
{
    return PACKAGE_VERSION_GIT;
}

/*
 * Returns the WeeChat version + the git version (between brackets, and only if
 * it is not empty).
 *
 * Examples:
 *   0.3.9-dev (git: v0.3.9-104-g7eb5cc)
 *   0.3.9-dev
 *   0.3.9-rc1 (git: v0.3.9-rc1)
 *   0.3.9
 */

const char *
version_get_version_with_git ()
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
 * Returns date of WeeChat compilation.
 *
 * Example:
 *   Dec 16 2012
 */

const char *
version_get_compilation_date ()
{
    return __DATE__;
}

/*
 * Returns time of WeeChat compilation.
 *
 * Example:
 *   18:10:22
 */

const char *
version_get_compilation_time ()
{
    return __TIME__;
}

/*
 * Returns date/time of WeeChat compilation.
 *
 * Example:
 *   Dec 16 2012 18:10:22
 */

const char *
version_get_compilation_date_time ()
{
    return __DATE__ " " __TIME__;
}
