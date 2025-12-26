/*
 * C2D - VM/370 Version
 */

#include "lstring.h"
#if ALLOW_DECNUMBER
  #include "context.h"
  #include "dnNumber.h"
  #include "options.h"
#endif

/* ------------------- Lc2d ------------------------- */
void __CDECL
Lc2d(const PLstr to, const PLstr from, long n) {
    int i;
    bool negative;
    long num;
#if ALLOW_DECNUMBER
    decContext *dc;
#endif

    L2STR(from);

#if ALLOW_DECNUMBER
    Context *context = (Context *) CMSGetPG();
    if (CHECK_OPT(OPT_DECIMAL_MATH)) {
        *dc = (context->rexx_proc)[(context->rexx_rx_proc)].decContext;
#error Wrong!  C2D() is binary string to decimal number!
        decNumberFromString(LDEC(*to), LSTR(*from), *dc);
        if (decContextGetStatus(*dc))
            (context->lstring_Lerror)(ERR_INVALID_CHAR, 0);
        return;
    }
#endif
    if (!LLEN(*from) || !n) {
        Licpy(to, 0);
        return;
    }
    if (n < 1 || n > sizeof(long)) n = sizeof(long);

    Lstrcpy(to, from);
    Lreverse(to);

    if (n <= LLEN(*to))
        negative = LSTR(*to)[n - 1] & 0x80;  /* msb = 1 */
    else
        negative = FALSE;

    n = MIN(n, LLEN(*from));
    num = 0;
    for (i = n - 1; i >= 0; i--)
        num = (num << 8) | ((byte) (LSTR(*to)[i]) & 0xFF);
    if (negative) {
        if (n == sizeof(long))
            num = -(~num + 1);
        else
            num = num - (1L << (n * 8));
    }
    Licpy(to, num);
} /* Lc2d */
