/*
 * wee-version.c - functions for WeeChat version
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "config-git.h"


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
 * if compilation was made* using the git repository, if git command was found).
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
