/*
 * $Id: mult.c,v 1.4 2008/07/15 07:40:54 bnv Exp $
 * $Log: mult.c,v $
 * Revision 1.4  2008/07/15 07:40:54  bnv
 * #include changed from <> to ""
 *
 * Revision 1.3  2002/06/11 12:37:15  bnv
 * Added: CDECL
 *
 * Revision 1.2  2001/06/25 18:49:48  bnv
 * Header changed to Id
 *
 * Revision 1.1  1998/07/02 17:18:00  bnv
 * Initial Version
 *
 */

#include "lstring.h"
#if ALLOW_DECNUMBER
  #include "context.h"
  #include "dnNumber.h"
  #include "options.h"
#endif

/* ------------------- Lmult ----------------- */
void __CDECL
Lmult(const PLstr to, const PLstr A, const PLstr B) {
#if ALLOW_DECNUMBER
    decContext *dc;
#endif
    L2NUM(A);
    L2NUM(B);

#if ALLOW_DECNUMBER
    Context *context = (Context *) CMSGetPG();
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
        decContextZeroStatus(*dc);
        decNumberMultiply(LDEC(*to), TODEC(*A), TODEC(*B), *cp);
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
        LTYPE(*to) = LDECIMAL_TY;
        LLEN(*to) = LMAXLEN(*to) = LDEC_LEN(*to);
        return;
    }
#endif
    if ((LTYPE(*A) == LINTEGER_TY) && (LTYPE(*B) == LINTEGER_TY)
         && (LINT(*A) < 46340) && (LINT(*B) < 46340)) {
        LINT(*to) = LINT(*A) * LINT(*B);
        LTYPE(*to) = LINTEGER_TY;
        LLEN(*to) = sizeof(long);
    } else {
        LREAL(*to) = TOREAL(*A) * TOREAL(*B);
        LTYPE(*to) = LREAL_TY;
        LLEN(*to) = sizeof(double);
    }
} /* Lmult */
