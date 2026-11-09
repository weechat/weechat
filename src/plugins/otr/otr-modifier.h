/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_MODIFIER_H
#define WEECHAT_PLUGIN_OTR_MODIFIER_H

extern char *otr_modifier_in_cb (const void *pointer, void *data,
                                 const char *modifier,
                                 const char *modifier_data,
                                 const char *string);
extern char *otr_modifier_out_cb (const void *pointer, void *data,
                                  const char *modifier,
                                  const char *modifier_data,
                                  const char *string);

#endif /* WEECHAT_PLUGIN_OTR_MODIFIER_H */
