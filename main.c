/*
 * main.c
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <cmssys.h>
#include <time.h>

#include "lstring.h"
#include "rexx.h"
#include "rxdefs.h"
#include "context.h"

/* Temporary aid to register entry point for HRC402DS           */
/* Will be removed once bREXX CMS deployment approach confirmed */
/* Entry point for REXX is stored @ 0x90 - NUCRSV6 */
#define REXX_ENTRY_HANDLE 0x90

void __CRT0(void); /* Entry Point pointer */

/* ------- Includes for any other external library ------- */
#ifdef RXCONIO
extern void __CDECL RxConIOInitialize();
#endif

#ifdef RXMYSQLSTATIC
# include "rxmysql.c"
#endif

/* Compare argument */
int issamearg(const char *arg, const char *val) {
    const char *c1 = arg;
    const char *c2 = val;
    while (*c1 == toupper(*c2)) {
        if (*c1 == 0) return 1;
        c1++;
        c2++;
    }
    return 0;
}

/* --------------------- main ---------------------- */
int __CDECL
main(int ac, char *av[]) {
    Lstr args[MAXARGS], tracestr;
    int ia, ir, i;
    int returnCode;
    int entry_point;
    Context *context;
    unsigned int initial_debug;  /* Initial debugging status, copied to/from CMS. */
    char prog_filename[8+1]; /* Program filename for CMS */
    char *cp;

    if (!ac) return -1; /* Should never happen! */

    initial_debug = ( ( (unsigned int) CMSGetNUCON((void *) REXX_ENTRY_HANDLE) >> 24) & 0x80) == 0x80;

    if (CMScalltype() != 5) {
        if (issamearg("DMSREX", av[0])) {
            int show_version_msg = 0;
            int show_debug_msg = 0;
            int arg_error = 0;

            if (ac <= 1) {
                ;
            } else if (ac > 3) {
                arg_error = 1;
            } else if (ac == 2 & issamearg("VERSION", av[1])) {
                show_version_msg = 1;
            } else if (ac >= 2 & issamearg("DEBUG", av[1])) {
                show_version_msg = 1;
                show_debug_msg = 1;
                if (ac == 3) {
                    if (issamearg("ON", av[2])) {
                        initial_debug = 1; /* Turn on bREXX debugging */
#ifndef __DEBUG__
                        printf("WARNING: bREXX not compiled with debug.\n");
#endif
                    } else if (issamearg("OFF", av[2])) {
                        initial_debug = 0; /* Turn off bREXX debugging */
                    } else arg_error = 1;
                }
            } else arg_error = 1;
            if (arg_error) {
                printf("Invalid arguments\n");
                printf("   DMSREX [VERSION | DEBUG [ON | OFF]]\n");
                return -1;
            }

            /* Register Entry Point Address */
            entry_point = ((int) __CRT0) & 0x00ffffff;
            if (initial_debug)
                entry_point = entry_point | 0x80000000;  /* Save initial_debug in high bit */
            CMSSetNUCON((void *) REXX_ENTRY_HANDLE, entry_point);

            if (show_version_msg) {
                printf("bREXX Version %s ", VERSIONSTR " "
                #ifndef __DEBUG__
                    "no"
                #endif
                    "debug\n");
            }
            if (show_debug_msg) {
                printf("bREXX Entry Address is 0x%x saved in NUCON at 0x%x, debugging is %s.\n",
                       entry_point, REXX_ENTRY_HANDLE, (initial_debug ? "active" : "inactive"));
            }
            return 0;
        }
    }

    InitContext();
    context = (Context *) CMSGetPG();
    if (initial_debug) {
       (context->rexx__debug__) = TRUE;    /* Turn on bREXX debugging */
        __SDEBUG(1);    /* Turn on GCCLIB debugging */
    } else {
       (context->rexx__debug__) = FALSE;    /* Turn off bREXX debugging */
        __SDEBUG(0);    /* Turn off GCCLIB debugging */
    }

    for (ia = 0; ia < MAXARGS; ia++)
        LINITSTR(args[ia]);
    LINITSTR(tracestr);

    /* Start Processing arguments */
    ia = (CMScalltype() == 5) ? 0 : 1;

    /* No program name */
    if (ia >= ac) {
        return 0;
    }

    /* --- Initialise --- */
    RxInitialize(av[ia]);

    /* Trace Flag */
    if (CMSGetFlag(TRACEFLAG)) Lscpy(&tracestr, "?A"); /* Trace All? */

    /* prepare arguments for program */
    if (CMScalltype() == 5) {
        ADLEN *param = CMSeplist()->ArgList;
        if (param) {
            for (i = 0; (int) (param[i].Data) != -1; i++) {
                Lmcpy(&args[i], param[i].Data, param[i].Len);
            }
        }
    } else {
        for (ir = ia + 1; ir < ac; ir++) {
            Lcat(&args[0], av[ir]);
            if (ir < ac - 1) Lcat(&args[0], " ");
        }
    }

    Lscpy(&(context->rexxrxReturnResult), "0");
    (context->rexxrxReturnCode) = 0;

    memset(prog_filename, '\0', sizeof prog_filename);
    memcpy(prog_filename, CMSplist()[1], 8);
    if ((cp = strchr(prog_filename, ' ')) != NULL) *cp = '\0';
    prog_filename[8] = '\0';
    RxRun(prog_filename, NULL, args, &tracestr, NULL);

    /* Need to get the result here before RxFinalise() */
    returnCode = (context->rexxrxReturnCode);
    L2STR(&(context->rexxrxReturnResult));
    LASCIIZ((context->rexxrxReturnResult)); /* Make sure its a C str */
    CMSreturndata(LSTR(context->rexxrxReturnResult),
                  LLEN(context->rexxrxReturnResult)); /* If calltype 5 */

    /* --- Free everything --- */
    RxFinalize();

    for (ia = 0; ia < MAXARGS; ia++)
        LFREESTR(args[ia]);
    LFREESTR(tracestr);

    if ((context->rexx__debug__) && mem_allocated()!=0) {
     fprintf(STDERR,"\nMemory left allocated: %ld\n",mem_allocated());
     mem_list();
    }

    return returnCode;
} /* main */
