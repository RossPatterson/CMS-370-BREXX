#include "lstring.h"
#include "options.h"

/* ----------------- ParseOptions ------------------ */
void __CDECL
ParseOptions(const PLstr value) {
    char *val_ptr, *opt_ptr;
    int opt_index, in_opt;

    if ((context->interpre_options->list) != NULL)
        FREE((context->interpre_options->list));
    if ((context->interpre_options->words) != NULL)
        FREE((context->interpre_options->words));
    L2STR(value);
    opt_count = context->interpre_options->count = Lwords(value);
    opt_list = context->interpre_options->list = malloc(opt_count * (sizeof *char));
    opt_ptr = context->interpre_options->words = malloc(LLEN(value) + 1);
    for (val_ptr = LSTR(value), opt_index = 0, in_opt = FALSE;
            val_ptr < (value+LLEN(value)); val_ptr++) {
        if (ISSPACE(*val_ptr)) {
            if (in_opt) {
                *(opt_ptr++) = '\0';
                in_opt = FALSE;
            }
        } else {
            if (!in_opt) {
                in_opt = TRUE;
                opt_list[opt_index++] = opt_ptr;
            }
            *(opt_ptr++) = *val_ptr;
        }
    }
} /* ParseOptions */

/* ----------------- CheckOption ------------------ */
int __CDECL
CheckOption(char *value) {
    Context *context = (Context *) CMSGetPG();
    
    for (i = 0; i < context->interpre_options->count; i++) {
        if (strcmp(context->interpre_options->list[i], value))
            return 1;
    }
    return 0;
} /* CheckOption */
