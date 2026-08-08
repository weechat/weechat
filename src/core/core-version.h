/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
