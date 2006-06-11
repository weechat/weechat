/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* weechat-aspell.h: Aspell plugin support for WeeChat */

#ifndef WEECHAT_ASPELL__H
#define WEECHAT_ASPELL__H 1

#include <aspell.h>

#define _PLUGIN_NAME    "Aspell"
#define _PLUGIN_VERSION "0.1"
#define _PLUGIN_DESC    "Aspell plugin for WeeChat"
#define _PLUGIN_COMMAND "aspell"

char plugin_name[]        = _PLUGIN_NAME;
char plugin_version[]     = _PLUGIN_VERSION;
char plugin_description[] = _PLUGIN_DESC;
char plugin_command[]     = _PLUGIN_COMMAND;

#define _PLUGIN_OPTION_WORD_SIZE 2
#define _PLUGIN_OPTION_CHECK_SYNC 0
#define _PLUGIN_OPTION_COLOR "red"

typedef struct speller_t
{
    AspellSpeller *speller;
    char *lang;
    int refs;

    struct speller_t *prev_speller;
    struct speller_t *next_speller;
} speller_t;

typedef struct config_t
{
    char *server;
    char *channel;
    speller_t *speller;
    
    struct config_t *prev_config;
    struct config_t *next_config;
} config_t;

typedef struct options_t
{
    int word_size;
    int check_sync;
    int color;
    char *color_name;
} options_t;

typedef struct iso_langs_t
{
    char *code;
    char *name;
} iso_langs_t;

typedef struct iso_countries_t
{
    char *code;
    char *name;
} iso_countries_t;

/* aspell supported langs 2006-05-27 */
iso_langs_t langs_avail[] = 
{
    { "af", "Afrikaans"},
    { "am", "Amharic"},
    { "az", "Azerbaijani"},
    { "be", "Belarusian"},
    { "bg", "Bulgarian"},
    { "bn", "Bengali"},
    { "br", "Breton"},
    { "ca", "Catalan"},
    { "cs", "Czech"},
    { "csb", "Kashubian"},
    { "cy", "Welsh"},
    { "da", "Danish"},
    { "de", "German"},
    { "el", "Greek Modern"},
    { "en", "English"},
    { "eo", "Esperanto"},
    { "es", "Spanish"},
    { "et", "Estonian"},
    { "fa", "Persian"},
    { "fi", "Finnish"},
    { "fo", "Faroese"},
    { "fr", "French"},
    { "ga", "Irish"},
    { "gd", "Gaelic"},
    { "gl", "Galician"},
    { "gu", "Gujarati"},
    { "gv", "Manx"},
    { "he", "Hebrew"},
    { "hi", "Hiligaynon"},
    { "hr", "Croatian"},
    { "hsb", "Upper Sorbian"},
    { "hu", "Hungarian"},
    { "ia", "Interlingua"},
    { "id", "Indonesian"},
    { "is", "Icelandic"},
    { "it", "Italian"},
    { "ku", "Kurdish"},
    { "la", "Latin"},
    { "lt", "Lithuanian"},
    { "lv", "Latvian"},
    { "mg", "Malagasy"},
    { "mi", "Maori"},
    { "mk", "Macedonian"},
    { "mn", "Mongolian"},
    { "mr", "Marathi"},
    { "ms", "Malay"},
    { "mt", "Maltese"},
    { "nb", "Norwegian Bokmal"},
    { "nds", "Saxon Low"},
    { "nl", "Flemish"},
    { "nn", "Norwegian Nynorsk"},
    { "no", "Norwegian"},
    { "ny", "Nyanja"},
    { "or", "Oriya"},
    { "pa", "Panjabi"},
    { "pl", "Polish"},
    { "pt", "Portuguese"},
    { "qu", "Quechua"},
    { "ro", "Romanian"},
    { "ru", "Russian"},
    { "rw", "Kinyarwanda"},
    { "sc", "Sardinian"},
    { "sk", "Slovak"},
    { "sl", "Slovenian"},
    { "sr", "Serbian"},
    { "sv", "Swedish"},
    { "sw", "Swahili"},
    { "ta", "Tamil"},
    { "te", "Telugu"},
    { "tet", "Tetum"},
    { "tl", "Tagalog"},
    { "tn", "Tswana"},
    { "tr", "Turkish"},
    { "uk", "Ukrainian"},
    { "uz", "Uzbek"},
    { "vi", "Vietnamese"},
    { "wa", "Walloon"},
    { "yi", "Yiddish"},
    { "zu", "Zulu"},
    { NULL, NULL}
};

iso_countries_t countries_avail[] = 
{
    { "AT", "Austria" },
    { "BR", "Brazil" },
    { "CA", "Canada" },
    { "CH", "Switzerland" },
    { "DE", "Germany" },
    { "FR", "France" },
    { "GB", "Great Britain" },
    { "PT", "Portugal" },
    { "SK", "Slovakia" },
    { "US", "United States of America" },
    { NULL, NULL}
};

#endif /* WEECHAT_ASPELL__H */
