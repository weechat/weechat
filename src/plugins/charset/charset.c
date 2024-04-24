/*
 * charset.c - charset plugin for WeeChat: encode/decode strings
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

#include <stdio.h>
#include <stdlib.h>
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <string.h>
#include <iconv.h>

#include "../weechat-plugin.h"
#include "charset.h"


WEECHAT_PLUGIN_NAME(CHARSET_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Charset conversions"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(CHARSET_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_charset_plugin = NULL;

struct t_config_file *charset_config_file = NULL;
struct t_config_option *charset_default_decode = NULL;
struct t_config_option *charset_default_encode = NULL;
struct t_config_section *charset_config_section_decode = NULL;
struct t_config_section *charset_config_section_encode = NULL;

char *charset_terminal = NULL;
char *charset_internal = NULL;


/*
 * Reloads charset configuration file.
 */

int
charset_config_reload (const void *pointer, void *data,
                       struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* free all decode/encode charsets */
    weechat_config_section_free_options (charset_config_section_decode);
    weechat_config_section_free_options (charset_config_section_encode);

    return weechat_config_reload (config_file);
}

/*
 * Checks if a decoding charset is allowed (different from "UTF-8", which is the
 * internal charset).
 *
 * Returns:
 *   1: charset is allowed
 *   0: charset not allowed
 */

int
charset_decode_is_allowed (const char *charset)
{
    if (weechat_strcasestr (charset, "utf-8")
        || weechat_strcasestr (charset, "utf8"))
    {
        weechat_printf (NULL,
                        _("%s%s: UTF-8 is not allowed in charset decoding "
                          "options (it is internal and default charset: decode "
                          "of UTF-8 is OK even if you specify another charset "
                          "to decode)"),
                        weechat_prefix ("error"), CHARSET_PLUGIN_NAME);
        return 0;
    }

    return 1;
}

/*
 * Checks the validity of a decoding charset.
 *
 * Returns:
 *   1: valid
 *   0: invalid
 */

int
charset_check_charset_decode_cb (const void *pointer, void *data,
                                 struct t_config_option *option,
                                 const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    return charset_decode_is_allowed (value);
}

/*
 * Sets a charset.
 */

int
charset_config_create_option (const void *pointer, void *data,
                              struct t_config_file *config_file,
                              struct t_config_section *section,
                              const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = weechat_config_option_set (ptr_option, value, 1);
            else
            {
                weechat_config_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                if ((section != charset_config_section_decode)
                    || charset_decode_is_allowed (value))
                {
                    ptr_option = weechat_config_new_option (
                        config_file, section,
                        option_name, "string", NULL,
                        NULL, 0, 0, "", value, 0,
                        (section == charset_config_section_decode) ? &charset_check_charset_decode_cb : NULL, NULL, NULL,
                        NULL, NULL, NULL,
                        NULL, NULL, NULL);
                    rc = (ptr_option) ?
                        WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
                }
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating charset \"%s\" => \"%s\""),
                        weechat_prefix ("error"), CHARSET_PLUGIN_NAME,
                        option_name, value);
    }

    return rc;
}

/*
 * Initializes charset configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
charset_config_init ()
{
    struct t_config_section *ptr_section;

    charset_config_file = weechat_config_new (
        CHARSET_CONFIG_PRIO_NAME,
        &charset_config_reload, NULL, NULL);
    if (!charset_config_file)
        return 0;

    ptr_section = weechat_config_new_section (charset_config_file, "default",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (charset_config_file);
        charset_config_file = NULL;
        return 0;
    }

    charset_default_decode = weechat_config_new_option (
        charset_config_file, ptr_section,
        "decode", "string",
        N_("global decoding charset: charset used to decode incoming messages "
           "when they are not UTF-8 valid"),
        NULL, 0, 0,
        (charset_terminal && charset_internal
         && (weechat_strcasecmp (charset_terminal,
                                 charset_internal) != 0)) ?
        charset_terminal : "iso-8859-1", NULL, 0,
        &charset_check_charset_decode_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    charset_default_encode = weechat_config_new_option (
        charset_config_file, ptr_section,
        "encode", "string",
        N_("global encoding charset: charset used to encode outgoing messages "
           "(if empty, default is UTF-8 because it is the WeeChat internal "
           "charset)"),
        NULL, 0, 0, "", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    ptr_section = weechat_config_new_section (
        charset_config_file, "decode",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &charset_config_create_option, NULL, NULL,
        NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (charset_config_file);
        charset_config_file = NULL;
        return 0;
    }

    charset_config_section_decode = ptr_section;

    ptr_section = weechat_config_new_section (
        charset_config_file, "encode",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &charset_config_create_option, NULL, NULL,
        NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (charset_config_file);
        charset_config_file = NULL;
        return 0;
    }

    charset_config_section_encode = ptr_section;

    return 1;
}

/*
 * Reads charset configuration file.
 */

int
charset_config_read ()
{
    return weechat_config_read (charset_config_file);
}

/*
 * Writes charset configuration file.
 */

int
charset_config_write ()
{
    return weechat_config_write (charset_config_file);
}

/*
 * Checks if a charset is valid.
 *
 * Returns:
 *   1: charset is valid
 *   0: charset is not valid
 */

int
charset_check (const char *charset)
{
    iconv_t cd;

    if (!charset || !charset[0])
        return 0;

    cd = iconv_open (charset, charset_internal);
    if (cd == (iconv_t)(-1))
        return 0;

    iconv_close (cd);
    return 1;
}

/*
 * Reads a charset in configuration file.
 *
 * First tries with all arguments, then removes one by one to find charset (from
 * specific to general charset).
 */

const char *
charset_get (struct t_config_section *section, const char *name,
             struct t_config_option *default_charset)
{
    char *option_name, *ptr_end;
    struct t_config_option *ptr_option;

    option_name = strdup (name);
    if (option_name)
    {
        ptr_end = option_name + strlen (option_name);
        while (ptr_end >= option_name)
        {
            ptr_option = weechat_config_search_option (charset_config_file,
                                                       section,
                                                       option_name);
            if (ptr_option)
            {
                free (option_name);
                return weechat_config_string (ptr_option);
            }
            ptr_end--;
            while ((ptr_end >= option_name) && (ptr_end[0] != '.'))
            {
                ptr_end--;
            }
            if ((ptr_end >= option_name) && (ptr_end[0] == '.'))
                ptr_end[0] = '\0';
        }
        ptr_option = weechat_config_search_option (charset_config_file,
                                                   section,
                                                   option_name);

        free (option_name);

        if (ptr_option)
            return weechat_config_string (ptr_option);
    }

    /* nothing found => return default decode/encode charset (if set) */
    if (weechat_config_string (default_charset)
        && weechat_config_string (default_charset)[0])
        return weechat_config_string (default_charset);

    /* no default charset set */
    return NULL;
}

/*
 * Decodes a string with a charset to internal charset (UTF-8).
 */

char *
charset_decode_cb (const void *pointer, void *data,
                   const char *modifier, const char *modifier_data,
                   const char *string)
{
    const char *charset;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;

    charset = charset_get (charset_config_section_decode, modifier_data,
                           charset_default_decode);
    if (weechat_charset_plugin->debug)
    {
        weechat_printf (NULL,
                        "charset: debug: using 'decode' charset: %s "
                        "(modifier=\"%s\", modifier_data=\"%s\", string=\"%s\")",
                        charset, modifier, modifier_data, string);
    }
    if (charset && charset[0])
        return weechat_iconv_to_internal (charset, string);

    return NULL;
}

/*
 * Encodes a string from internal charset (UTF-8) to another charset.
 */

char *
charset_encode_cb (const void *pointer, void *data,
                   const char *modifier, const char *modifier_data,
                   const char *string)
{
    const char *charset;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;

    charset = charset_get (charset_config_section_encode, modifier_data,
                           charset_default_encode);
    if (weechat_charset_plugin->debug)
    {
        weechat_printf (NULL,
                        "charset: debug: using 'encode' charset: %s "
                        "(modifier=\"%s\", modifier_data=\"%s\", string=\"%s\")",
                        charset, modifier, modifier_data, string);
    }
    if (charset && charset[0])
        return weechat_iconv_from_internal (charset, string);

    return NULL;
}

/*
 * Sets a charset.
 */

void
charset_set (struct t_config_section *section, const char *type,
             const char *name, const char *value)
{
    if (charset_config_create_option (NULL, NULL,
                                      charset_config_file,
                                      section,
                                      name,
                                      value) > 0)
    {
        if (value && value[0])
            weechat_printf (NULL, "%s: %s, \"%s\" => %s",
                            CHARSET_PLUGIN_NAME, type, name, value);
        else
            weechat_printf (NULL, _("%s: %s, \"%s\": removed"),
                            CHARSET_PLUGIN_NAME, type, name);
    }
}

/*
 * Displays terminal and internal charsets.
 */

void
charset_display_charsets ()
{
    weechat_printf (NULL,
                    _("%s: terminal: %s, internal: %s"),
                    CHARSET_PLUGIN_NAME, charset_terminal, charset_internal);
}

/*
 * Callback for command "/charset".
 */

int
charset_command_cb (const void *pointer, void *data,
                    struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    struct t_config_section *ptr_section;
    int length;
    char *ptr_charset, *option_name;
    const char *plugin_name, *name, *charset_modifier;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc < 2)
    {
        charset_display_charsets ();
        return WEECHAT_RC_OK;
    }

    ptr_section = NULL;

    plugin_name = weechat_buffer_get_string (buffer, "plugin");
    name = weechat_buffer_get_string (buffer, "name");

    charset_modifier = weechat_buffer_get_string (buffer,
                                                  "localvar_charset_modifier");
    if (charset_modifier)
        option_name = strdup (charset_modifier);
    else
    {
        length = strlen (plugin_name) + 1 + strlen (name) + 1;
        option_name = malloc (length);
        if (!option_name)
            WEECHAT_COMMAND_ERROR;

        snprintf (option_name, length, "%s.%s", plugin_name, name);
    }

    if (weechat_strcmp (argv[1], "reset") == 0)
    {
        charset_set (charset_config_section_decode, "decode", option_name,
                     NULL);
        charset_set (charset_config_section_encode, "encode", option_name,
                     NULL);
    }
    else
    {
        if (argc > 2)
        {
            if (weechat_strcmp (argv[1], "decode") == 0)
            {
                ptr_section = charset_config_section_decode;
                ptr_charset = argv_eol[2];
            }
            else if (weechat_strcmp (argv[1], "encode") == 0)
            {
                ptr_section = charset_config_section_encode;
                ptr_charset = argv_eol[2];
            }
            if (!ptr_section)
            {
                weechat_printf (NULL,
                                _("%s%s: wrong charset type (decode or encode "
                                  "expected)"),
                                weechat_prefix ("error"), CHARSET_PLUGIN_NAME);
                free (option_name);
                return WEECHAT_RC_OK;
            }
        }
        else
            ptr_charset = argv_eol[1];

        if (!charset_check (ptr_charset))
        {
            weechat_printf (NULL,
                            _("%s%s: invalid charset: \"%s\""),
                            weechat_prefix ("error"), CHARSET_PLUGIN_NAME,
                            ptr_charset);
            free (option_name);
            return WEECHAT_RC_OK;
        }
        if (ptr_section)
        {
            charset_set (ptr_section, argv[1], option_name, ptr_charset);
        }
        else
        {
            charset_set (charset_config_section_decode, "decode", option_name,
                         ptr_charset);
            charset_set (charset_config_section_encode, "encode", option_name,
                         ptr_charset);
        }
    }

    free (option_name);

    return WEECHAT_RC_OK;
}

/*
 * Initializes charset plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    /* get terminal & internal charsets */
    charset_terminal = weechat_info_get ("charset_terminal", "");
    charset_internal = weechat_info_get ("charset_internal", "");

    /* display message */
    if (weechat_charset_plugin->debug >= 1)
        charset_display_charsets ();

    if (!charset_config_init ())
        return WEECHAT_RC_ERROR;

    charset_config_read ();

    /* /charset command */
    weechat_hook_command (
        "charset",
        N_("change charset for current buffer"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("decode|encode <charset>"
           " || reset"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[decode]: change decoding charset"),
            N_("raw[encode]: change encoding charset"),
            N_("charset: new charset for current buffer"),
            N_("raw[reset]: reset charsets for current buffer")),
        "decode|encode|reset",
        &charset_command_cb, NULL, NULL);

    /* modifiers hooks */
    weechat_hook_modifier ("charset_decode", &charset_decode_cb, NULL, NULL);
    weechat_hook_modifier ("charset_encode", &charset_encode_cb, NULL, NULL);

    return WEECHAT_RC_OK;
}

/*
 * Ends charset plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    charset_config_write ();

    weechat_config_free (charset_config_file);
    charset_config_file = NULL;

    free (charset_terminal);
    charset_terminal = NULL;

    free (charset_internal);
    charset_internal = NULL;

    return WEECHAT_RC_OK;
}
