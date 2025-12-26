#include <cmssys.h>

#include "lstring.h"
#include "options.h"

typedef struct OptList_st {
    char *name;
    long flags;
} OptList;
OptList optlist[] = {
    {"COMPAT_BREXX_1_0",                OPT_COMPAT_BREXX_1_0},
    {"COMPAT_BREXX_1_1",                OPT_COMPAT_BREXX_1_1},
#if ALLOW_DECNUMBER
    {"DECIMAL_MATH",                    OPT_DECIMAL_MATH},
#endif
    {"STORAGE_DECIMAL",                 OPT_STORAGE_DECIMAL},
    {NULL, OPT_NONE},
};

/* ----------------- ParseOptions ------------------ */
void __CDECL
ParseOptions(const PLstr value) {
    char *val_ptr, *opt_ptr, *val_end;
    int i, j, opt_no, in_opt;
    long word_count, word_num;
    Lstr word;

    Context *context = (Context *) CMSGetPG();
    LINITSTR(word);
    (context->interpre_options) = 0UL;
    opt_ptr = LSTR(*value);
    opt_no = FALSE;
    word_count = Lwords(value);
    for (word_num = 1L; word_num <= word_count; word_num++) {
        Lword(&word, value, word_num);
        LASCIIZ(word);
        opt_ptr = LSTR(word);
        if (opt_no = (opt_ptr[0] == 'N' && opt_ptr[1] == 'O')) {
            opt_ptr += 2;
        }
        for (j = 0; optlist[j].name != NULL; j++) {
            if (strcmp(optlist[j].name, opt_ptr) == 0) {
                if (opt_no) {
                    (context->interpre_options) &= (0xffffffffffffffffUL - optlist[j].flags);
                } else {
                    (context->interpre_options) |= optlist[j].flags;
                }
                break;
            }
        }
    }
    LFREESTR(word);
} /* ParseOptions */
