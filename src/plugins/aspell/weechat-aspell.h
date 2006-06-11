
#ifndef WEECHAT_ASPELL__H
#define WEECHAT_ASPELL__H 1

#include <aspell.h>

char plugin_name[]        = "Aspell";
char plugin_version[]     = "0.1";
char plugin_description[] = "Aspell plugin for WeeChat";
char plugin_command[]     = "aspell";

#define _OPTION_WORD_SIZE 2
#define _OPTION_CHECK_SYNC 0
#define _OPTION_COLOR "red"

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
