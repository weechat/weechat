/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_HOOK_COMMAND_H
#define WEECHAT_HOOK_COMMAND_H

struct t_weechat_plugin;
struct t_infolist_item;
struct t_gui_buffer;

#define HOOK_COMMAND(hook, var) (((struct t_hook_command *)hook->hook_data)->var)

/* max calls that can be done for a command (recursive calls) */
#define HOOK_COMMAND_MAX_CALLS  5

/* return code when a command is executed */
#define HOOK_COMMAND_EXEC_OK                    1
#define HOOK_COMMAND_EXEC_ERROR                 0
#define HOOK_COMMAND_EXEC_NOT_FOUND            -1
#define HOOK_COMMAND_EXEC_AMBIGUOUS_PLUGINS    -2
#define HOOK_COMMAND_EXEC_AMBIGUOUS_INCOMPLETE -3
#define HOOK_COMMAND_EXEC_RUNNING              -4

/* same command found with a different case */
#define HOOK_COMMAND_SIMILAR_DIFF_CASE_ONLY -99

typedef int (t_hook_callback_command)(const void *pointer, void *data,
                                      struct t_gui_buffer *buffer,
                                      int argc, char **argv, char **argv_eol);

struct t_hook_command
{
    t_hook_callback_command *callback;  /* command callback                 */
    char *command;                      /* name of command (without '/')    */
    char *description;                  /* (for /help) short cmd description*/
    char *args;                         /* (for /help) command arguments    */
    char *args_description;             /* (for /help) args long description*/
    char *completion;                   /* template for completion          */

    /* templates */
    int cplt_num_templates;             /* number of templates for compl.   */
    char **cplt_templates;              /* completion templates             */
    char **cplt_templates_static;       /* static part of template (at      */
                                        /* beginning                        */

    /* arguments for each template */
    int *cplt_template_num_args;        /* number of arguments for template */
    char ***cplt_template_args;         /* arguments for each template      */

    /* concatenation of arg N for each template */
    int cplt_template_num_args_concat; /* number of concatenated arguments  */
    char **cplt_template_args_concat;  /* concatenated arguments            */

    /* other features */
    int keep_spaces_right;             /* if set to 1: don't strip trailing */
                                       /* spaces in args when the command   */
                                       /* is executed                       */
};

struct t_hook_command_similar
{
    const char *command;               /* pointer to command name           */
    int relevance;                     /* lower is better: mostly based on  */
                                       /* Levenshtein distance between cmds */
};

extern char *hook_command_get_description (struct t_hook *hook);
extern char *hook_command_format_args_description (const char *args_description);
extern struct t_hook *hook_command (struct t_weechat_plugin *plugin,
                                    const char *command,
                                    const char *description,
                                    const char *args,
                                    const char *args_description,
                                    const char *completion,
                                    t_hook_callback_command *callback,
                                    const void *callback_pointer,
                                    void *callback_data);
extern int hook_command_exec (struct t_gui_buffer *buffer, int any_plugin,
                              struct t_weechat_plugin *plugin,
                              const char *string);
extern void hook_command_display_error_unknown (const char *command);
extern void hook_command_free_data (struct t_hook *hook);
extern int hook_command_add_to_infolist (struct t_infolist_item *item,
                                         struct t_hook *hook);
extern void hook_command_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_COMMAND_H */
