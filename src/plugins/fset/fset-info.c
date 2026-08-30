/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Info, infolist and hdata hooks for Fast Set plugin */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-option.h"


/*
 * Return fset infolist "fset_option".
 */

struct t_infolist *
fset_info_infolist_fset_option_cb (const void *pointer, void *data,
                                   const char *infolist_name,
                                   void *obj_pointer,
                                   const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_fset_option *ptr_fset_option;
    int num_options, i;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    if (obj_pointer && !fset_option_valid (obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* Build list with only one option. */
        if (!fset_option_add_to_infolist (ptr_infolist, obj_pointer))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* Build list with all options matching arguments. */
        num_options = weechat_arraylist_size (fset_options);
        for (i = 0; i < num_options; i++)
        {
            ptr_fset_option = weechat_arraylist_get (fset_options, i);
            if (ptr_fset_option
                && (!arguments || !arguments[0]
                    || weechat_string_match (ptr_fset_option->name,
                                             arguments, 0)))
            {
                if (!fset_option_add_to_infolist (ptr_infolist, ptr_fset_option))
                {
                    weechat_infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Hook infolist and hdata.
 */

void
fset_info_init (void)
{
    /* Infolist hooks */
    weechat_hook_infolist (
        "fset_option",
        N_("list of fset options"),
        N_("fset option pointer (optional)"),
        N_("option name "
           "(wildcard \"*\" is allowed) (optional)"),
        &fset_info_infolist_fset_option_cb, NULL, NULL);

    /* Hdata hooks */
    weechat_hook_hdata (
        "fset_option", N_("fset options"),
        &fset_option_hdata_option_cb, NULL, NULL);
}
