/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* charset.c: Charset plugin for WeeChat */


#include <stdio.h>
#include <stdlib.h>
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <string.h>
#include <iconv.h>

#include "../weechat-plugin.h"


WEECHAT_PLUGIN_NAME("charset");
WEECHAT_PLUGIN_DESCRIPTION("Charset plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

#define CHARSET_CONFIG_NAME "charset"

struct t_weechat_plugin *weechat_charset_plugin = NULL;
#define weechat_plugin weechat_charset_plugin

struct t_config_file *charset_config_file = NULL;
struct t_config_option *charset_default_decode = NULL;
struct t_config_option *charset_default_encode = NULL;
struct t_config_section *charset_config_section_decode = NULL;
struct t_config_section *charset_config_section_encode = NULL;

char *charset_terminal = NULL;
char *charset_internal = NULL;

int charset_debug = 0;


/*
 * charset_debug_cb: callback for "debug" signal
 */

int
charset_debug_cb (void *data, char *signal, char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (weechat_strcasecmp ((char *)signal_data, "charset") == 0)
        {
            charset_debug ^= 1;
            if (charset_debug)
                weechat_printf (NULL, _("%s: debug enabled"), "charset");
            else
                weechat_printf (NULL, _("%s: debug disabled"), "charset");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * charset_config_reaload: reload charset configuration file
 */

int
charset_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;
    
    /* free all decode/encode charsets */
    weechat_config_section_free_options (charset_config_section_decode);
    weechat_config_section_free_options (charset_config_section_encode);
    
    return weechat_config_reload (config_file);
}

/*
 * charset_config_set_option: set a charset
 */

int
charset_config_create_option (void *data, struct t_config_file *config_file,
                              struct t_config_section *section,
                              char *option_name, char *value)
{
    struct t_config_option *ptr_option;
    int rc;
    
    /* make C compiler happy */
    (void) data;
    
    rc = 0;
    
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
                weechat_config_option_free (section, ptr_option);
                rc = 1;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "string", NULL,
                    NULL, 0, 0, value, NULL, NULL, NULL, NULL, NULL, NULL);
                rc = (ptr_option) ? 1 : 0;
            }
            else
                rc = 1;
        }
    }
    
    if (rc == 0)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating charset \"%s\" => \"%s\""),
                        weechat_prefix ("error"), "charset",
                        option_name, value);
    }
    
    return rc;
}

/*
 * charset_config_init: init charset configuration file
 *                      return: 1 if ok, 0 if error
 */

int
charset_config_init ()
{
    struct t_config_section *ptr_section;
    
    charset_config_file = weechat_config_new (CHARSET_CONFIG_NAME,
                                              &charset_config_reload, NULL);
    if (!charset_config_file)
        return 0;
    
    ptr_section = weechat_config_new_section (charset_config_file, "default",
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (charset_config_file);
        return 0;
    }
    
    charset_default_decode = weechat_config_new_option (
        charset_config_file, ptr_section,
        "decode", "string",
        N_("global decoding charset"),
        NULL, 0, 0,
        (charset_terminal && charset_internal
         && (strcasecmp (charset_terminal,
                         charset_internal) != 0)) ?
        charset_terminal : "iso-8859-1",
        NULL, NULL, NULL, NULL, NULL, NULL);
    charset_default_encode = weechat_config_new_option (
        charset_config_file, ptr_section,
        "encode", "string",
        N_("global encoding charset"),
        NULL, 0, 0, "",
        NULL, NULL, NULL, NULL, NULL, NULL);
    
    ptr_section = weechat_config_new_section (charset_config_file, "decode",
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &charset_config_create_option, NULL);
    if (!ptr_section)
    {
        weechat_config_free (charset_config_file);
        return 0;
    }
    
    charset_config_section_decode = ptr_section;
    
    ptr_section = weechat_config_new_section (charset_config_file, "encode",
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &charset_config_create_option, NULL);
    if (!ptr_section)
    {
        weechat_config_free (charset_config_file);
        return 0;
    }
    
    charset_config_section_encode = ptr_section;
    
    return 1;
}

/*
 * charset_config_read: read charset configuration file
 */

int
charset_config_read ()
{
    return weechat_config_read (charset_config_file);
}

/*
 * charset_config_write: write charset configuration file
 */

int
charset_config_write ()
{
    return weechat_config_write (charset_config_file);
}

/*
 * charset_check: check if a charset is valid
 *                return 1 if charset is valid, 0 if not valid
 */

int
charset_check (char *charset)
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
 * charset_get: read a charset in config file
 *              we first try with all arguments, then remove one by one
 *              to find charset (from specific to general charset)
 */

char *
charset_get (struct t_config_section *section, char *name,
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
 * charset_decode: decode a string with a charset to internal charset
 */

char *
charset_decode (void *data, char *modifier, char *modifier_data,
                char *string)
{
    char *charset;
    
    /* make C compiler happy */
    (void) data;
    (void) modifier;
    
    charset = charset_get (charset_config_section_decode, modifier_data,
                           charset_default_decode);
    if (charset_debug)
    {
        weechat_printf (NULL,
                        "charset: debug: using 'decode' charset: %s "
                        "(modifier='%s', modifier_data='%s', string='%s')",
                        charset, modifier, modifier_data, string);
    }
    if (charset && charset[0])
        return weechat_iconv_to_internal (charset, string);
    
    return NULL;
}

/*
 * charset_encode: encode a string from internal charset to another one
 */

char *
charset_encode (void *data, char *modifier, char *modifier_data,
                char *string)
{
    char *charset;
    
    /* make C compiler happy */
    (void) data;
    (void) modifier;
    
    charset = charset_get (charset_config_section_encode, modifier_data,
                           charset_default_encode);
    if (charset_debug)
    {
        weechat_printf (NULL,
                        "charset: debug: using 'encode' charset: %s "
                        "(modifier='%s', modifier_data='%s', string='%s')",
                        charset, modifier, modifier_data, string);
    }
    if (charset && charset[0])
        return weechat_iconv_from_internal (charset, string);
    
    return NULL;
}

/*
 * charset_set: set a charset
 */

void
charset_set (struct t_config_section *section, char *type,
             char *name, char *value)
{
    if (charset_config_create_option (NULL,
                                      charset_config_file,
                                      section,
                                      name,
                                      value) > 0)
    {
        if (value && value[0])
            weechat_printf (NULL, _("Charset: %s, %s => %s"),
                            type, name, value);
        else
            weechat_printf (NULL, _("Charset: %s, %s: removed"),
                            type, name);
    }
}

/*
 * charset_command_cb: callback for /charset command
 */

int
charset_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    struct t_config_section *ptr_section;
    int length;
    char *ptr_charset, *option_name, *plugin_name, *category, *name;
    
    /* make C compiler happy */
    (void) data;

    if (argc < 2)
    {
        weechat_printf (NULL,
                        _("%s%s: missing parameters"),
                        weechat_prefix ("error"), "charset");
        return WEECHAT_RC_ERROR;
    }
    
    ptr_section = NULL;
    
    plugin_name = weechat_buffer_get_string (buffer, "plugin");
    category = weechat_buffer_get_string (buffer, "category");
    name = weechat_buffer_get_string (buffer, "name");
    
    length = ((plugin_name) ? strlen (plugin_name) : 0) + 1 +
        strlen (category) + 1 + strlen (name) + 1;
    option_name = malloc (length);
    if (!option_name)
        return WEECHAT_RC_ERROR;
    
    snprintf (option_name, length, "%s%s%s.%s",
              (plugin_name) ? plugin_name : "",
              (plugin_name) ? "." : "",
              category,
              name);
    
    if ((argc > 1) && (weechat_strcasecmp (argv[1], "reset") == 0))
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
            if (weechat_strcasecmp (argv[1], "decode") == 0)
            {
                ptr_section = charset_config_section_decode;
                ptr_charset = argv_eol[2];
            }
            else if (weechat_strcasecmp (argv[1], "encode") == 0)
            {
                ptr_section = charset_config_section_encode;
                ptr_charset = argv_eol[2];
            }
            if (!ptr_section)
            {
                weechat_printf (NULL,
                                _("%s%s: wrong charset type (decode or encode "
                                  "expected)"),
                                weechat_prefix ("error"), "charset");
                if (option_name)
                    free (option_name);
                return WEECHAT_RC_ERROR;
            }
        }
        else
            ptr_charset = argv_eol[1];
        
        if (!charset_check (ptr_charset))
        {
            weechat_printf (NULL,
                            _("%s%s: invalid charset: \"%s\""),
                            weechat_prefix ("error"), "charset", ptr_charset);
            if (option_name)
                free (option_name);
            return WEECHAT_RC_ERROR;
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
 * weechat_plugin_init: init charset plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;
    
    /* get terminal & internal charsets */
    charset_terminal = weechat_info_get ("charset_terminal");
    charset_internal = weechat_info_get ("charset_internal");
    
    /* display message */
    weechat_printf (NULL,
                    _("%s: terminal: %s, internal: %s"),
                    "charset", charset_terminal, charset_internal);
    
    if (!charset_config_init ())
    {
        weechat_printf (NULL,
                        _("%s%s: error creating configuration file"),
                        weechat_prefix("error"), "charset");
        return WEECHAT_RC_ERROR;
    }
    charset_config_read ();
    
    /* /charset command */
    weechat_hook_command ("charset",
                          _("change charset for current buffer"),
                          _("[[decode | encode] charset] | [reset]"),
                          _(" decode: change decoding charset\n"
                            " encode: change encoding charset\n"
                            "charset: new charset for current buffer\n"
                            "  reset: reset charsets for current buffer"),
                          "decode|encode|reset",
                          &charset_command_cb, NULL);
    
    /* modifiers hooks */
    weechat_hook_modifier ("charset_decode", &charset_decode, NULL);
    weechat_hook_modifier ("charset_encode", &charset_encode, NULL);
    
    /* callback for debug */
    weechat_hook_signal ("debug", &charset_debug_cb, NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end charset plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    charset_config_write ();
    
    weechat_config_free (charset_config_file);
    
    return WEECHAT_RC_OK;
}
