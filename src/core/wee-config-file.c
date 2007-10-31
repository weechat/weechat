/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* wee-config-file.c: I/O functions to read/write options in a config file */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "weechat.h"
#include "wee-config-file.h"
#include "wee-config-option.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"


/*
 * config_file_read_option: read an option for a section in config file
 *                          This function is called when a new section begins
 *                          or for an option of a section
 *                          if options == NULL, then option is out of section
 *                          if option_name == NULL, it's beginning of section
 *                          otherwise it's an option for a section
 *                          Return:  0 = successful
 *                                  -1 = option not found
 *                                  -2 = bad format/value
 */

int
config_file_read_option (struct t_config_option *options,
                         char *option_name, char *value)
{
    struct t_config_option *ptr_option;
    
    if (option_name)
    {
        /* no option allowed outside a section by default */
        if (!options)
            return -1;
        
        /* search option in list of options for section */
        ptr_option = config_option_search (options, option_name);
        if (!ptr_option)
            return -1;
        
        /* set value for option found */
        if (config_option_set (ptr_option, value) < 0)
            return -2;
    }
    else
    {
        /* by default, does nothing for a new section */
    }
    
    /* all ok */
    return 0;
}

/*
 * config_file_read: read a configuration file
 *                   return:  0 = successful
 *                           -1 = config file file not found
 *                           -2 = error in config file
 */

int
config_file_read (char **config_sections, struct t_config_option **options,
                  t_config_func_read_option **read_functions,
                  t_config_func_read_option *read_function_default,
                  t_config_func_write_options **write_default_functions,
                  char *config_filename)
{
    int filename_length;
    char *filename;
    FILE *file;
    int section, line_number, rc;
    char line[1024], *ptr_line, *ptr_line2, *pos, *pos2;
    
    filename_length = strlen (weechat_home) + strlen (config_filename) + 2;
    filename = (char *) malloc (filename_length * sizeof (char));
    if (!filename)
        return -2;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_filename);
    if ((file = fopen (filename, "r")) == NULL)
    {
        /* if no default write functions provided and that config file
           is not here, then return without doing anything */
        if (!write_default_functions)
            return 0;

        /* create default config, then go on with reading */
        config_file_write_default (config_sections, options,
                                   write_default_functions,
                                   config_filename);
        if ((file = fopen (filename, "r")) == NULL)
        {
            gui_chat_printf (NULL, _("%s config file \"%s\" not found.\n"),
                             WEECHAT_WARNING, filename);
            free (filename);
            return -1;
        }
    }
    
    config_option_section_option_set_default_values (config_sections, options);
    
    section = -1;
    line_number = 0;
    while (!feof (file))
    {
        ptr_line = fgets (line, sizeof (line) - 1, file);
        line_number++;
        if (ptr_line)
        {
            /* encode line to internal charset */
            ptr_line2 = string_iconv_to_internal (NULL, ptr_line);
            if (ptr_line2)
            {
                snprintf (line, sizeof (line) - 1, "%s", ptr_line2);
                free (ptr_line2);
            }
            
            /* skip spaces */
            while (ptr_line[0] == ' ')
                ptr_line++;
            /* not a comment and not an empty line */
            if ((ptr_line[0] != '#') && (ptr_line[0] != '\r')
                && (ptr_line[0] != '\n'))
            {
                /* beginning of section */
                if (ptr_line[0] == '[')
                {
                    pos = strchr (line, ']');
                    if (pos == NULL)
                        gui_chat_printf (NULL,
                                         _("%s %s, line %d: invalid syntax, "
                                           "missing \"]\"\n"),
                                         WEECHAT_WARNING, filename,
                                         line_number);
                    else
                    {
                        pos[0] = '\0';
                        pos = ptr_line + 1;
                        section = config_option_section_get_index (config_sections, pos);
                        if (section < 0)
                            gui_chat_printf (NULL,
                                             _("%s %s, line %d: unknown section "
                                               "identifier (\"%s\")\n"),
                                             WEECHAT_WARNING, filename,
                                             line_number, pos);
                        else
                        {
                            rc = ((int) (read_functions[section]) (options[section],
                                                                   NULL, NULL));
                            if (rc < 0)
                                gui_chat_printf (NULL,
                                                 _("%s %s, line %d: error "
                                                   "reading new section \"%s\"\n"),
                                                 WEECHAT_WARNING, filename,
                                                 line_number, pos);
                        }
                    }
                }
                else
                {
                    pos = strchr (line, '=');
                    if (pos == NULL)
                        gui_chat_printf (NULL,
                                         _("%s %s, line %d: invalid syntax, "
                                           "missing \"=\"\n"),
                                         WEECHAT_WARNING, filename,
                                         line_number);
                    else
                    {
                        pos[0] = '\0';
                        pos++;
                        
                        /* remove spaces before '=' */
                        pos2 = pos - 2;
                        while ((pos2 > line) && (pos2[0] == ' '))
                        {
                            pos2[0] = '\0';
                            pos2--;
                        }
                        
                        /* skip spaces after '=' */
                        while (pos[0] && (pos[0] == ' '))
                        {
                            pos++;
                        }
                        
                        /* remove CR/LF */
                        pos2 = strchr (pos, '\r');
                        if (pos2 != NULL)
                            pos2[0] = '\0';
                        pos2 = strchr (pos, '\n');
                        if (pos2 != NULL)
                            pos2[0] = '\0';
                        
                        /* remove simple or double quotes 
                           and spaces at the end */
                        if (strlen(pos) > 1)
                        {
                            pos2 = pos + strlen (pos) - 1;
                            while ((pos2 > pos) && (pos2[0] == ' '))
                            {
                                pos2[0] = '\0';
                                pos2--;
                            }
                            pos2 = pos + strlen (pos) - 1;
                            if (((pos[0] == '\'') &&
                                 (pos2[0] == '\'')) ||
                                ((pos[0] == '"') &&
                                 (pos2[0] == '"')))
                            {
                                pos2[0] = '\0';
                                pos++;
                            }
                        }
                        
                        if (section < 0)
                            rc = ((int) (read_function_default) (NULL, line, pos));
                        else
                            rc = ((int) (read_functions[section]) (options[section],
                                                                   line, pos));
                        if (rc < 0)
                        {
                            switch (rc)
                            {
                                case -1:
                                    if (section < 0)
                                        gui_chat_printf (NULL,
                                                         _("%s %s, line %d: unknown "
                                                           "option \"%s\" "
                                                           "(outside a section)\n"),
                                                         WEECHAT_WARNING,
                                                         filename,
                                                         line_number, line);
                                    else
                                        gui_chat_printf (NULL,
                                                         _("%s %s, line %d: option "
                                                           "\"%s\" unknown for "
                                                           "section \"%s\"\n"),
                                                         WEECHAT_WARNING, filename,
                                                         line_number, line,
                                                         config_sections[section]);
                                    break;
                                case -2:
                                    gui_chat_printf (NULL,
                                                     _("%s %s, line %d: invalid "
                                                       "value for option \"%s\"\n"),
                                                     WEECHAT_WARNING, filename,
                                                     line_number, line);
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    fclose (file);
    free (filename);
    
    return 0;
}

/*
 * config_file_write_options: write a section with options in a config file
 *                            This function is called when config is saved,
 *                            for a section with standard options
 *                            Return:  0 = successful
 *                                    -1 = write error
 */

int
config_file_write_options (FILE *file, char *section_name,
                           struct t_config_option *options)
{
    int i;
    
    string_iconv_fprintf (file, "\n[%s]\n", section_name);
    
    for (i = 0; options[i].name; i++)
    {
        switch (options[i].type)
        {
            case OPTION_TYPE_BOOLEAN:
                string_iconv_fprintf (file, "%s = %s\n",
                                      options[i].name,
                                      (options[i].ptr_int &&
                                       *options[i].ptr_int) ? 
                                      "on" : "off");
                break;
            case OPTION_TYPE_INT:
                string_iconv_fprintf (file, "%s = %d\n",
                                      options[i].name,
                                      (options[i].ptr_int) ?
                                      *options[i].ptr_int :
                                      options[i].default_int);
                break;
            case OPTION_TYPE_INT_WITH_STRING:
                string_iconv_fprintf (file, "%s = %s\n",
                                      options[i].name,
                                      (options[i].ptr_int) ?
                                      options[i].array_values[*options[i].ptr_int] :
                                      options[i].array_values[options[i].default_int]);
                break;
            case OPTION_TYPE_COLOR:
                string_iconv_fprintf (file, "%s = %s\n",
                                      options[i].name,
                                      (options[i].ptr_int) ?
                                      gui_color_get_name (*options[i].ptr_int) :
                                      options[i].default_string);
                break;
            case OPTION_TYPE_STRING:
                string_iconv_fprintf (file, "%s = \"%s\"\n",
                                      options[i].name,
                                      (options[i].ptr_string) ?
                                      *options[i].ptr_string :
                                      options[i].default_string);
                break;
        }
    }
    
    /* all ok */
    return 0;
}

/*
 * config_file_write_options_default_values: write a section with options in a config file
 *                                           This function is called when new config file
 *                                           is created, with default values, for a
 *                                           section with standard options
 *                                           Return:  0 = successful
 *                                                   -1 = write error
 */

int
config_file_write_options_default_values (FILE *file, char *section_name,
                                          struct t_config_option *options)
{
    int i;
    
    string_iconv_fprintf (file, "\n[%s]\n", section_name);
    
    for (i = 0; options[i].name; i++)
    {
        switch (options[i].type)
        {
            case OPTION_TYPE_BOOLEAN:
                string_iconv_fprintf (file, "%s = %s\n",
                                      options[i].name,
                                      (options[i].default_int) ?
                                      "on" : "off");
                break;
            case OPTION_TYPE_INT:
                string_iconv_fprintf (file, "%s = %d\n",
                                      options[i].name,
                                      options[i].default_int);
                break;
            case OPTION_TYPE_INT_WITH_STRING:
            case OPTION_TYPE_COLOR:
                string_iconv_fprintf (file, "%s = %s\n",
                                      options[i].name,
                                      options[i].default_string);
                break;
            case OPTION_TYPE_STRING:
                string_iconv_fprintf (file, "%s = \"%s\"\n",
                                      options[i].name,
                                      options[i].default_string);
                break;
        }
    }
    
    /* all ok */
    return 0;
}

/*
 * config_file_write_header: write header in a config file
 */

void
config_file_write_header (FILE *file)
{
    time_t current_time;
    
    current_time = time (NULL);
    string_iconv_fprintf (file, _("#\n# %s configuration file, created by "
                                  "%s v%s on %s"),
                          PACKAGE_NAME, PACKAGE_NAME, PACKAGE_VERSION,
                          ctime (&current_time));
    string_iconv_fprintf (file, _("# WARNING! Be careful when editing this file, "
                                  "WeeChat may write it at any time.\n#\n"));
}

/*
 * config_file_write_default: create a default configuration file
 *                            return:  0 if ok
 *                                   < 0 if error
 */

int
config_file_write_default (char **config_sections, struct t_config_option **options,
                           t_config_func_write_options **write_functions,
                           char *config_filename)
{
    int filename_length, i, rc;
    char *filename;
    FILE *file;
    
    filename_length = strlen (weechat_home) +
        strlen (config_filename) + 2;
    filename =
        (char *) malloc (filename_length * sizeof (char));
    if (!filename)
        return -2;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_filename);
    if ((file = fopen (filename, "w")) == NULL)
    {
        gui_chat_printf (NULL, _("%s cannot create file \"%s\"\n"),
                         WEECHAT_ERROR, filename);
        free (filename);
        return -1;
    }
    
    string_iconv_fprintf (stdout,
                          _("%s: creating default config file \"%s\"...\n"),
                          PACKAGE_NAME,
                          config_filename);
    weechat_log_printf (_("Creating default config file \"%s\"\n"),
                        config_filename);
    
    config_file_write_header (file);
    
    for (i = 0; config_sections[i]; i++)
    {
        rc = ((int) (write_functions[i]) (file, config_sections[i],
                                          options[i]));
    }
    
    fclose (file);
    chmod (filename, 0600);
    free (filename);
    return 0;
}

/*
 * config_file_write: write a configurtion file
 *                    return:  0 if ok
 *                           < 0 if error
 */

int
config_file_write (char **config_sections, struct t_config_option **options,
                   t_config_func_write_options **write_functions,
                   char *config_filename)
{
    int filename_length, i, rc;
    char *filename, *filename2;
    FILE *file;
    
    filename_length = strlen (weechat_home) +
        strlen (config_filename) + 2;
    filename =
        (char *) malloc (filename_length * sizeof (char));
    if (!filename)
        return -2;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_filename);
    
    filename2 = (char *) malloc ((filename_length + 32) * sizeof (char));
    if (!filename2)
    {
        free (filename);
        return -2;
    }
    snprintf (filename2, filename_length + 32, "%s.weechattmp", filename);
    
    if ((file = fopen (filename2, "w")) == NULL)
    {
        gui_chat_printf (NULL, _("%s cannot create file \"%s\"\n"),
                         WEECHAT_ERROR, filename2);
        free (filename);
        free (filename2);
        return -1;
    }
    
    config_file_write_header (file);

    for (i = 0; config_sections[i]; i++)
    {
        rc = ((int) (write_functions[i]) (file, config_sections[i],
                                          options[i]));
    }
    
    fclose (file);
    chmod (filename2, 0600);
    unlink (filename);
    rc = rename (filename2, filename);
    free (filename);
    free (filename2);
    if (rc != 0)
        return -1;
    return 0;
}
