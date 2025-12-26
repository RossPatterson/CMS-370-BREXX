/*
 * $Id: expose.c,v 1.5 2008/07/15 07:40:54 bnv Exp $
 * $Log: expose.c,v $
 * Revision 1.5  2008/07/15 07:40:54  bnv
 * #include changed from <> to ""
 *
 * Revision 1.4  2002/06/11 12:37:15  bnv
 * Added: CDECL
 *
 * Revision 1.3  2001/06/25 18:49:48  bnv
 * Header changed to Id
 *
 * Revision 1.2  1999/11/26 09:56:55  bnv
 * Changed: Formatting
 *
 * Revision 1.1  1998/07/02 17:18:00  bnv
 * Initial Version
 *
 */

#include <cmssys.h>
#include "lerror.h"
#include "lstring.h"

/* -----------------  expose ---------------- */
void __CDECL
Lexpose(const PLstr to, const PLstr A, const PLstr B) {
    double ar, r;
    long bi;
    bool minusA;
    bool minusB;
    Context *context = (Context *) CMSGetPG();
#if ALLOW_DECNUMBER
    decContext *dc;
#endif

#if ALLOW_DECNUMBER
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
        decContextZeroStatus(*dc);
        decNumberPower(LDEC(*to), TODEC(*A), TODEC(*B), *dc);
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
        LTYPE(*to) = LDECIMAL_TY;
        LLEN(*to) = LMAXLEN(*to) = LDEC_LEN(*to);
        return;
    }
#endif
    ar = Lrdreal(A);
    bi = Lrdint(B);

    if (ar < 0) {
        if (ODD(bi))
            minusA = TRUE;
        else
            minusA = FALSE;
        ar = -ar;
    } else
        minusA = FALSE;

    if (bi < 0) {
        minusB = TRUE;
        bi = -bi;
    } else
        minusB = FALSE;
    r = 1;

    while (bi != 0) {
        if (ODD(bi)) r *= ar;
        ar *= ar;
        bi /= 2;
    }
    if (minusA) r = -r;

    if (minusB) {
        if (r == 0) (context->lstring_Lerror)(ERR_ARITH_OVERFLOW, 0);
        LREAL(*to) = 1 / r;
    } else
        LREAL(*to) = r;

    LTYPE(*to) = LREAL_TY;
    LLEN(*to) = sizeof(double);
} /* Lexpose */
