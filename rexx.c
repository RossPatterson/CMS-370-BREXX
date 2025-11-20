/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: rexx.c,v 1.12 2009/06/30 13:51:40 bnv Exp bnv $
 * $Log: rexx.c,v $
 * Revision 1.12  2009/06/30 13:51:40  bnv
 * Added -a option to break arg into words
 *
 * Revision 1.11  2008/07/15 14:57:55  bnv
 * mvs corrections
 *
 * Revision 1.10  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.9  2008/07/14 13:08:42  bnv
 * MVS,CMS support
 *
 * Revision 1.8  2006/01/26 10:25:52  bnv
 * Windows CE
 *
 * Revision 1.7  2004/08/16 15:28:54  bnv
 * Changed: name of mnemonic operands from xxx_mn to O_XXX
 *
 * Revision 1.6  2003/10/30 13:16:28  bnv
 * Variable name change
 *
 * Revision 1.5  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.4  2002/06/06 08:24:36  bnv
 * Corrected: Registry support for WCE
 *
 * Revision 1.3  2001/06/25 18:51:48  bnv
 * Header -> Id
 *
 * Revision 1.2  1999/11/26 13:13:47  bnv
 * Changed: To use the new macros.
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

#define __REXX_C__

#include <string.h>
#include <setjmp.h>

#include "lerror.h"
#include "lstring.h"

#include "rexx.h"
#include "stack.h"
#include "trace.h"
#include "bintree.h"
#include "compile.h"
#include "interpre.h"
#include "nextsymb.h"
#include "context.h"

#if defined(UNIX)

# include <dlfcn.h>

#elif defined(WCE)
# include <winfunc.h>
#endif

#ifdef __CMS__

#include <cmssys.h>               // rpo
#include <cmsfblk.h>

#endif

/* ----------- Function prototypes ------------ */
void __CDECL Rerror(const int, const int, ...);

void __CDECL RxRegFunctionDone(void);

void __CDECL disasm();   /* in disasm.c */

/* ---------------- RxInitProc ---------------- */
static void
RxInitProc(void) {
    Context *context = (Context *) CMSGetPG();
    (context->rexx_rx_proc) = -1;
    (context->rexx_proc_size) = PROC_INC;
    (context->rexx_proc) = (RxProc *) MALLOC(
            (context->rexx_proc_size) * sizeof(RxProc), "RxProc");
    MEMSET((context->rexx_proc), 0, (context->rexx_proc_size) * sizeof(RxProc));
} /* RxInitProc */

/* ----------------- RxInitialize ----------------- */
void __CDECL
RxInitialize(char *prorgram_name) {
    Lstr str;
    Context *context = (Context *) CMSGetPG();

    (context->rexx_prgname) = prorgram_name;

    LINITSTR(str);

#ifdef __CMS__
    // We repeat the initialization of static variables in lstring.h to here because when BREXX is
    // loaded as a resident program in CMS the static initializatino is not refreshed at each
    // invocation.
    (context->lstring_lLastScannedNumber) = 0.0;
    (context->lstring_lNumericDigits) = 9;
#endif

    /* do the basic initialisation */
    Linit(Rerror);  /* initialise with Rerror as error function */
    LINITSTR((context->nextsymbsymbolstr));
    Lfx(&(context->nextsymbsymbolstr),
        250); /* create (context->nextsymbsymbol) string */
    LINITSTR((context->error_errmsg));
    Lfx(&(context->error_errmsg), 250); /* create error message string */

    LINITSTR((context->rexxrxReturnResult));  /* Return Result */

    /* --- first locate configuration file --- */
    /* rexx.rc for DOS in the executable program directory */
    /* .rexxrc for unix in the HOME directory */

    (context->rexx_procidcnt) = 1;  /* Program id counter */

#ifndef __CMS__
    DQINIT((context->rexxrxStackList)); /* initialise stacks */
    CreateStack();  /* create first stack */
#endif
    (context->rexxrxFileList) = NULL; /* initialise rexx files */
    LPMALLOC((context->rexx_code));
    (context->compileCompileClause) = NULL;

    RxInitProc();  /* initialize prg list */
    RxInitInterpret(); /* initialise interpreter*/
    RxInitVariables(); /* initialise hash table for variables */

    BINTREEINIT((context->rexx_labels)); /* initialise labels */
    BINTREEINIT((context->rexxrxLitterals)); /* initialise literals */

    Lscpy(&str, "HALT");
    (context->rexxhaltStr) = _Add2Lits(&str, FALSE);

    Lscpy(&str, "1");
    (context->rexxoneStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "");
    (context->rexxnullStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "0");
    (context->rexxzeroStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "ERROR");
    (context->rexxerrorStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "FAILURE");
    (context->rexxfailureStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "RESULT");
    (context->rexxresultStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "NOVALUE");
    (context->rexxnoValueStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "NOTREADY");
    (context->rexxnotReadyStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "SIGL");
    (context->rexxsiglStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "RC");
    (context->rexxRCStr) = _Add2Lits(&str, FALSE);
    Lscpy(&str, "SYNTAX");
    (context->rexxsyntaxStr) = _Add2Lits(&str, FALSE);

#ifdef __CMS__
    /* CMS sets the default ADDRESS env as follows:
        1) If there is an FBLOCK with a non-blank and non-null environment name,
           use it for the name.  If its prefix is a PSW, use it for host
           commands, otherwise use the environment name.
        2) If there is an FBLOCK with a non-blank and non-null prefix, use it.
        3) If there is an FBLOCK with a non-blank and non-null filetype, use it.
        4) Use "CMS".
    */

#define IS_BLANK(field) (!memcmp((field), "        ", 8))
#define IS_ZEROES(field) (!memcmp((field), "\0\0\0\0\0\0\0\0", 8))
/* This is the definition per the IBM Rexx Reference manual: */
#define IS_PSW(field) ((field)[3] == 0x00)

    char *cp;
    char env_name[9];
    char env_prefix[9];
    memset(env_name, ' ', sizeof env_name);
    memset(env_prefix, ' ', sizeof env_prefix);
    EPLIST *EPlist = CMSeplist();
    FBLOCK *FBlock = (EPlist && EPlist->CallContext) ? (FBLOCK *) (EPlist->CallContext) : NULL;
    if (FBlock && FBlock->lenwords >= 6 && !IS_BLANK(FBlock->env_name) && !IS_ZEROES(FBlock->env_name)) {
        memcpy(&env_name, FBlock->env_name, 8);
        if (!IS_ZEROES(FBlock->prefix) && IS_PSW(FBlock->prefix))
            memcpy(&env_prefix, FBlock->prefix, 8);
        else
            memcpy(&env_prefix, env_name, 8);
    } else if (FBlock && FBlock->lenwords >= 4 && !IS_BLANK(FBlock->prefix) && !IS_ZEROES(FBlock->prefix)) {
        memcpy(&env_prefix, FBlock->prefix, 8);
        memcpy(&env_name, env_prefix, 8);
    } else if (FBlock && !IS_BLANK(FBlock->filetype) && !IS_ZEROES(FBlock->filetype) &&
            (memcmp(FBlock->filetype, "EXEC    ", 8) != 0)) {
        memcpy(&env_name, FBlock->filetype, 8);
        memcpy(&env_prefix, env_name, 8);
    } else {
        memcpy(&env_name, "CMS     ", 8);
        memcpy(&env_prefix, env_name, 8);
    }
    LPMALLOC(context->rexxsystemPrefix);
    if (IS_PSW(env_prefix)) {
        Lmcpy(context->rexxsystemPrefix, env_prefix, 8);
    } else {
        if ((cp = memchr(env_prefix, ' ', 8)) != NULL) *cp = '\0';
        env_prefix[8] = '\0';
        Lscpy(context->rexxsystemPrefix, env_prefix);
    }
    if (IS_PSW(env_name)) {
        Lmcpy(&str, env_name, 8);
    } else {
        if ((cp = memchr(env_name, ' ', 8)) != NULL) *cp = '\0';
        env_name[8] = '\0';
        Lscpy(&str, env_name);
    }
#undef IS_BLANK
#undef IS_ZEROES
#undef IS_PSW
#else
    Lscpy(&str, "SYSTEM");
#endif
    (context->rexxsystemStr) = _Add2Lits(&str, FALSE);

    LFREESTR(str);

} /* RxInitialize */

/* ----------------- RxFinalize ----------------- */
void __CDECL
RxFinalize(void) {
    Context *context = (Context *) CMSGetPG();
    LFREESTR(
            (context->nextsymbsymbolstr)); /* delete (context->nextsymbsymbol) string */
    LFREESTR((context->error_errmsg)); /* delete error msg str */
    LFREESTR((context->rexxrxReturnResult));  /* Return Result */
    RxDoneInterpret();
    FREE((context->rexx_proc));  /* free prg list */
#ifndef __CMS__
    while ((context->rexxrxStackList).items > 0) DeleteStack();
#endif
    LPFREE((context->rexx_code));
    (context->rexx_code) = NULL;

    /* will free also (context->rexxnullStr), (context->rexxzeroStr) and (context->rexxoneStr) */
    BinDisposeLeaf(&(context->rexxrxLitterals),
                   (context->rexxrxLitterals).parent, FREE);
    BinDisposeLeaf(&(context->rexx_labels), (context->rexx_labels).parent,
                   FREE);
    RxDoneVariables();
    RxRegFunctionDone(); /* initialise register functions */
} /* RxFinalize */

/* ----------------- RxFileAlloc ------------------- */
RxFile *__CDECL
RxFileAlloc(char *fname) {
    RxFile *rxf;

    rxf = (RxFile *) MALLOC(sizeof(RxFile), "RxFile");
    if (rxf == NULL)
        return rxf;
    MEMSET(rxf, 0, sizeof(RxFile));
    Lscpy(&(rxf->name), fname);
    LASCIIZ(rxf->name);

    return rxf;
} /* RxFileAlloc */

/* ----------------- RxFileType ------------------- */
void __CDECL
RxFileType(RxFile *rxf) {
#ifdef CMS
    char *c1, *c2;
    int len;
#else
    char *c;
#endif

#ifdef CMS
    /* Extract filename and filetype from "fn.ft.fm" */
    rxf->filename = MALLOC(9, "RxFile filename");
    strcpy(rxf->filename, "?");
    rxf->filetype = MALLOC(9, "RxFile filetype");
    strcpy(rxf->filetype, "?");
    L2STR(&(rxf->name));
    LASCIIZ(rxf->name);
    if ((c1 = strchr(LSTR(rxf->name), '.')) != NULL) {
        memset(rxf->filename, 0, 9);
        len = c1 - LSTR(rxf->name);
        len = len > 8 ? 8 : len;
        strncpy(rxf->filename, LSTR(rxf->name), len);
        if ((c2 = strchr(++c1, '.')) != NULL) {
            memset(rxf->filetype, 0, 9);
            len = c2 - c1;
            len = len > 8 ? 8 : len;
            strncpy(rxf->filetype, c1, len);
        }
    }
#else
    /* find file type */
    c = LSTR(rxf->name) + LLEN(rxf->name);
    for (; c > LSTR(rxf->name) && *c != '.'; c--);;
    if (*c == '.')
        rxf->filetype = c;
    for (; c > LSTR(rxf->name) && *c != FILESEP; c--);;
    if (c > LSTR(rxf->name))
        c++;
    rxf->filename = c;
#endif
} /* RxFileType */

/* ----------------- RxFileFree ------------------- */
void __CDECL
RxFileFree(RxFile *rxf) {
    RxFile *f;

    while (rxf) {
        f = rxf;
        rxf = rxf->next;
        LFREESTR(f->name);
        LFREESTR(f->file);
#ifdef CMS
        if (f->filename) FREE(f->filename);
        if (f->filetype) FREE(f->filetype);
#endif
        FREE(f);
    }
} /* RxFileFree */

/* ----------------- RxFileLoad ------------------- */
int __CDECL
RxFileLoad(RxFile *rxf) {
    FILEP f;
    char filename[21];
    char *cp;

    /* Convert to ASCIIZ */
    L2STR(&(rxf->name));
    LASCIIZ(rxf->name);

    /* "fn.ft.fm" -> "fn ft fm" */
    memset(filename, ' ', sizeof(filename));
    strcpy(filename, LSTR(rxf->name));
    if ((cp = strchr(filename, '.')) == NULL) return FALSE;
    *cp = ' ';
    if ((cp = strchr(cp+1, '.')) == NULL) return FALSE;
    *cp = ' ';
    if (strchr(cp+1, '.') != NULL) return FALSE;

    if ((f = FOPEN(filename, "r")) == NULL) return FALSE;
    Lread(f, &(rxf->file), LREADFILE);
    FCLOSE(f);

    return TRUE;
} /* RxFileLoad */

/* --- _LoadRexxLibrary --- */
static int
_LoadRexxLibrary(RxFile *rxf, PLstr libname) {
    size_t ip;
    Context *context = (Context *) CMSGetPG();

    if (!RxFileLoad(rxf)) return 1;

    ip = (size_t) ((byte huge *) (context->interpreRxcip) -
                   (byte huge *) (context->interpreRxcodestart));
    MEMCPY((context->rexx_old_trap), (context->rexx_error_trap),
           sizeof((context->rexx_error_trap)));
    RxFileType(rxf);
    RxInitCompile(rxf, NULL);
    RxCompile();

    /* restore state */
    MEMCPY((context->rexx_error_trap), (context->rexx_old_trap),
           sizeof((context->rexx_error_trap)));
    (context->interpreRxcodestart) = (CIPTYPE *) LSTR(*(context->rexx_code));
    (context->interpreRxcip) = (CIPTYPE *) (
            (byte huge *) (context->interpreRxcodestart) + ip);

    if ((context->rexxrxReturnCode)) RxSignalCondition(SC_SYNTAX, "");
    return 0;
} /* _LoadRexxLibrary */

/* ----------------- RxLoadLibrary ------------------- */
int __CDECL
RxLoadLibrary(PLstr libname) {
    RxFile *rxf, *last;
    Lstr name;
    Context *context = (Context *) CMSGetPG();

    LINITSTR(name);
    Lfx(&name, LLEN(*libname));
    Lstrip(&name, libname, LBOTH, ' ');
    Lupper(&name);

    /* Convert to C String */
    L2STR(&name);
    LASCIIZ(name);

    /* check to see if it is already loaded */
    for (rxf = (context->rexxrxFileList); rxf != NULL; rxf = rxf->next)
        if (!strcmp(rxf->filename, LSTR(name)))
            return -1;

    /* create a RxFile structure */
    rxf = RxFileAlloc(LSTR(name));

    /* try to load the file as rexx library */
    if (_LoadRexxLibrary(rxf, &name)) {
        RxFileFree(rxf);
        LFREESTR(name);
        return 1;
    }

    /* find the last in the queue */
    for (last = (context->rexxrxFileList); last->next != NULL;)
        last = last->next;
    last->next = rxf;

    LFREESTR(name);

    return 0;
} /* RxLoadLibrary */

/* ----------------- RxRun ------------------ */
int __CDECL
RxRun(char *filename, PLstr programstr,
      PLstr arguments, PLstr tracestr, char *environment) {
    RxProc *pr;
    int i;
    Context *context = (Context *) CMSGetPG();

    /* --- set exit jmp position --- */
    if ((i = setjmp((context->rexx_exit_trap))) != 0)
        goto run_exit;
    /* --- set temporary error trap --- */
    if (setjmp((context->rexx_error_trap)) != 0)
        return (context->rexxrxReturnCode);

    /* ====== first load the file ====== */
#ifdef __CMS__
    /* CMS supports the "FBLOCK" interface, which is surfaced here as the CallContext struct. */
    EPLIST *EPlist = CMSeplist();
    FBLOCK *FBlock = (EPlist && EPlist->CallContext) ? (FBLOCK *) (EPlist->CallContext) : NULL;

    /* CMS allows the caller to optionally specify the full fileid of the program. */
    char new_filename[21+1];
    memset(new_filename, 0, sizeof(new_filename));
    if (FBlock && FBlock->filename[0] != ' '&& FBlock->filename[0] != '\0') {
        char *cp;
        strncpy(new_filename, FBlock->filename, 8);
        if ((cp = strchr(new_filename, ' ')) != NULL) *cp = '\0';
        strcat(new_filename, ".");
        strncat(new_filename, FBlock->filetype, 8);
        if ((cp = strchr(new_filename, ' ')) != NULL) *cp = '\0';
        strcat(new_filename, ".");
        if (FBlock->filemode[0] != ' ') {
            strncat(new_filename, FBlock->filemode, 1);
            if ((cp = strchr(new_filename, ' ')) != NULL) *cp = '\0';
            if (FBlock->filemode[1] != ' ') {
                strncat(new_filename, FBlock->filemode+1, 1);
                if ((cp = strchr(new_filename, ' ')) != NULL) *cp = '\0';
            }
        }
        else {
            strcat(new_filename, "*");
        }
    } else {
        /* If no FBLOCK info, then filetype is "EXEC" and filemode is "*" */
        strcpy(new_filename, filename);
        strcat(new_filename, ".EXEC.*");
    }
    filename = new_filename;

    /* CMS allows the caller to optionally preload the file in memory. */
    if (FBlock && FBlock->lenwords >= 2 && FBlock->descriptors && FBlock->descriptor_len) {
        (context->rexxrxFileList) = RxFileAlloc(filename);
        ADLEN *line;
        ADLEN *lastline = FBlock->descriptors + (FBlock->descriptor_len / sizeof (ADLEN));
        /* Compile() expects it in a single LSTR, so we have to copy it :-( */
        size_t filesize = 0;
        for (line = FBlock->descriptors; line < lastline; line++) {
            filesize += line->Len + 1;
        }
        Lfx(&((context->rexxrxFileList)->file), filesize);
        char *nextline = LSTR((context->rexxrxFileList)->file);
        for (line = FBlock->descriptors; line < lastline; line++) {
            memcpy(nextline, line->Data, line->Len);
            nextline += line->Len;
            *nextline = '\n';
            nextline += 1;
        }
        LLEN((context->rexxrxFileList)->file) = filesize;
        LTYPE((context->rexxrxFileList)->file) = LSTRING_TY;
    }
#endif
    if (filename) {
        if (!(context->rexxrxFileList)) {
            (context->rexxrxFileList) = RxFileAlloc(filename);

            /* --- Load file --- */
            if (!RxFileLoad((context->rexxrxFileList))) {
#ifndef WCE
                fprintf(STDERR, "Error %d running \"%s\": File not found\n",
                        ERR_FILE_NOT_FOUND, LSTR((context->rexxrxFileList)->name));
#else
                PUTS("Error: File not found.");
#endif
                RxFileFree((context->rexxrxFileList));
                return 1;
            }
        }
    } else {
        (context->rexxrxFileList) = RxFileAlloc("<STDIN>");
        Lfx(&((context->rexxrxFileList)->file), LLEN(*programstr));
        Lstrcpy(&((context->rexxrxFileList)->file), programstr);
    }
    RxFileType((context->rexxrxFileList));
    LASCIIZ((context->rexxrxFileList)->file);

#ifdef __DEBUG__
    if ((context->rexx__debug__)) {
     printf("File is:\n%s\n",LSTR((context->rexxrxFileList)->file));
     getchar();
    }
#endif

    /* ====== setup procedure ====== */
    (context->rexx_rx_proc)++;  /* increase program items */
    pr = (context->rexx_proc) +
         (context->rexx_rx_proc); /* pr = Proc pointer  */

    /* set program id counter */
    pr->id = (context->rexx_procidcnt)++;

    /* --- initialise Proc structure --- */
    /* arguments...  */
    pr->arg.n = 0;
    for (i = 0; i < MAXARGS; i++) {
        if (LLEN(arguments[i])) {
            pr->arg.n = i + 1;
            pr->arg.a[i] = &(arguments[i]);
        } else
            pr->arg.a[i] = NULL;
    }
    pr->arg.r = NULL;

    /* call type...  */
    if (CMScalltype() == 5) {
        if (CMSisproc()) pr->calltype = CT_PROCEDURE;
        else pr->calltype = CT_FUNCTION;
    } else pr->calltype = CT_PROGRAM;

    pr->ip = 0;   /* procedure ip  */
    pr->stack = -1;  /* prg stck, will be set in interpret */
    pr->stacktop = -1;  /* no arguments  */

    pr->scope = RxScopeMalloc();
#ifdef __CMS__
    pr->env = MALLOC(sizeof(CmsEnv), "RxProcEnv");
    LPMALLOC(pr->env->name);
    LPMALLOC(pr->env->prefix);
    pr->env_alt = MALLOC(sizeof(CmsEnv), "RxProcEnv");
    LPMALLOC(pr->env_alt->name);
    LPMALLOC(pr->env_alt->prefix);
#else
    LPMALLOC(pr->env);
    LPMALLOC(pr->env_alt);
#endif
    if (environment) {
#ifdef __CMS__
        Lscpy(pr->env->name, environment);
        Lscpy(pr->env->prefix, environment);
#else
        Lscpy(pr->env, environment);
#endif
    } else {
#ifdef __CMS__
        Lstrcpy(pr->env->name, &((context->rexxsystemStr)->key));
        Lstrcpy(pr->env->prefix, (context->rexxsystemPrefix));
#else
        Lstrcpy(pr->env, &((context->rexxsystemStr)->key));
#endif
    }
#ifdef __CMS__
    Lstrcpy(pr->env_alt->name, pr->env->name);
    Lstrcpy(pr->env_alt->prefix, pr->env->prefix);
#else
    Lstrcpy(pr->env_alt, pr->env);
#endif
#ifdef __CMS__
    pr->digits = 9;
#else
    pr->digits = LMAXNUMERICDIGITS;
#endif
    pr->fuzz = 0;
    pr->form = SCIENTIFIC;
    pr->condition = 0;
    pr->lbl_error = &((context->rexxerrorStr)->key);
    pr->lbl_halt = &((context->rexxhaltStr)->key);
    pr->lbl_novalue = &((context->rexxnoValueStr)->key);
    pr->lbl_notready = &((context->rexxnotReadyStr)->key);
    pr->lbl_syntax = &((context->rexxsyntaxStr)->key);
    pr->codelen = 0;
    pr->trace = normal_trace;
    pr->interactive_trace = FALSE;
    if (tracestr && LLEN(*tracestr)) TraceSet(tracestr);

    /* ======= Compile file ====== */
    RxInitCompile((context->rexxrxFileList), NULL);
    RxCompile();

#ifdef __DEBUG__
    if ((context->rexx__debug__)) {
        printf("Literals are:\n");
        BinPrint((context->rexxrxLitterals).parent);
        getchar();

        printf("Labels(&functions) are:\n");
        BinPrint((context->rexx_labels).parent);
        printf("Code Size: %d\n\n",LLEN(*(context->rexx_code)));
        getchar();

        printf("Compiled code is:\n");
        disasm();
        getchar();
    }
#endif

    /* ======= Execute code ======== */
    if (!(context->rexxrxReturnCode))
        RxInterpret();

    run_exit:
    /* pr pointer might have changed if Proc was resized */
    pr = (context->rexx_proc) + (context->rexx_rx_proc);
#ifdef __DEBUG__
    if ((context->rexx__debug__))
     printf("Return Code = %d\n",(context->rexxrxReturnCode));
#endif

    /* ======== close files and free up memory ======== */
    RxFileFree((context->rexxrxFileList));
    if (context->rawstdin) fclose(context->rawstdin);

#ifdef CMS
    LPFREE(pr->env->name);
    LPFREE(pr->env->prefix);
    FREE(pr->env);
#else
    LPFREE(pr->env));
#endif
    if ((context->compileCompileClause)) {
        FREE((context->compileCompileClause));
        (context->compileCompileClause) = NULL;
    }

    RxScopeFree(pr->scope);
    FREE(pr->scope);
    (context->rexx_rx_proc)--;

    return (context->rexxrxReturnCode);
} /* RxRun */
