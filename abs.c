/*
 * $Id: abs.c,v 1.4 2008/07/15 07:40:54 bnv Exp $
 * $Log: abs.c,v $
 * Revision 1.4  2008/07/15 07:40:54  bnv
 * #include changed from <> to ""
 *
 * Revision 1.3  2002/06/11 12:37:15  bnv
 * Added: CDECL
 *
 * Revision 1.2  2001/06/25 18:49:48  bnv
 * Header changed to Id
 *
 * Revision 1.1  1998/07/02 17:16:35  bnv
 * Initial revision
 *
 */

#include <math.h>
#include "lstring.h"
#if ALLOW_DECNUMBER
  #include "context.h"
  #include "dnNumber.h"
  #include "options.h"
#endif

/* ------------------ Labs ---------------------- */
void __CDECL
Labs(const PLstr to, const PLstr num) {
#if ALLOW_DECNUMBER
    decContext *dc;
#endif
    L2NUM(num);

#if ALLOW_DECNUMBER
    Context *context = (Context *) CMSGetPG();
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
        decContextZeroStatus(*dc);
        decNumberAbs(LDEC(*to), TODEC(*num), *cp);
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
        LTYPE(*to) = LDECIMAL_TY;
        LLEN(*to) = LMAXLEN(*to) = LDEC_LEN(*to);
        return;
    }
#endif
    switch (LTYPE(*num)) {
        case LINTEGER_TY:
            Licpy(to, labs(LINT(*num)));
            break;
        case LREAL_TY:
            Lrcpy(to, fabs(LREAL(*num)));
            break;
    }
} /* Labs */
