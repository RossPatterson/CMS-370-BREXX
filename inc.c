/*
 * $Id: inc.c,v 1.4 2008/07/15 07:40:54 bnv Exp $
 * $Log: inc.c,v $
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

/* ------------------- Linc ------------------ */
void __CDECL
Linc(const PLstr num) {
#if ALLOW_DECNUMBER
    decContext *dc;
    decNumber one;
#endif

#if ALLOW_DECNUMBER
    Context *context = (Context *) CMSGetPG();
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
        decContextZeroStatus(*dc);
        one = decNumberFromInt32(1);
        decNumberAdd(LDEC(*num), TODEC(*num), &one, *cp);
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
        LLEN(*num) = LMAXLEN(*num) = LDEC_LEN(*num);
        return;
    }
#endif
    L2NUM(num);

    if (LTYPE(*num) == LINTEGER_TY)
        LINT(*num) += 1;
    else
        LREAL(*num) += 1.0;
} /* Linc */
