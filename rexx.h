/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: rexx.h,v 2.3 2008/07/15 07:40:07 bnv Exp $
 * $Log: rexx.h,v $
 * Revision 2.3  2008/07/15 07:40:07  bnv
 * MVS, CMS support
 *
 * Revision 2.2  2004/08/16 15:33:00  bnv
 * Changed: name of mnemonic operands from xxx_mn to O_XXX
 *
 * Revision 2.1  2003/02/26 16:33:09  bnv
 * Version 2.1
 *
 * Revision 1.11  2003/02/12 16:39:17  bnv
 * 2.0.8
 *
 * Revision 1.10  2002/08/19 15:39:09  bnv
 * Corrected: Version string
 *
 * Revision 1.9  2002/07/03 13:14:17  bnv
 * Changed: Version string
 *
 * Revision 1.8  2002/06/11 12:37:56  bnv
 * Added: CDECL
 *
 * Revision 1.7  2002/06/06 08:23:11  bnv
 * New Version
 *
 * Revision 1.6  2001/06/25 18:52:04  bnv
 * Header -> Id
 *
 * Revision 1.5  2000/04/07 07:17:10  bnv
 * Release 2.0.3
 *
 * Revision 1.4  1999/11/29 14:58:00  bnv
 * Changed: Some defines
 *
 * Revision 1.3  1999/05/14 13:08:00  bnv
 * Release 2.0.1
 *
 * Revision 1.2  1999/03/10 16:58:05  bnv
 * New release 2.0
 *
 * Revision 1.1  1998/07/02 17:35:50  bnv
 * Initial revision
 *
 */

/*
 * Defined symbols in makefile
 * __DEBUG__ enable debuging
 * MSDOS  MSDOS compilation routines
 * __BORLANDC__ to enable PORT,INTR,STORAGE()
 * ALIGN  to enable DWORD align instead of byte
 * INLINE  to inline some functions
 */

#include "lerror.h"
#include "lstring.h"

#include "dqueue.h"
#include "bintree.h"
#include "variable.h"

#ifndef __REXX_H_
#define __REXX_H_

#include <setjmp.h>

/* ------------ some defines ------------------ */
#define VERSIONSTR PACKAGE_STRING" "__DATE__
#define AUTHOR  "Vasilis.Vlachoudis@cern.ch"
#define REGAPPKEY TEXT("Software\\Marmita\\BRexx")
#define SCIENTIFIC 0
#define ENGINEERING 1

#define MAXARGS  15
#define PROC_INC 10
#define CLAUSE_INC 100
#define CODE_INC 256
#define STCK_SIZE 255

/* call types */
#define CT_PROGRAM 0
#define CT_PROCEDURE 1
#define CT_FUNCTION 2
#define CT_INTERPRET 3
#define CT_INTERACTIVE 4

/* signal on condition */
#define SC_ERROR 0x01
#define SC_HALT  0x02
#define SC_NOVALUE 0x04
#define SC_NOTREADY 0x08
#define SC_SYNTAX 0x10
#define SC_FAILURE 0x20

/* long jmp values */
#define JMP_CONTINUE 2
#define JMP_ERROR 98
#define JMP_EXIT 99

/* rexx variables */
#define RCVAR  0
#define SIGLVAR  1

#ifdef ALIGN
# define CTYPE dword
#else
# define CTYPE word
#endif

/* ----------------- file structure --------------- */
typedef
struct trxfile {
    Lstr name;  /* complete file path */
    char *filename; /* filename in name */
    char *filetype; /* filetype in name */
    void *libHandle; /* Shared library handle*/
    Lstr file;  /* actual file  */
    struct trxfile *next; /* prev in list  */
} RxFile;

/* ------------- clause structure ----------------- */
typedef
struct tclause {
    size_t code;  /* code start position */
    size_t line;  /* line number in file */
    int nesting; /* nesting level */
    char *ptr;  /* pointer in file */
    RxFile *fptr;  /* RxFile pointer */
} Clause;

/* ----------------- ident info ------------------- */
typedef
struct tidentinfo {
    int id;  /* the last prg that set leaf value */
    int stem;  /* if it is a stem   */
    PBinLeaf leaf[1]; /* Variable array of leafs  */
    /* Variable value if stem=0 OR  */
    /* pointers to litterals  */
} IdentInfo;

/* ------------ argument structure ---------------- */
typedef
struct targs {
    int n;  /* number of args */
    PLstr r;  /* return data  */
    PLstr a[MAXARGS]; /* argument pointers */
} Args;

/* ------------ internal rexxfunctions ------------ */
typedef
struct tbltfunc {
    char *name;

    void (__CDECL *func)(int);

    int opt;
} TBltFunc;

/* ----------- proc data structure ---------------- */
typedef
struct trxproc {
    int id;  /* procedure id  */
    int calltype; /* call type...  */
    size_t ip;  /* instruction pointer */
    size_t stack;  /* stack position */
    size_t stacktop; /* stack after args */
    Scope scope;  /* Variables  */
    Args arg;  /* stck pos of args */
    PLstr env;  /* environment  */
    int digits;  /* numeric digits */
    int fuzz;  /* numeric fuzz  */
    int form;  /* numeric form  */
    int condition; /* signal on condition */
    PLstr lbl_error; /* labels  */
    PLstr lbl_failure; /*   */
    PLstr lbl_halt; /*   */
    PLstr lbl_novalue; /*   */
    PLstr lbl_notready; /*   */
    PLstr lbl_syntax; /*   */
    int codelen; /* used in OP_INTERPRET */
    int clauselen; /* used in OP_INTERPRET */
    int trace;  /* trace type  */
    bool interactive_trace;
} RxProc;

/* ------------- global variables ----------------- */
#include "context.h"

/* ============= function prototypes ============== */
#ifdef __cplusplus
extern "C" {
#endif

void __CDECL RxInitialize(char *program_name);

void __CDECL RxFinalize(void);

RxFile *__CDECL RxFileAlloc(char *fname);

void __CDECL RxFileFree(RxFile *rxf);

void __CDECL RxFileType(RxFile *rxf);

int __CDECL RxFileLoad(RxFile *rxf);

int __CDECL RxLoadLibrary(PLstr libname);

int __CDECL RxRun(char *filename, PLstr programstr,
                  PLstr arguments, PLstr tracestr, char *environment);

int __CDECL RxRegFunction(char *name, void (__CDECL *func)(int), int opt);

void __CDECL RxHaltTrap(int);

void __CDECL RxSignalCondition(int);

#if !defined (__CMS__) && !defined(__MVS__)

int __CDECL RxRedirectCmd(PLstr cmd, int in, int out, PLstr resultstr);

#endif

int __CDECL RxExecuteCmd(PLstr cmd, PLstr env);

#ifdef __cplusplus
}
#endif

#endif
