extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "src/core/wee-utf8.h"
#include "src/core/wee-string.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
        char *new_m = (char*)malloc(size+1);
        memcpy(new_m, data,size);
        new_m[size] = '\0';

        char *res = NULL;
        res = string_reverse_screen(new_m);
        if (res != NULL) free(res);

        res = string_convert_escaped_chars(new_m);
        if (res != NULL) free(res);

		char **argv = NULL;
		int argc = -1;
		argv = string_split_shell(new_m, &argc);
		if (argv != NULL) string_free_split (argv);

        if (size > 20)
        {
                char *highlight_words = (char*)malloc(10);
                highlight_words[0] = new_m[0];
                highlight_words[1] = new_m[2];
                highlight_words[2] = new_m[4];
                highlight_words[3] = new_m[6];
                highlight_words[4] = new_m[8];
                highlight_words[5] = new_m[10];
                highlight_words[6] = new_m[12];
                highlight_words[7] = new_m[14];
                highlight_words[8] = new_m[16];
                highlight_words[9] = '\0';

                string_has_highlight(new_m, highlight_words);
                free(highlight_words);
        }

        free(new_m);
        return 0;
}
