/*
 * BREXX/370
 */

#include "lstring.h"
#if ALLOW_DECNUMBER
  #include "context.h"
  #include "dnNumber.h"
  #include "options.h"
#endif

/* ---------------- Ladd ------------------- */
void __CDECL
Ladd(const PLstr to, const PLstr A, const PLstr B) {
    int int_a, int_b, overflow;
#if ALLOW_DECNUMBER
    decContext *dc;
#endif

#if ALLOW_DECNUMBER
    Context *context = (Context *) CMSGetPG();
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
        decContextZeroStatus(*dc);
        decNumberAdd(LDEC(*to), TODEC(*A), TODEC(*B), *cp);
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
        LTYPE(*to) = LDECIMAL_TY;
        LLEN(*to) = LMAXLEN(*to) = LDEC_LEN(*to);
        return;
    }
#endif
    L2NUM(A);
    L2NUM(B);
    if ((LTYPE(*A) == LINTEGER_TY) && (LTYPE(*B) == LINTEGER_TY)) {
        /* OK 2 integers - lets add them and see if it is all good */
        int_a = LINT(*A);
        int_b = LINT(*B);

        /*
         * Detect Overflow using inline assembler (a hack but C can't do this)
         * See https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html
         * (Remember we are only in GCC version 3 and in S/370 - so trial and
         * error ... specifically the goto stuff does not seem to be supported)
        */
        overflow = 1;
        __asm__("AR %[a],%[b]\n\t"
                "BC 1,*+6\n\t"
                "SR %[overflow],%[overflow]"
        : [overflow] "=d"(overflow), [a] "+d"(int_a)  /* Output* Operands */
        : [b] "d"(int_b)                                /* Input Operands */
        );
        /*  *Note that the "+d" marks int_a as input/output
         *  (the d means type int)
         */

        if (!overflow) {
            /* No overflow - all done as fast integers */
            LINT(*to) = int_a;
            LTYPE(*to) = LINTEGER_TY;
            LLEN(*to) = sizeof(long);
            return; /* All done */
        }
    }

    /* We have to do the maths using floats */
    LREAL(*to) = TOREAL(*A) + TOREAL(*B);
    LTYPE(*to) = LREAL_TY;
    LLEN(*to) = sizeof(double);
} /* Ladd */
