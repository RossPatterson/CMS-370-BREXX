/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: trunc.c,v 1.6 2008/07/15 07:40:54 bnv Exp $
 * $Log: trunc.c,v $
 * Revision 1.6  2008/07/15 07:40:54  bnv
 * #include changed from <> to ""
 *
 * Revision 1.5  2008/07/14 13:08:16  bnv
 * MVS,CMS support
 *
 * Revision 1.4  2002/06/11 12:37:15  bnv
 * Added: CDECL
 *
 * Revision 1.3  2001/06/25 18:49:48  bnv
 * Header changed to Id
 *
 * Revision 1.2  1999/11/26 12:52:25  bnv
 * Changed: To use the fcvt()
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

/* ---------------- Ltrunc ----------------- */
void __CDECL
Ltrunc(const PLstr to, const PLstr from, long n) {
#if ALLOW_DECNUMBER
    decContext *dc;
#endif

#if ALLOW_DECNUMBER
    Context *context = (Context *) CMSGetPG();
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
        decContextZeroStatus(*dc);
		???
#error Not ready!
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
        LTYPE(*to) = LDECIMAL_TY;
        LLEN(*to) = LMAXLEN(*to) = LDEC_LEN(*to);
        return;
    }
#endif
    if (n < 0) n = 0;

    Lround(from);

    if (!n) {
        Lstrcpy(to, from);
        L2REAL(to);
        LINT(*to) = (long) LREAL(*to);
        LTYPE(*to) = LINTEGER_TY;
        LLEN(*to) = sizeof(long);
    } else {
        L2REAL(from);
        Lfx(to, n + 16);
        sprintf(LSTR(*to),"%.*f", (int)n+1, LREAL(*from));
        LTYPE(*to) = LSTRING_TY;
        n = STRLEN(LSTR(*to)) - 1;
        LSTR(*to)[n] = 0;
        LLEN(*to) = n;
    }
} /* R_trunc */
