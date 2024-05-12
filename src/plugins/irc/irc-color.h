/*
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

#ifndef WEECHAT_PLUGIN_IRC_COLOR_H
#define WEECHAT_PLUGIN_IRC_COLOR_H

#define IRC_NUM_COLORS        100

/*
 * shift ncurses colors for compatibility with colors
 * in IRC messages (same as other IRC clients)
 */

#define WEECHAT_COLOR_BLACK   COLOR_BLACK
#define WEECHAT_COLOR_RED     COLOR_BLUE
#define WEECHAT_COLOR_GREEN   COLOR_GREEN
#define WEECHAT_COLOR_YELLOW  COLOR_CYAN
#define WEECHAT_COLOR_BLUE    COLOR_RED
#define WEECHAT_COLOR_MAGENTA COLOR_MAGENTA
#define WEECHAT_COLOR_CYAN    COLOR_YELLOW
#define WEECHAT_COLOR_WHITE   COLOR_WHITE

/* attributes in IRC messages for color & style (bold, ..) */

#define IRC_COLOR_BOLD_CHAR      '\x02'  /* bold text                       */
#define IRC_COLOR_BOLD_STR       "\x02"  /*   [02]...[02]                   */

#define IRC_COLOR_COLOR_CHAR     '\x03'  /* text color: fg/fg,bg/,bg        */
#define IRC_COLOR_COLOR_STR      "\x03"  /*   [03]15,05...[03]              */

#define IRC_COLOR_HEX_COLOR_CHAR '\x04'  /* text color (hex): fg/fg,bg/,bg  */
#define IRC_COLOR_HEX_COLOR_STR  "\x04"  /*   [04]FFFF00,8B008B...[04]      */

#define IRC_COLOR_RESET_CHAR     '\x0F'  /* reset color/attributes          */
#define IRC_COLOR_RESET_STR      "\x0F"  /*   [0F]...                       */

#define IRC_COLOR_REVERSE_CHAR   '\x16'  /* reverse video (fg <--> bg)      */
#define IRC_COLOR_REVERSE_STR    "\x16"  /*   [16]...[16]                   */

#define IRC_COLOR_ITALIC_CHAR    '\x1D'  /* italic text                     */
#define IRC_COLOR_ITALIC_STR     "\x1D"  /*   [1D]...[1D]                   */

#define IRC_COLOR_UNDERLINE_CHAR '\x1F'  /* underlined text                 */
#define IRC_COLOR_UNDERLINE_STR  "\x1F"  /*   [1F]...[1F]                   */

#define IRC_COLOR_TERM2IRC_NUM_COLORS 16

/* macros for WeeChat core and IRC colors */

#define IRC_COLOR_BAR_FG weechat_color("bar_fg")
#define IRC_COLOR_BAR_BG weechat_color("bar_bg")
#define IRC_COLOR_BAR_DELIM weechat_color("bar_delim")
#define IRC_COLOR_RESET weechat_color("reset")
#define IRC_COLOR_CHAT_CHANNEL weechat_color("chat_channel")
#define IRC_COLOR_CHAT_DELIMITERS weechat_color("chat_delimiters")
#define IRC_COLOR_CHAT_HOST weechat_color("chat_host")
#define IRC_COLOR_CHAT_NICK weechat_color("chat_nick")
#define IRC_COLOR_CHAT_NICK_SELF weechat_color("chat_nick_self")
#define IRC_COLOR_CHAT_NICK_OTHER weechat_color("chat_nick_other")
#define IRC_COLOR_CHAT_SERVER weechat_color("chat_server")
#define IRC_COLOR_CHAT_VALUE weechat_color("chat_value")
#define IRC_COLOR_NICK_PREFIX_OP weechat_color(weechat_config_string(irc_config_color_nick_prefix_op))
#define IRC_COLOR_NICK_PREFIX_HALFOP weechat_color(weechat_config_string(irc_config_color_nick_prefix_halfop))
#define IRC_COLOR_NICK_PREFIX_VOICE weechat_color(weechat_config_string(irc_config_color_nick_prefix_voice))
#define IRC_COLOR_NICK_PREFIX_USER weechat_color(weechat_config_string(irc_config_color_nick_prefix_user))
#define IRC_COLOR_NOTICE weechat_color(weechat_config_string(irc_config_color_notice))
#define IRC_COLOR_STATUS_NUMBER weechat_color("status_number")
#define IRC_COLOR_STATUS_NAME weechat_color("status_name")
#define IRC_COLOR_STATUS_NAME_INSECURE weechat_color("status_name_insecure")
#define IRC_COLOR_STATUS_NAME_TLS weechat_color("status_name_tls")
#define IRC_COLOR_MESSAGE_JOIN weechat_color(weechat_config_string(irc_config_color_message_join))
#define IRC_COLOR_MESSAGE_ACCOUNT weechat_color(weechat_config_string(irc_config_color_message_account))
#define IRC_COLOR_MESSAGE_CHGHOST weechat_color(weechat_config_string(irc_config_color_message_chghost))
#define IRC_COLOR_MESSAGE_KICK weechat_color(weechat_config_string(irc_config_color_message_kick))
#define IRC_COLOR_MESSAGE_QUIT weechat_color(weechat_config_string(irc_config_color_message_quit))
#define IRC_COLOR_MESSAGE_SETNAME weechat_color(weechat_config_string(irc_config_color_message_setname))
#define IRC_COLOR_REASON_KICK weechat_color(weechat_config_string(irc_config_color_reason_kick))
#define IRC_COLOR_REASON_QUIT weechat_color(weechat_config_string(irc_config_color_reason_quit))
#define IRC_COLOR_TOPIC_CURRENT weechat_color(weechat_config_string(irc_config_color_topic_current))
#define IRC_COLOR_TOPIC_OLD weechat_color(weechat_config_string(irc_config_color_topic_old))
#define IRC_COLOR_TOPIC_NEW weechat_color(weechat_config_string(irc_config_color_topic_new))
#define IRC_COLOR_INPUT_NICK weechat_color(weechat_config_string(irc_config_color_input_nick))
#define IRC_COLOR_ITEM_AWAY weechat_color(weechat_config_string(irc_config_color_item_away))
#define IRC_COLOR_ITEM_LAG_COUNTING weechat_color(weechat_config_string(irc_config_color_item_lag_counting))
#define IRC_COLOR_ITEM_LAG_FINISHED weechat_color(weechat_config_string(irc_config_color_item_lag_finished))
#define IRC_COLOR_ITEM_NICK_MODES weechat_color(weechat_config_string(irc_config_color_item_nick_modes))
#define IRC_COLOR_ITEM_TLS_VERSION_OK weechat_color(weechat_config_string(irc_config_color_item_tls_version_ok))
#define IRC_COLOR_ITEM_TLS_VERSION_DEPRECATED weechat_color(weechat_config_string(irc_config_color_item_tls_version_deprecated))
#define IRC_COLOR_ITEM_TLS_VERSION_INSECURE weechat_color(weechat_config_string(irc_config_color_item_tls_version_insecure))

struct t_irc_color_ansi_state
{
    char keep_colors;
    char bold;
    char underline;
    char italic;
};

extern char *irc_color_decode (const char *string, int keep_colors);
extern char *irc_color_encode (const char *string, int keep_colors);
extern char *irc_color_decode_ansi (const char *string, int keep_colors);
extern char *irc_color_modifier_cb (const void *pointer, void *data,
                                    const char *modifier,
                                    const char *modifier_data,
                                    const char *string);
extern char *irc_color_for_tags (const char *color);
extern int irc_color_weechat_add_to_infolist (struct t_infolist *infolist);
extern void irc_color_end ();

#endif /* WEECHAT_PLUGIN_IRC_COLOR_H */
