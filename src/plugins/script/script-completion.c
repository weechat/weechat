/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * script-completion.c: completion for script commands
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-repo.h"


/*
 * script_completion_scripts_cb: callback for completion with scripts in
 *                               repository
 */

int
script_completion_scripts_cb (void *data, const char *completion_item,
                              struct t_gui_buffer *buffer,
                              struct t_gui_completion *completion)
{
    struct t_repo_script *ptr_script;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_script = repo_scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        weechat_hook_completion_list_add (completion,
                                          ptr_script->name_with_extension,
                                          0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * script_completion_scripts_installed_cb: callback for completion with scripts
 *                                         installed
 */

int
script_completion_scripts_installed_cb (void *data, const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_repo_script *ptr_script;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_script = repo_scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (ptr_script->status & SCRIPT_STATUS_INSTALLED)
        {
            weechat_hook_completion_list_add (completion,
                                              ptr_script->name_with_extension,
                                              0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * script_completion_exec_file_cb: callback called for each file in script
 *                                 directories
 */

void
script_completion_exec_file_cb (void *data, const char *filename)
{
    struct t_gui_completion *completion;
    const char *extension;
    char *pos, *filename2, *ptr_base_name;

    completion = (struct t_gui_completion *)(((void **)data)[0]);
    extension = (const char *)(((void **)data)[1]);

    pos = strrchr (filename, '.');
    if (!pos)
        return;

    /* ignore scripts that do not ends with expected extension */
    if (strcmp (pos + 1, extension) != 0)
        return;

    filename2 = strdup (filename);
    if (filename2)
    {
        ptr_base_name = basename (filename2);
        weechat_hook_completion_list_add (completion,
                                          ptr_base_name,
                                          0, WEECHAT_LIST_POS_SORT);
        free (filename2);
    }
}

/*
 * script_completion_scripts_files_cb: callback for completion with files in
 *                                     script directories
 */

int
script_completion_scripts_files_cb (void *data, const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    const char *weechat_home;
    char *directory;
    int length, i;
    void *pointers[2];

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    weechat_home = weechat_info_get ("weechat_dir", NULL);

    length = strlen (weechat_home) + 128 + 1;
    directory = malloc (length);
    if (directory)
    {
        for (i = 0; script_language[i]; i++)
        {
            pointers[0] = completion;
            pointers[1] = script_extension[i];

            /* look for files in "~/.weechat/<language>/" */
            snprintf (directory, length,
                      "%s/%s", weechat_home, script_language[i]);
            weechat_exec_on_files (directory, 0,
                                   pointers, &script_completion_exec_file_cb);

            /* look for files in "~/.weechat/<language>/autoload/" */
            snprintf (directory, length,
                      "%s/%s/autoload", weechat_home, script_language[i]);
            weechat_exec_on_files (directory, 0,
                                   pointers, &script_completion_exec_file_cb);
        }
        free (directory);
    }

    return WEECHAT_RC_OK;
}

/*
 * script_completion_init: init completion for script plugin
 */

void
script_completion_init ()
{
    weechat_hook_completion ("script_scripts",
                             N_("list of scripts in repository"),
                             &script_completion_scripts_cb, NULL);
    weechat_hook_completion ("script_scripts_installed",
                             N_("list of scripts installed (from repository)"),
                             &script_completion_scripts_installed_cb, NULL);
    weechat_hook_completion ("script_files",
                             N_("files in script directories"),
                             &script_completion_scripts_files_cb, NULL);
}
