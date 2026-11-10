/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_CALLBACK_H
#define WEECHAT_PLUGIN_OTR_CALLBACK_H

/* Data given to libotr functions (opdata), received by the callbacks. */

struct t_otr_callback_data
{
    char *unencrypted;                 /* plaintext received while session  */
                                       /* is encrypted (ignored by libotr)  */
    char *last_injected;               /* last OTR message sent by libotr   */
};

extern const OtrlMessageAppOps otr_callback_ops;

#endif /* WEECHAT_PLUGIN_OTR_CALLBACK_H */
