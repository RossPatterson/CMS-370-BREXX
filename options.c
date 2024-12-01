#ifdef __CMS__
#include <cmssys.h>
#define HAS_CONTEXT
#endif

#include "lstring.h"
#include "options.h"

#ifdef HAS_CONTEXT
#  include "context.h"
#  define CONTEXT(file, field) (context->file##field)
#else
#  define CONTEXT(file, field) field
#endif

typedef struct OptList_st {
    char *name;
    long flags;
} OptList;
OptList optlist[] = {
    {"STORAGE_DECIMAL", OPT_STORAGE_DECIMAL},
    {NULL, 0L}
};

/* ----------------- ParseOptions ------------------ */
void __CDECL
ParseOptions(const PLstr value) {
    char *val_ptr, *opt_ptr, *val_end;
    int i, j, opt_no, in_opt;
    long word_count, word_num;
    Lstr word;
#ifdef HAS_CONTEXT
    Context *context = (Context *) CMSGetPG();
#endif
    LINITSTR(word);
    CONTEXT(interpre_, options) = 0UL;
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
                    CONTEXT(interpre_, options) &= (0xffffffffffffffffUL - optlist[j].flags);
                } else {
                    CONTEXT(interpre_, options) |= optlist[j].flags;
                }
                break;
            }
        }
    }
    LFREESTR(word);
} /* ParseOptions */
