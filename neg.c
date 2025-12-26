/*
 * $Id: neg.c,v 1.4 2008/07/15 07:40:54 bnv Exp $
 * $Log: neg.c,v $
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

/* ------------------- Lneg ------------------ */
void __CDECL
Lneg(const PLstr to, const PLstr num) {
#if ALLOW_DECNUMBER
    decContext *dc;
#endif

#if ALLOW_DECNUMBER
    Context *context = (Context *) CMSGetPG();
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
        decContextZeroStatus(*dc);
        decNumberMinus(LDEC(*to), TODEC(*num), *dc);
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
        LTYPE(*to) = LDECIMAL_TY;
        LLEN(*to) = LMAXLEN(*to) = LDEC_LEN(*to);
        return;
    }
#endif
		L2NUM(num);

    if (LTYPE(*num) == LINTEGER_TY) {
        LINT(*to) = -LINT(*num);
        LTYPE(*to) = LINTEGER_TY;
        LLEN(*to) = sizeof(long);
    } else {
        LREAL(*to) = -LREAL(*num);
        LTYPE(*to) = LREAL_TY;
        LLEN(*to) = sizeof(double);
    }
} /* Lneg */

/* ------------------- Lplus ----------------- */
void __CDECL
Lplus(const PLstr to, const PLstr num) {
#if ALLOW_DECNUMBER
    decContext *dc;
#endif

#if ALLOW_DECNUMBER
    Context *context = (Context *) CMSGetPG();
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
        decContextZeroStatus(*dc);
        decNumberPlus(LDEC(*to), TODEC(*num), *dc);
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
        LTYPE(*to) = LDECIMAL_TY;
        LLEN(*to) = LMAXLEN(*to) = LDEC_LEN(*to);
        return;
    }
#endif
    L2NUM(num);

    if (LTYPE(*num) == LINTEGER_TY) {
        LINT(*to) = LINT(*num);
        LTYPE(*to) = LINTEGER_TY;
        LLEN(*to) = sizeof(long);
    } else {
        LREAL(*to) = LREAL(*num);
        LTYPE(*to) = LREAL_TY;
        LLEN(*to) = sizeof(double);
    }
} /* Lplus */

