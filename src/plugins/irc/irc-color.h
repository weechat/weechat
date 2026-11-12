/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_COLOR_H
#define WEECHAT_PLUGIN_IRC_COLOR_H

#define IRC_NUM_COLORS        100
#define IRC_COLOR_DEFAULT     99   /* default color (fg or bg)         */

/*
 * Shift ncurses colors for compatibility with colors
 * in IRC messages (same as other IRC clients).
 */

#define WEECHAT_COLOR_BLACK   COLOR_BLACK
#define WEECHAT_COLOR_RED     COLOR_BLUE
#define WEECHAT_COLOR_GREEN   COLOR_GREEN
#define WEECHAT_COLOR_YELLOW  COLOR_CYAN
#define WEECHAT_COLOR_BLUE    COLOR_RED
#define WEECHAT_COLOR_MAGENTA COLOR_MAGENTA
#define WEECHAT_COLOR_CYAN    COLOR_YELLOW
#define WEECHAT_COLOR_WHITE   COLOR_WHITE

/* Attributes in IRC messages for color & style (bold, ..) */

#define IRC_COLOR_BOLD_CHAR          '\x02'  /* bold text                   */
#define IRC_COLOR_BOLD_STR           "\x02"  /*   [02]...[02]               */

#define IRC_COLOR_COLOR_CHAR         '\x03'  /* text color: fg / fg,bg      */
#define IRC_COLOR_COLOR_STR          "\x03"  /*   [03]15,05...[03]          */

#define IRC_COLOR_HEX_COLOR_CHAR     '\x04'  /* hex text col.: fg / fg,bg   */
#define IRC_COLOR_HEX_COLOR_STR      "\x04"  /*   [04]FFFF00,8B008B...[04]  */

#define IRC_COLOR_RESET_CHAR         '\x0F'  /* reset color/attributes      */
#define IRC_COLOR_RESET_STR          "\x0F"  /*   [0F]...                   */

#define IRC_COLOR_REVERSE_CHAR       '\x16'  /* reverse video (fg <--> bg)  */
#define IRC_COLOR_REVERSE_STR        "\x16"  /*   [16]...[16]               */

#define IRC_COLOR_ITALIC_CHAR        '\x1D'  /* italic text                 */
#define IRC_COLOR_ITALIC_STR         "\x1D"  /*   [1D]...[1D]               */

#define IRC_COLOR_STRIKETHROUGH_CHAR '\x1E'  /* strikethrough text          */
#define IRC_COLOR_STRIKETHROUGH_STR  "\x1E"  /*   [1E]...[1E]               */

#define IRC_COLOR_UNDERLINE_CHAR     '\x1F'  /* underlined text             */
#define IRC_COLOR_UNDERLINE_STR      "\x1F"  /*   [1F]...[1F]               */

#define IRC_COLOR_MONOSPACE_CHAR     '\x11'  /* monospace text              */
#define IRC_COLOR_MONOSPACE_STR      "\x11"  /*   [11]...[11]               */

#define IRC_COLOR_TERM2IRC_NUM_COLORS 16

/* Flag for a RGB color (IRC hex color "^Drrggbb") in an IRC formatting */
#define IRC_COLOR_RGB_FLAG           0x1000000

/* Colors of IRC formatting which are not a color */
#define IRC_COLOR_FORMATTING_NONE    -1 /* no color code                    */

/* Macros for WeeChat core and IRC colors */

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

#define IRC_COLOR_MSG(__string)                                         \
    irc_color_decode_const (                                            \
        __string,                                                       \
        weechat_config_boolean (irc_config_network_colors_receive))

/* Formatting (attributes and colors) at a position in an IRC message */
struct t_irc_color_formatting
{
    char bold;
    char reverse;
    char italic;
    char strikethrough;
    char underline;
    char monospace;
    int fg;                            /* IRC color (0-99), RGB color with  */
                                       /* flag IRC_COLOR_RGB_FLAG, or       */
                                       /* IRC_COLOR_FORMATTING_NONE         */
    int bg;                            /* background (same values as fg)    */
};

/* State of ANSI colors converted to IRC colors */
struct t_irc_color_ansi_state
{
    struct t_irc_color_formatting formatting; /* IRC formatting sent        */
};

extern int irc_color_is_hex_color (const unsigned char *string);
extern int irc_color_hex_color_value (const unsigned char *string);
extern int irc_color_code_parse (const char *string, int *fg, int *bg);
extern int irc_color_char_size (const char *string);
extern int irc_color_code_size (const char *string);
extern void irc_color_formatting_init (struct t_irc_color_formatting *formatting);
extern void irc_color_formatting_set_colors (int *fg, int *bg, int code_fg,
                                             int code_bg, int reset);
extern void irc_color_formatting_update (struct t_irc_color_formatting *formatting,
                                         const char *string, int length);
extern void irc_color_formatting_add_color (char **output, int color);
extern void irc_color_formatting_add_code (char **output, int fg, int bg);
extern int irc_color_formatting_transition (char **output,
                                            const struct t_irc_color_formatting *from,
                                            const struct t_irc_color_formatting *to,
                                            int colors_only);
extern void irc_color_formatting_add_separator (char **output, int fg_only,
                                                const char *text, int length);
extern char *irc_color_formatting_codes (struct t_irc_color_formatting *formatting,
                                         const char *next_text,
                                         int attributes_only);
extern char *irc_color_decode (const char *string, int keep_colors);
extern const char *irc_color_decode_const (const char *string, int keep_colors);
extern char *irc_color_encode (const char *string, int keep_colors);
extern char *irc_color_decode_ansi (const char *string, int keep_colors);
extern char *irc_color_modifier_cb (const void *pointer, void *data,
                                    const char *modifier,
                                    const char *modifier_data,
                                    const char *string);
extern char *irc_color_for_tags (const char *color);
extern int irc_color_weechat_add_to_infolist (struct t_infolist *infolist);
extern void irc_color_init (void);
extern void irc_color_end (void);

#endif /* WEECHAT_PLUGIN_IRC_COLOR_H */
