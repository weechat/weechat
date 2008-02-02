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
#define __USE_GNU
#include <string.h>
#include <iconv.h>

#include "../weechat-plugin.h"
#include "charset.h"


WEECHAT_PLUGIN_NAME("charset");
WEECHAT_PLUGIN_DESCRIPTION("Charset plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL");

struct t_weechat_plugin *weechat_charset_plugin = NULL;
#define weechat_plugin weechat_charset_plugin

struct t_config_file *charset_config_file = NULL;
struct t_charset *charset_list = NULL;
struct t_charset *last_charset = NULL;

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
            charset_debug ^= 1;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * charset_search: search a charset
 */

struct t_charset *
charset_search (char *name)
{
    struct t_charset *ptr_charset;
    
    for (ptr_charset = charset_list; ptr_charset;
         ptr_charset = ptr_charset->next_charset)
    {
        if (strcmp (name, ptr_charset->name) == 0)
            return ptr_charset;
    }
    return NULL;
}

/*
 * charset_new: create new charset and add it to charset list
 */

struct t_charset *
charset_new (char *name, char *charset)
{
    struct t_charset *new_charset, *ptr_charset;
    
    if (!name || !name[0] || !charset || !charset[0])
        return NULL;
    
    ptr_charset = charset_search (name);
    if (ptr_charset)
    {
	if (ptr_charset->charset)
	    free (ptr_charset->charset);
	ptr_charset->charset = strdup (charset);
	return ptr_charset;
    }
    
    new_charset = (struct t_charset *)malloc (sizeof (struct t_charset));
    {
        new_charset->name = strdup (name);
        new_charset->charset = strdup (charset);
        
        new_charset->prev_charset = last_charset;
        new_charset->next_charset = NULL;
        if (charset_list)
            last_charset->next_charset = new_charset;
        else
            charset_list = new_charset;
        last_charset = new_charset;
    }
    
    return new_charset;
}

/*
 * charset_free: free a charset and remove it from list
 */

void
charset_free (struct t_charset *charset)
{
    struct t_charset *new_charset_list;

    /* remove charset from list */
    if (last_charset == charset)
        last_charset = charset->prev_charset;
    if (charset->prev_charset)
    {
        (charset->prev_charset)->next_charset = charset->next_charset;
        new_charset_list = charset_list;
    }
    else
        new_charset_list = charset->next_charset;
    if (charset->next_charset)
        (charset->next_charset)->prev_charset = charset->prev_charset;

    /* free data */
    if (charset->name)
        free (charset->name);
    if (charset->charset)
        free (charset->charset);
    free (charset);
    
    charset_list = new_charset_list;
}

/*
 * charset_free_all: free all charsets
 */

void
charset_free_all ()
{
    while (charset_list)
        charset_free (charset_list);
}

/*
 * charset_config_reaload: reload charset configuration file
 */

int
charset_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    
    charset_free_all ();
    return weechat_config_reload (charset_config_file);
}

/*
 * charset_config_read_line: read a charset in configuration file
 */

void
charset_config_read_line (void *data, struct t_config_file *config_file,
                          char *option_name, char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    
    if (option_name && value)
    {
        /* create new charset */
        if (!charset_new (option_name, value))
        {
            weechat_printf (NULL,
                            _("%s%s: error creating charset \"%s\" => \"%s\""),
                            weechat_prefix ("error"), "charset",
                            option_name, value);
        }
    }
}

/*
 * charseet_config_write_section: write charset section in configuration file
 *                                Return:  0 = successful
 *                                        -1 = write error
 */

void
charset_config_write_section (void *data, struct t_config_file *config_file,
                              char *section_name)
{
    struct t_charset *ptr_charset;
    
    /* make C compiler happy */
    (void) data;
    
    weechat_config_write_line (config_file, section_name, NULL);
    
    for (ptr_charset = charset_list; ptr_charset;
         ptr_charset = ptr_charset->next_charset)
    {
        weechat_config_write_line (config_file,
                                   ptr_charset->name,
                                   "\"%s\"",
                                   ptr_charset->charset);
    }
}

/*
 * charset_config_write_default_aliases: write default charsets in configuration file
 */

void
charset_config_write_default_charsets (void *data,
                                       struct t_config_file *config_file,
                                       char *section_name)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_config_write_line (config_file, section_name, NULL);

    if (charset_terminal && charset_internal
        && (strcasecmp (charset_terminal,
                        charset_internal) != 0))
        weechat_config_write_line (config_file, "decode", "%s", charset_terminal);
    else
        weechat_config_write_line (config_file, "decode", "%s", "iso-8859-1");
}

/*
 * charset_config_init: init charset configuration file
 *                      return: 1 if ok, 0 if error
 */

int
charset_config_init ()
{
    struct t_config_section *ptr_section;
    
    charset_config_file = weechat_config_new (CHARSET_CONFIG_FILENAME,
                                              &charset_config_reload, NULL);
    if (!charset_config_file)
        return 0;
    
    ptr_section = weechat_config_new_section (charset_config_file, "charset",
                                              &charset_config_read_line,
                                              NULL,
                                              &charset_config_write_section,
                                              NULL,
                                              &charset_config_write_default_charsets,
                                              NULL);
    if (!ptr_section)
    {
        weechat_config_free (charset_config_file);
        return 0;
    }
    
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
 * charset_parse_irc_msg: return nick, command, channel and position
 *                        of arguments in IRC message
 */

void
charset_parse_irc_msg (char *message, char **nick, char **command,
                       char **channel, char **pos_args)
{
    char *pos, *pos2, *pos3, *pos4, *pos_tmp;
    
    *nick = NULL;
    *command = NULL;
    *channel = NULL;
    *pos_args = NULL;
    
    if (message[0] == ':')
    {
        pos = message + 1;
        pos_tmp = strchr (pos, ' ');
        if (pos_tmp)
            pos_tmp[0] = '\0';
        pos2 = strchr (pos, '!');
        if (pos2)
            *nick = weechat_strndup (pos, pos2 - pos);
        else
        {
            pos2 = strchr (pos, ' ');
            if (pos2)
                *nick = weechat_strndup (pos, pos2 - pos);
        }
        if (pos_tmp)
            pos_tmp[0] = ' ';
        pos = strchr (message, ' ');
        if (!pos)
            pos = message;
    }
    else
        pos = message;
    
    if (pos && pos[0])
    {
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            *command = weechat_strndup (pos, pos2 - pos);
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
            *pos_args = pos2;
            if (pos2[0] != ':')
            {
                if ((pos2[0] == '#') || (pos2[0] == '&')
                    || (pos2[0] == '+') || (pos2[0] == '!'))
                {
                    pos3 = strchr (pos2, ' ');
                    if (pos3)
                        *channel = weechat_strndup (pos2, pos3 - pos2);
                    else
                        *channel = strdup (pos2);
                }
                else
                {
                    pos3 = strchr (pos2, ' ');
                    if (!*nick)
                    {
                        if (pos3)
                            *nick = weechat_strndup (pos2, pos3 - pos2);
                        else
                            *nick = strdup (pos2);
                    }
                    if (pos3)
                    {
                        pos3++;
                        while (pos3[0] == ' ')
                            pos3++;
                        if ((pos3[0] == '#') || (pos3[0] == '&')
                            || (pos3[0] == '+') || (pos3[0] == '!'))
                        {
                            pos4 = strchr (pos3, ' ');
                            if (pos4)
                                *channel = weechat_strndup (pos3, pos4 - pos3);
                            else
                                *channel = strdup (pos3);
                        }
                    }
                }
            }
        }
    }
}

/*
 * charset_set: set a charset
 *              return 1 if ok, 0 if error
 */

int
charset_set (char *name, char *charset)
{
    struct t_charset *ptr_charset;
    
    if (charset && charset[0])
    {
        if (charset_new (name, charset))
        {
            weechat_printf (NULL,
                            _("%sCharset \"%s\" => \"%s\""),
                            weechat_prefix ("info"), name, charset);
        }
        else
        {
            weechat_printf (NULL,
                            _("%s%s: error creating charset \"%s\" "
                              "=> \"%s\""),
                            weechat_prefix ("error"), "charset", name, charset);
            return 0;
        }
    }
    else
    {
        ptr_charset = charset_search (name);
        if (!ptr_charset)
        {
            weechat_printf (NULL,
                            _("%s%s: charset \"%s\" not found"),
                            weechat_prefix ("error"), "charset", name);
            return 0;
        }
        charset_free (ptr_charset);
        weechat_printf (NULL,
                        _("%sCharset \"%s\" removed"),
                        weechat_prefix ("info"), name);
    }
    
    return 1;
}

/*
 * charset_get: read a charset in config file
 *              type is "decode" or "encode"
 *              we first try with all arguments, then remove one by one
 *              to find charset (from specific to general charset)
 */

char *
charset_get (char *type, char *name)
{
    char *option_name, *ptr_end;
    struct t_charset *ptr_charset;
    int length;

    length = strlen (type) + 1 + strlen (name) + 1; 
    option_name = (char *)malloc (length * sizeof (char));
    if (option_name)
    {
        snprintf (option_name, length, "%s.%s",
                  type, name);
        ptr_end = option_name + strlen (option_name);
        while (ptr_end >= option_name)
        {
            ptr_charset = charset_search (option_name);
            if (ptr_charset)
            {
                free (option_name);
                return ptr_charset->charset;
            }
            ptr_end--;
            while ((ptr_end >= option_name) && (ptr_end[0] != '.'))
            {
                ptr_end--;
            }
            if ((ptr_end >= option_name) && (ptr_end[0] == '.'))
                ptr_end[0] = '\0';
        }
        ptr_charset = charset_search (option_name);
        
        free (option_name);
        
        if (ptr_charset)
            return ptr_charset->charset;
    }
    
    /* nothing found => no decode/encode for this message! */
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
    
    charset = charset_get ("decode", modifier_data);
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
    
    charset = charset_get ("encode", modifier_data);
    if (charset && charset[0])
        return weechat_iconv_from_internal (charset, string);
    
    return NULL;
}

/*
 * charset_command_cb: callback for /charset command
 */

int
charset_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    int charset_found, length, rc;
    struct t_charset *ptr_charset;
    char *option_name, *ptr_value;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if ((argc > 2) && (strcmp (argv[2], "=") == 0))
    {
        ptr_value = (argc > 3) ? argv_eol[3] : NULL;
        if ((weechat_strncasecmp (argv[1], "decode.", 7) != 0)
            && (weechat_strncasecmp (argv[1], "encode.", 7) != 0))
        {
            length = strlen (argv[1]) + strlen ("decode.") + 1;
            option_name = (char *)malloc (length * sizeof (char));
            if (option_name)
            {
                rc = 1;
                snprintf (option_name, length, "decode.%s", argv[1]);
                if (!charset_set (option_name, ptr_value))
                    rc = 0;
                snprintf (option_name, length, "encode.%s", argv[1]);
                if (!charset_set (option_name, ptr_value))
                    rc = 0;
                if (!rc)
                    return WEECHAT_RC_ERROR;
            }
        }
        else
        {
            if (!charset_set (argv[1], ptr_value))
                return WEECHAT_RC_ERROR;
        }
    }
    else
    {
        /* list all charsets */
        if (charset_list)
        {
            weechat_printf (NULL, "");
            if (argc == 1)
                weechat_printf (NULL, _("List of charsets:"));
            else
                weechat_printf (NULL, _("List of charsets with \"%s\":"),
                                argv_eol[1]);
            charset_found = 0;
            for (ptr_charset = charset_list; ptr_charset;
                 ptr_charset = ptr_charset->next_charset)
            {
                if ((argc < 2)
                    || (weechat_strcasestr (ptr_charset->name, argv_eol[1])))
                {
                    charset_found = 1;
                    weechat_printf (NULL,
                                    "  %s %s=>%s %s",
                                    ptr_charset->name,
                                    weechat_color ("color_chat_delimiters"),
                                    weechat_color ("color_chat"),
                                    ptr_charset->charset);
                }
            }
            if (!charset_found)
                weechat_printf (NULL, _("No charset found"));
        }
        else
            weechat_printf (NULL, _("No charset defined"));
    }
    
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
                    _("%s%s: terminal: %s, internal: %s"),
                    weechat_prefix ("info"), "charset",
                    charset_terminal, charset_internal);
    
    if (!charset_config_init ())
    {
        weechat_printf (NULL,
                        _("%s%s: error creating configuration file \"%s\""),
                        weechat_prefix("error"), "charset",
                        CHARSET_CONFIG_FILENAME);
        return WEECHAT_RC_ERROR;
    }
    charset_config_read ();
    
    /* add command handler */
    weechat_hook_command ("charset",
                          _("manage charsets"),
                          _("[[type.]plugin.string [= charset]]"),
                          _("   type: \"decode\" or \"encode\" (if type is "
                            "omitted, then both \"decode\" and \"encode\" are "
                            "set)\n"
                            " plugin: plugin name\n"
                            " string: string specific to plugin (for example "
                            "a server name or server.channel for IRC plugin)\n"
                            "charset: charset to use (if empty, then charset "
                            "is removed)\n\n"
                            "Examples :\n"
                            "/charset decode iso-8859-15 => set global "
                            "decode charset to iso-8859-15\n"
                            "/charset encode iso-8859-15 => set global "
                            "encode charset to iso-8859-15\n"
                            "/charset decode.irc.freenode => set decode "
                            "charset to iso-8859-15 for IRC server "
                            "\"freenode\" (all channels)\n"
                            "/charset decode.irc.freenode.#weechat => set "
                            "decode charset to iso-8859-15 for IRC channel "
                            "\"#weechat\" on server \"freenode\""),
                          "%(charset_name) %(charset)",
                          &charset_command_cb, NULL);
    
    /* add messge modifiers */
    weechat_hook_modifier ("charset_decode", &charset_decode, NULL);
    weechat_hook_modifier ("charset_encode", &charset_encode, NULL);
    
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
    charset_free_all ();
    weechat_config_free (charset_config_file);
    
    return WEECHAT_RC_OK;
}
