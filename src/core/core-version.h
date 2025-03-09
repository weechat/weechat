/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_VERSION_H
#define WEECHAT_VERSION_H

extern const char *version_get_name (void);
extern const char *version_get_version (void);
extern const char *version_get_name_version (void);
extern const char *version_get_git (void);
extern const char *version_get_version_with_git (void);
extern const char *version_get_compilation_date (void);
extern const char *version_get_compilation_time (void);
extern const char *version_get_compilation_date_time (void);

#endif /* WEECHAT_VERSION_H */
