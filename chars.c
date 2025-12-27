/* VM/370 CMS and GCCLIB chars.c */

#include <stdio.h>
#include "lstring.h"

/* ---------------- Lchars ------------------- */
long Lchars(FILEP f) {
    long l;
    l = ftell(f);  /* read current position */
    if (l == -1) {
        /* File does not support character counting */
        return !fateof(f);
    }
	if (l < 0) {
		/* Workaround for GCCLIB bug #58.
		 * See https://github.com/adesutherland/CMS-370-GCCLIB/issues/58.
		 * Delete this when the bug is fixed and the fix is incorporated into
		 * VM/370 Community Edition.
		 */
		l = 0;
    }
    return fgetlen(f) - l;
} /* Lchars */
