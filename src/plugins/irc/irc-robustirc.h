#ifndef WEECHAT_IRC_ROBUSTIRC_H
#define WEECHAT_IRC_ROBUSTIRC_H 1

#include "../../core/wee-hook.h"

extern int irc_robustirc_init (void);
extern int irc_robustirc_send (struct t_irc_server *server, const char *buffer, int size_buf);
extern void irc_robustirc_connect (struct t_irc_server *server, const char *address, t_hook_callback_connect_robustirc *callback, void *callback_data);

#endif /* WEECHAT_IRC_ROBUSTIRC_H */
