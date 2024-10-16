/*
 * $Id: compile.c,v 1.16 2008/07/15 07:40:25 bnv Exp $
 * $Log: compile.c,v $
 * Revision 1.16  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.15  2008/07/14 13:08:42  bnv
 * MVS,CMS support
 *
 * Revision 1.14  2006/01/26 10:24:40  bnv
 * Added: Indirect exposure to variables
 *
 * Revision 1.13  2004/08/16 15:28:15  bnv
 * Changed: name of mnemonic operands from xxx_mn to O_XXX
 *
 * Revision 1.12  2004/04/30 15:30:01  bnv
 * Fixed: When tracing no clause was appearing after END
 *
 * Revision 1.11  2004/03/27 08:33:34  bnv
 * Corrected: If procedure was following on the next line after the label
 *
 * Revision 1.10  2003/10/30 13:16:10  bnv
 * Variable name change
 *
 * Revision 1.9  2002/08/22 12:25:41  bnv
 * Corrected: copy2tmp added before any call to C_template, to avoid errors like parse var a a b, that b is always null
 *
 * Revision 1.8  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.7  2001/06/25 18:51:48  bnv
 * Header -> Id
 *
 * Revision 1.6  1999/11/26 13:13:47  bnv
 * Changed: To use the new macros
 *
 * Revision 1.5  1999/05/28 07:20:58  bnv
 * ITERATE was jumping to the wrong position inside a "DO UNTIL" loop
 *
 * Revision 1.5  1999/05/27 10:13:37  bnv
 * ITERATE was pointing to a wrong place in a "DO xxx UNTIL" loop
 *
 * Revision 1.4  1999/05/14 12:31:22  bnv
 * Corrected a bug when registering a label_sy
 *
 * Revision 1.3  1999/03/15 10:08:09  bnv
 * Changed: Do not create IdentInfo for non Symbol strings
 *
 * Revision 1.2  1999/03/10 16:53:32  bnv
 * LoopCtrl changed from word to size_t
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

#define __COMPILE_C__

#include <setjmp.h>
#include <cmssys.h>

#include "lerror.h"
#include "lstring.h"
#include "bintree.h"
#include "dqueue.h"

#include "rexx.h"
#include "trace.h"
#include "compile.h"
#include "nextsymb.h"
#include "context.h"

/*
//
// Need To DO
//  DO NOT ACCEPT LABELS inside DO-END, IF THEN SELECT etc... blocks
// Kill after the interpetation the extra CLAUSES!!!!!
//
*/

/* ------------ local defines ------------ */
#ifndef ALIGN
#define CODEFIXUPB(p,v) *(byte *)(LSTR(*(context->compileCompileCode)) + (p)) = (v)
#define CODEFIXUP(p,v) *(word *)(LSTR(*(context->compileCompileCode)) + (p)) = (v)
#define CLAUSESTEP sizeof(byte)
#else
#define CODEFIXUP(p, v) *(dword *)(LSTR(*(context->compileCompileCode)) + (p)) = (v)
#define CODEFIXUPB(p, v) CODEFIXUP(p,v)
#define CLAUSESTEP sizeof(dword)
#endif

/* ---- function prototypes ---- */
int __CDECL C_expr(int);

void __CDECL C_template(void);

TBltFunc *__CDECL C_isBuiltin(PLstr);

typedef struct loop_st {
    size_t Citerate;
    size_t Cleave;
    int noofvars;
    PLstr ctrlvar;
} LoopCtrl;

/* ------------ local prototypes ------------ */
static bool C_instr(bool);

static int C_chk4assign(void);

static void C_address(void);

static void C_assign(void);

static void C_arg(void);

static void C_call(void);

static void C_do(void);

static void C_drop(void);

static void C_error(void);

static void C_exit(void);

static void C_if(void);

static void C_interpret(void);

static void C_iterate(void);

static void C_leave(void);

static void C_lower(void);

static void C_nop(void);

static void C_numeric(void);

static void C_parse(void);

static void C_pull(void);

static void C_push(void);

static void C_queue(void);

static void C_return(void);

static void C_say(void);

static void C_select(void);

static void C_signal(void);

static void C_trace(void);

static void C_upper(void);

/* ---------------------------------------------------------- */
/* because ths is const we shall keep it really global */
static const
struct sort_list_st {
    char *name;

    void (*func)(void);
}
/*  WARNING THE LIST MUST BE SORTED!!!!!!!!!!!! */
statements_list[] = {
        {"ADDRESS",   C_address},
        {"ARG",       C_arg},
        {"CALL",      C_call},
        {"DO",        C_do},
        {"DROP",      C_drop},
        {"ELSE",      C_error},
        {"EXIT",      C_exit},
        {"IF",        C_if},
        {"INTERPRET", C_interpret},
        {"ITERATE",   C_iterate},
        {"LEAVE",     C_leave},
        {"LOWER",     C_lower},
        {"NOP",       C_nop},
        {"NUMERIC",   C_numeric},
        {"OTHERWISE", C_error},
        {"PARSE",     C_parse},
        {"PROCEDURE", C_error},
        {"PULL",      C_pull},
        {"PUSH",      C_push},
        {"QUEUE",     C_queue},
        {"RETURN",    C_return},
        {"SAY",       C_say},
        {"SELECT",    C_select},
        {"SIGNAL",    C_signal},
        {"THEN",      C_error},
        {"TRACE",     C_trace},
        {"UPPER",     C_upper},
        {"WHEN",      C_error}
};

/* ---------------- crloopctrl ------------------- */
static LoopCtrl *
crloopctrl(size_t it, size_t le, int vars, PLstr cv) {
    LoopCtrl *lc;
    Context *context = (Context *) CMSGetPG();

    lc = (LoopCtrl *) MALLOC(sizeof(LoopCtrl), "LoopCtrl");
    lc->Citerate = it;
    lc->Cleave = le;
    lc->noofvars = vars;
    lc->ctrlvar = cv;

    DQPUSH(&(context->compile_Loop), lc);
    return lc;
} /* LoopCtrl */

/* ------------- CreateClause ---------------- */
static void
CreateClause(void) {
    Context *context = (Context *) CMSGetPG();
    /* --- Check if the previous mnemonic was a NEWCLAUSE also --- */
    if ((context->compileCompileCurClause) &&
        (context->compileCompileClause)[(context->compileCompileCurClause) -
                                        1].code ==
        (context->compileCompileCodeLen) - CLAUSESTEP)
        return;

    /* --- create a clause --- */
    (context->compileCompileClause)[(context->compileCompileCurClause)].ptr =
            (context->nextsymbsymbolprevptr);
    (context->compileCompileClause)[(context->compileCompileCurClause)].line =
            (context->nextsymbsymboline);
    (context->compileCompileClause)[(context->compileCompileCurClause)].code =
            (context->compileCompileCodeLen);
    (context->compileCompileClause)[(context->compileCompileCurClause)]
            .nesting = (context->compileCompileNesting);
    (context->compileCompileClause)[(context->compileCompileCurClause)].fptr =
            (context->compileCompileRxFile);

    (context->compileCompileCurClause)++;
    if ((context->compileCompileCurClause) ==
        (context->compileCompileClauseItems)) {
        (context->compileCompileClause) = (Clause *) REALLOC(
                (context->compileCompileClause),
                ((context->compileCompileClauseItems) + CLAUSE_INC) *
                sizeof(Clause));
        (context->compileCompileClauseItems) += CLAUSE_INC;
    }

    _CodeAddByte(OP_NEWCLAUSE);
} /* CreateClause */

/* --------------- _mustbe -------------------- */
void __CDECL
_mustbe(enum symboltype sym, int errno, int subno) {
    Context *context = (Context *) CMSGetPG();
    if ((context->nextsymbsymbol) == sym)
        nextsymbol();
    else
        (context->lstring_Lerror)(errno, subno, &(context->nextsymbsymbolstr));
/*
///// This is not correct!!!!!
*/
} /* _mustbe */

#ifndef ALIGN
/* ### If not defined ALIGN ### */

/* --------------- _CodeInsByte --------------- */
word
_CodeInsByte( word pos, byte b )
{
 Context *context = (Context*)CMSGetPG();
 if ((context->compileCompileCodeLen)+sizeof(b) >= LMAXLEN(*(context->compileCompileCode))) {
  Lfx((context->compileCompileCode), (context->compileCompileCodeLen) + CODE_INC);
  (context->compileCompileCodePtr) = (byte*)LSTR(*(context->compileCompileCode)) + (context->compileCompileCodeLen);
 }
 /* shift entire code by one byte */
 MEMMOVE(LSTR(*(context->compileCompileCode))+pos+1,
  LSTR(*(context->compileCompileCode))+pos,
  (context->compileCompileCodeLen)-pos);
 LSTR(*(context->compileCompileCode))[pos] = b;
 (context->compileCompileCodePtr)++;
 return (context->compileCompileCodeLen)++;
} /* _CodeInsByte */

/* --------------- _CodeAddByte --------------- */
word
_CodeAddByte( byte b )
{
 Context *context = (Context*)CMSGetPG();
 if ((context->compileCompileCodeLen)+sizeof(b) >= LMAXLEN(*(context->compileCompileCode))) {
  Lfx((context->compileCompileCode), (context->compileCompileCodeLen) + CODE_INC);
  (context->compileCompileCodePtr) = (byte*)LSTR(*(context->compileCompileCode)) + (context->compileCompileCodeLen);
 }
 *(context->compileCompileCodePtr)++ = b;
 return (context->compileCompileCodeLen)++;
} /* _CodeAddByte */

/* --------------- _CodeAddWord --------------- */
word
_CodeAddWord( word w )
{
 word pos;
 Context *context = (Context*)CMSGetPG();
 if ((context->compileCompileCodeLen)+sizeof(w) >= LMAXLEN(*(context->compileCompileCode))) {
  Lfx((context->compileCompileCode), (context->compileCompileCodeLen) + CODE_INC);
  (context->compileCompileCodePtr) = (byte*)LSTR(*(context->compileCompileCode)) + (context->compileCompileCodeLen);
 }
 pos = (context->compileCompileCodeLen);
 *(word *)(context->compileCompileCodePtr) = w;
 (context->compileCompileCodePtr) += sizeof(w);
 (context->compileCompileCodeLen) += sizeof(w);
 return pos;
} /* _CodeAddWord */

/* ### If defined ALIGN ### */
#else

/* --------------- _CodeInsByte --------------- */
dword
_CodeInsByte(dword pos, dword d) {
    Context *context = (Context *) CMSGetPG();
    if ((context->compileCompileCodeLen) + sizeof(d) >=
        LMAXLEN(*(context->compileCompileCode))) {
        Lfx((context->compileCompileCode),
            (context->compileCompileCodeLen) + CODE_INC);
        (context->compileCompileCodePtr) =
                (byte *) LSTR(*(context->compileCompileCode)) +
                (context->compileCompileCodeLen);
    }
    /* shift entire code by one dword */
    MEMMOVE(LSTR(*(context->compileCompileCode)) + pos + sizeof(dword),
            LSTR(*(context->compileCompileCode)) + pos,
            (context->compileCompileCodeLen) - pos);
    *(dword *) (LSTR(*(context->compileCompileCode)) + pos) = d;
    (context->compileCompileCodePtr) += sizeof(d);
    (context->compileCompileCodeLen) += sizeof(d);
    return (context->compileCompileCodeLen);
} /* _CodeInsByte */

/* --------------- _CodeAddDWord --------------- */
dword
_CodeAddDWord(dword d) {
    dword pos;
    Context *context = (Context *) CMSGetPG();

    if ((context->compileCompileCodeLen) + sizeof(d) >=
        LMAXLEN(*(context->compileCompileCode))) {
        Lfx((context->compileCompileCode),
            (context->compileCompileCodeLen) + CODE_INC);
        (context->compileCompileCodePtr) =
                (byte *) LSTR(*(context->compileCompileCode)) +
                (context->compileCompileCodeLen);
    }
    pos = (context->compileCompileCodeLen);
    *(dword *) (context->compileCompileCodePtr) = d;
    (context->compileCompileCodePtr) += sizeof(d);
    (context->compileCompileCodeLen) += sizeof(d);
    return pos;
} /* _CodeAddWord */

#endif

/* --------------- _CodeAddPtr ---------------- */
dword
_CodeAddPtr(void *ptr) {
    dword pos;
    Context *context = (Context *) CMSGetPG();

    if ((context->compileCompileCodeLen) + sizeof(ptr) >=
        LMAXLEN(*(context->compileCompileCode))) {
        Lfx((context->compileCompileCode),
            (context->compileCompileCodeLen) + CODE_INC);
        (context->compileCompileCodePtr) =
                (byte *) LSTR(*(context->compileCompileCode)) +
                (context->compileCompileCodeLen);
    }
    pos = (context->compileCompileCodeLen);
    *(dword *) (context->compileCompileCodePtr) = (dword) ptr;
    (context->compileCompileCodePtr) += sizeof(ptr);
    (context->compileCompileCodeLen) += sizeof(ptr);
    return pos;
} /* _CodeAddPtr */

/* -------------- _Add2Lits ----------------- */
void *
_Add2Lits(PLstr lit, int hasdot) {
    PBinLeaf leaf;
    PLstr tosearch;
    Lstr newstr, numstr, aux;
    char *ch;
    int i, t, cnt, start, stop;
    IdentInfo *inf;
    Context *context = (Context *) CMSGetPG();

    /* Find in tree */
    tosearch = lit;
    LINITSTR(numstr);
    t = _Lisnum(lit);
    if (t == LINTEGER_TY) {
        Lstrcpy(&numstr, lit);
        L2int(&numstr);
        if (!Lstrcmp(&numstr, lit)) {
            L2int(&numstr);
            tosearch = &numstr;
        }
    } else if (t == LREAL_TY) {
        Lstrcpy(&numstr, lit);
        L2real(&numstr);
        if (!Lstrcmp(&numstr, lit)) {
            L2real(&numstr);
            tosearch = &numstr;
        }
    }

    leaf = BinFind(&(context->rexxrxLitterals), tosearch);
    if (leaf == NULL)
        if (tosearch == &numstr)
            leaf = BinFind(&(context->rexxrxLitterals), tosearch);

    if (leaf == NULL) {
        LINITSTR(newstr);
        Lfx(&newstr, 1);
        Lstrcpy(&newstr, tosearch);

#ifdef USEOPTION
        /* set the option for faster recognition */
        if (LTYPE(newstr) == LINTEGER_TY)
         LSETOPT(newstr,LOPTINT);
        else
        if (LTYPE(newstr) == LREAL_TY)
         LSETOPT(newstr,LOPTREAL);
#endif

        if ((LTYPE(newstr) == LSTRING_TY) &&
            !LISNULL(newstr) && Ldatatype(&newstr, 'S')) {
            LASCIIZ(newstr);
            if (hasdot) {
                /* count number of dots */
                for (cnt = 1, ch = LSTR(newstr); *ch; ch++)
                    if (*ch == '.')
                        cnt++;

                /* allocate space */
                inf = (IdentInfo *) MALLOC
                (sizeof(IdentInfo) + (cnt - 1) * sizeof(PBinLeaf),
                 "ID_INFO_STEM");
                /* count substrings and add them to lits */
                inf->id = NO_CACHE;
                inf->stem = cnt;

                /* scan for substrings */
                LINITSTR(aux);
                _Lsubstr(&aux, &newstr, 1, hasdot);
                inf->leaf[0] = _Add2Lits(&aux, FALSE);

                stop = hasdot + 1; /* after the dot */
                ch = LSTR(newstr) + hasdot;
                i = 1;
                while (1) {
                    while (*ch == '.') {
                        inf->leaf[i++] = NULL;
                        ch++;
                        stop++;
                    }
                    if (!*ch) {
                        inf->leaf[i++] = NULL;
                        break;
                    }

                    start = stop; /* find next dot */
                    while (*ch && *ch != '.') {
                        ch++;
                        stop++;
                    }
                    _Lsubstr(&aux, &newstr, start, stop - start);
                    inf->leaf[i++] = _Add2Lits(&aux, FALSE);
                    if (!*ch) break;
                    if (*ch == '.') {
                        ch++;
                        stop++;
                    }
                }
                LFREESTR(aux);
            } else {
                inf = (IdentInfo *) MALLOC(sizeof(IdentInfo), "ID_INFO");
                inf->stem = 0;
                inf->leaf[0] = NULL;
            }
            inf->id = NO_CACHE;
        } else
            inf = NULL;

        leaf = BinAdd(&(context->rexxrxLitterals), &newstr, inf);
    }
    LFREESTR(numstr);
    return leaf;
} /* _Add2Lits */

/* ------------------- _AddLabel ------------------ */
PBinLeaf
_AddLabel(int type, size_t offset) {
    PBinLeaf leaf;
    Lstr newstr;
    RxFunc *func;
    TBltFunc *isbuiltin;
    Context *context = (Context *) CMSGetPG();

    /* --- check to see if we are interpeting a string --- */
    if ((context->compile_str_interpreted) && type == FT_LABEL &&
        offset != UNKNOWN_LABEL)
        (context->lstring_Lerror)(ERR_UNEXPECTED_LABEL, 1,
                                  &(context->nextsymbsymbolstr));

    /* --- Find in tree --- */
    leaf = BinFind(&(context->rexx_labels), &(context->nextsymbsymbolstr));
    if (leaf == NULL) {
        LINITSTR(newstr);
        Lstrcpy(&newstr, &(context->nextsymbsymbolstr));
        func = (RxFunc *) MALLOC(sizeof(RxFunc), "FuncLabel");
        func->type = type;
        func->builtin = NULL;
        func->label = offset;

        if ((context->nextsymbsymbolisstr)) {
            func->type = FT_SYSTEM;
            func->systype = SYST_UNKNOWN;
        } else if (type == FT_FUNCTION) {
            isbuiltin = C_isBuiltin(&(context->nextsymbsymbolstr));
            if (isbuiltin == NULL) {
                /* then it might be internal external or system */
                func->type = FT_FUNCTION;
                func->label = offset;
            } else {
                func->type = FT_BUILTIN;
                func->builtin = isbuiltin;
            }
        }

        leaf = BinAdd(&(context->rexx_labels), &newstr, func);
    } else if (offset != UNKNOWN_LABEL) {
        func = (RxFunc *) (leaf->value);
        if (func->label == UNKNOWN_LABEL) {
            /* label found change function type - Internal calls have priority */
            if (func->type == FT_BUILTIN || func->type == FT_SYSTEM)
                func->type = FT_INTERNAL;
            func->label = offset;
        }
    }
    return leaf;
} /* _AddLabel */

/* -------------------------------------------------------------- */
/* C_error,  reports an error, when an illegal (context->nextsymbsymbol) is found.   */
/* -------------------------------------------------------------- */
static void
C_error(void) {
    Context *context = (Context *) CMSGetPG();

    if (!CMP("ELSE")) (context->lstring_Lerror)(ERR_THEN_UNEXCEPTED, 2);
    else if (!CMP("OTHERWISE")) (context->lstring_Lerror)(ERR_WHEN_UNCEPTED, 0);
    else if (!CMP("PROCEDURE"))
        (context->lstring_Lerror)(ERR_UNEXPECTED_PROC, 0);
    else if (!CMP("THEN")) (context->lstring_Lerror)(ERR_THEN_UNEXCEPTED, 1);
    else if (!CMP("WHEN")) (context->lstring_Lerror)(ERR_WHEN_UNCEPTED, 0);
    else
        (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
} /* C_error */

/* -------------------------------------------------------------- */
/*  ADDRESS [<(context->nextsymbsymbol) | string> [expr]] ;                          */
/*      redirect commands or a single command to a new            */
/*      environment. ADDRESS VALUE expr may be used               */
/*      for an evaluated enviroment name.                         */
/* -------------------------------------------------------------- */
static void
C_address(void) {
    Context *context = (Context *) CMSGetPG();

    if ((context->nextsymbsymbol) == semicolon_sy) {
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr((context->rexxsystemStr));
        TraceByte(other_middle);
        _CodeAddByte(OP_STOREOPT);
        _CodeAddByte(environment_opt);
        return;
    }

    if ((context->nextsymbsymbol) == le_parent || identCMP("VALUE")) {
        if ((context->nextsymbsymbol) == ident_sy) nextsymbol();
        C_expr(exp_normal);
#ifdef MSDOS
        _CodeAddByte(OP_COPY2TMP);
        _CodeAddByte(OP_UPPER);
#endif
        _CodeAddByte(OP_STOREOPT);
        _CodeAddByte(environment_opt);
    } else if ((context->nextsymbsymbol) == literal_sy ||
               (context->nextsymbsymbol) == ident_sy) {
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr(SYMBOLADD2LITS_KEY);
        TraceByte(other_middle);
#ifdef MSDOS
        _CodeAddByte(OP_COPY2TMP);
        _CodeAddByte(OP_UPPER);
#endif
        nextsymbol();
        if ((context->nextsymbsymbol) != semicolon_sy) {
            C_expr(exp_normal);
            _CodeAddByte(OP_SYSTEM);
        } else {
            _CodeAddByte(OP_STOREOPT);
            _CodeAddByte(environment_opt);
        }
    } else
        (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
} /* C_address */

/* -------------------------------------------------------------- */
/*  ARG     <template> ;                                          */
/*      parse argument string(s), translated into uppercase,      */
/*      into variables. Short for PARSE UPPER ARG.                */
/* -------------------------------------------------------------- */
static void
C_arg(void) {
    int ai;
    Context *context = (Context *) CMSGetPG();

    ai = 0;
    do {
        if ((context->nextsymbsymbol) == comma_sy) nextsymbol();
        _CodeAddByte(OP_LOADARG);
        _CodeAddByte(ai);
        _CodeAddByte(OP_COPY2TMP);
        _CodeAddByte(OP_UPPER);
        C_template();
        ai++;
    } while ((context->nextsymbsymbol) == comma_sy);
} /* C_arg */

/* -------------------------------------------------------------- */
/*  CALL    [(context->nextsymbsymbol) | string] [<expr>] [,<expr>]... ;             */
/*      call an internal routine, an external routine or program, */
/*      or a built-in function. Depending on the type of          */
/*      routine called, the variable RESULT contains the result   */
/*      of the routine. RESULT is uninitialized if no result is   */
/*      returned.                                                 */
/* -------------------------------------------------------------- */
static void
C_call(void) {
    int ia, func, line, realarg = 0;
    word existarg = 0, bp = 1, lastarg = 0; /* bit mapped, if arguments exist */
    void *lbl;
    Context *context = (Context *) CMSGetPG();

    /* keep line number */
    line = (context->nextsymbsymboline);
    func = ((context->nextsymbsymbol) == function_sy);

    if (((context->nextsymbsymbol) != ident_sy) &&
        ((context->nextsymbsymbol) != literal_sy) &&
        ((context->nextsymbsymbol) != function_sy))
        (context->lstring_Lerror)(ERR_STRING_EXPECTED, 0);

/* ///////// NOT CORRECT //////////// */
/* Make it work as the SIGNAL ON ...  */
    if (!CMP("OFF") || (!CMP("ON"))) {
        C_signal();
        return;
    }
/* ///////////////////////////////// */

    lbl = _AddLabel(FT_FUNCTION, UNKNOWN_LABEL);

    /* since we don't know if it is going to return
       a result then we create a stack space */
    _CodeAddByte(OP_PUSHTMP);

    nextsymbol();
    ia = 0;

    if (((context->nextsymbsymbol) != semicolon_sy) ||
        (func && ((context->nextsymbsymbol) != ri_parent))) {
        if ((context->nextsymbsymbol) != comma_sy) {
            C_expr(exp_normal);
            realarg++;
            lastarg = ia + 1;
            existarg |= bp; /* argument exist */
        }
        ia++;
        bp <<= 1;       /* increase bit position */

        while ((context->nextsymbsymbol) == comma_sy) {
            nextsymbol();
            if (ia >= MAXARGS)
                (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
            if (!(((context->nextsymbsymbol) == comma_sy) ||
                  ((context->nextsymbsymbol) == ri_parent) ||
                  ((context->nextsymbsymbol) == semicolon_sy))) {
                C_expr(exp_normal);
                realarg++;
                lastarg = ia + 1;
                existarg |= bp;
            }
            ia++;
            bp <<= 1;       /* increase bit position */
        }
    }


    _CodeAddByte(OP_CALL);
    _CodeAddPtr(lbl); /* call pointer */
    _CodeAddByte(lastarg); /* arguments */
    _CodeAddByte(realarg); /* real args */
    _CodeAddWord(existarg); /* which exist */
    _CodeAddWord(line); /* (context->nextsymbsymbol) line */
    _CodeAddByte(CT_PROCEDURE); /* call type */
    TraceByte(nothing_middle);

    if (func)
        _mustbe(ri_parent, ERR_UNMATCHED_PARAN, 0);
} /* C_call */

/* -------------------------------------------------------------- */
/*  DO      [ [name=expri  [TO exprt] [BY exprb]                  */
/*            [FOR exprf]] | [ FOREVER | exprr ]                  */
/*          [UNTIL expru | WHILE exprw] ;                         */
/*          [instr]... ;                                          */
/*  END [(context->nextsymbsymbol)] ;                                                */
/*      group instructions together with optional repetition and  */
/*      condition. NAME is stepped from EXPRI to EXPRT in         */
/*      steps of EXPRB, for a maximum of EXPRF iterations.        */
/*      These variables are evaluated only once at the top of     */
/*      the loop and must result in numbers. The iterative        */
/*      phrase may be replaced by EXPRR, which is a loop          */
/*      count (no variable used), or by the keyword               */
/*      FOREVER. If a WHILE or UNTIL is given, EXPRU or           */
/*      EXPRW must evaluate to "0" or "1". The condition is       */
/*      tested at the top of the loop if WHILE or at the bottom   */
/*      if UNTIL.                                                 */
/* -------------------------------------------------------------- */
/* --- DO until END --- */
static void
DOuntilEND(void) {
    Context *context = (Context *) CMSGetPG();
    while (1) {  /* no need for leave & iterate label */
        SKIP_SEMICOLONS;
        if (C_instr(TRUE)) break;
    }
    nextsymbol(); /* Skip END */
} /* DOuntilEND */

#define DO_ASSIGN 0x0001
#define DO_TO  0x0002
#define DO_BY  0x0004
#define DO_FOR  0x0008
#define DO_FOREVER 0x0010
#define DO_WHILE 0x0020
#define DO_UNTIL 0x0040

/* --- C_do --- */
static void
C_do(void) {
    Context *context = (Context *) CMSGetPG();
    /*
     * I must define a label for ITERATE and LEAVE in all the cases
     * except the case of single loop;  DO ... END
     */
    enum stat_type old_statement = (context->nextsymbsymbolstat);
    LoopCtrl *lc;
    PLstr CtrlVar = NULL;
    void *cv_ptr = NULL;
    size_t body_p, iterate_p, fix_iterate = 0, leave_p, fix_leave = 0;
    size_t untilexpr = 0, overuntil, untilend = 0;
    size_t pat = 0, tmp;
    word idx = 0, idxTO = 0, idxBY = 0, idxFOR = 0;
    int dotype = 0;

    /* Simplest case */
    if ((context->nextsymbsymbol) == semicolon_sy) { /* SINGLE LOOP */
        (context->nextsymbsymbolstat) = in_do_st;
        DOuntilEND();
        (context->nextsymbsymbolstat) = old_statement;
        return;
    }

    (context->nextsymbsymbolstat) = in_do_init_st;

    /* --- First check for repetition --- */
    /* --- and construct header       --- */

    if (C_chk4assign()) {
        cv_ptr = SYMBOLADD2LITS;
        CtrlVar = &(((PBinLeaf) cv_ptr)->key);

        C_assign();
        dotype |= DO_ASSIGN;
        while ((context->nextsymbsymbol) == to_sy ||
               (context->nextsymbsymbol) == for_sy ||
               (context->nextsymbsymbol) == by_sy)
            switch ((context->nextsymbsymbol)) {
                case to_sy:
                    if (idxTO)
                        (context->lstring_Lerror)(ERR_INVALID_DO_SYNTAX, 0);
                    idxTO = ++idx;
                    nextsymbol(); /* Skip TO */
                    dotype |= DO_TO;
                    C_expr(exp_tmp);
                    break;
                case by_sy:
                    if (idxBY)
                        (context->lstring_Lerror)(ERR_INVALID_DO_SYNTAX, 0);
                    idxBY = ++idx;
                    nextsymbol(); /* Skip BY */
                    dotype |= DO_BY;
                    C_expr(exp_normal);
                    _CodeAddByte(OP_BYINIT);
                    pat = _CodeAddWord(0); /* Patch position */
                    break;
                case for_sy:
                    if (idxFOR)
                        (context->lstring_Lerror)(ERR_INVALID_DO_SYNTAX, 0);
                    idxFOR = ++idx;
                    nextsymbol(); /* Skip FOR */
                    dotype |= DO_FOR;
                    C_expr(exp_normal);
                    _CodeAddByte(OP_FORINIT);
                    break;
                default:
                    (context->lstring_Lerror)(ERR_INVALID_DO_SYNTAX, 0);
            }
    } else /* end of C_chk4assign */
    if (identCMP("FOREVER")) {
        dotype |= DO_FOREVER;
        nextsymbol(); /* Skip FOREVER */
    } else
        /* --- Check for WHILE/UNTIL --- */
    if (identCMP("WHILE")) /* do nothing */;
    else if (identCMP("UNTIL")) /* do nothing */;
    else {  /* ----- REPETITION LOOP ----- */
        if (idxFOR)
            (context->lstring_Lerror)(ERR_INVALID_DO_SYNTAX, 0);
        idxFOR = ++idx;
        if (identCMP("FOR")) nextsymbol(); /* skip FOR */
        dotype |= DO_FOR;
        C_expr(exp_normal);
        _CodeAddByte(OP_FORINIT);
    }

    /* Create a jmp reference at the end and to the iterate pos */
    /* jump over leave "jmp" */

    _CodeAddByte(OP_JMP);
    tmp = _CodeAddWord(0);
    leave_p = (context->compileCompileCodeLen);
    _CodeAddByte(OP_JMP);
    fix_leave = _CodeAddWord(0);

    if (dotype & DO_ASSIGN) {
        iterate_p = (context->compileCompileCodeLen);
        _CodeAddByte(OP_JMP);
        fix_iterate = _CodeAddWord(0);
    }

    CODEFIXUP(tmp, (context->compileCompileCodeLen));
    body_p = (context->compileCompileCodeLen);

    /* --- Create the main loop control --- */
    if (dotype & DO_ASSIGN)
        lc = crloopctrl(iterate_p, leave_p, idx, CtrlVar);
    else
        lc = crloopctrl(body_p, leave_p, idx, CtrlVar);


    if ((context->nextsymbsymbol) == while_sy || identCMP("WHILE")) {
        dotype |= DO_WHILE;
        nextsymbol();  /* Skip WHILE */
        C_expr(exp_normal);
        _CodeAddByte(OP_JF);
        _CodeAddWord(leave_p);
    } else if ((context->nextsymbsymbol) == until_sy || identCMP("UNTIL")) {
        dotype |= DO_UNTIL;
        nextsymbol();  /* Skip UNTIL */
        _CodeAddByte(OP_JMP);
        overuntil = _CodeAddWord(0);
        untilexpr = (context->compileCompileCodeLen);

        /* modify to the correct iterate address */
        /* to check the UNTIL expr after the iteration */
        lc->Citerate = (context->compileCompileCodeLen);

        C_expr(exp_normal);
        _CodeAddByte(OP_JT);
        _CodeAddWord(leave_p);
        _CodeAddByte(OP_JMP);
        untilend = _CodeAddWord(0);
        CODEFIXUP(overuntil, (context->compileCompileCodeLen));
    }

    /* --- create code for TO,BY,FOR --- */
    if (idxTO) {
        _CodeAddByte(OP_DUP);
        _CodeAddByte(idx - idxTO);
        _CodeAddByte(OP_LOAD);
        _CodeAddPtr(cv_ptr);
        TraceByte(nothing_middle);

        if (idxBY) {
            CODEFIXUP(pat,
                      (context->compileCompileCodeLen)); /* Patch reference */
        }
        _CodeAddByte(OP_TGE);  /* This byte will be patched */
        TraceByte(nothing_middle);
        _CodeAddByte(OP_JF);
        _CodeAddWord(leave_p);
    }
    if (idxFOR) {
        _CodeAddByte(OP_DECFOR);
        _CodeAddByte(idx - idxFOR); /* variable */
        _CodeAddWord(leave_p);
    }

    /* ===== main body ====== */
    _CodeAddByte(OP_NEWCLAUSE);
    (context->nextsymbsymbolstat) = in_do_st;
    _mustbe(semicolon_sy, ERR_INVALID_DO_SYNTAX, 0);
    DOuntilEND();

    if (CtrlVar) {
        if ((context->nextsymbsymbol) == ident_sy) {
            if (!Lstrcmp(&(context->nextsymbsymbolstr), CtrlVar))
                nextsymbol();
            else
                (context->lstring_Lerror)(ERR_UNMATCHED_END, 2,
                                          &(context->nextsymbsymbolstr));
        } else if ((context->nextsymbsymbol) != semicolon_sy)
            (context->lstring_Lerror)(ERR_SYMBOL_EXPECTED, 1,
                                      &(context->nextsymbsymbolstr));
    } else if ((context->nextsymbsymbol) == ident_sy)
        (context->lstring_Lerror)(ERR_UNMATCHED_END, 3,
                                  &(context->nextsymbsymbolstr));

    /* --- if UNTIL in DO --- */
    if (dotype & DO_UNTIL) {
        _CodeAddByte(OP_JMP);
        _CodeAddWord(untilexpr);
        CODEFIXUP(untilend, (context->compileCompileCodeLen));
    }

    /* --- calc next value --- */
    if (dotype & DO_ASSIGN) {
        CODEFIXUP(fix_iterate, (context->compileCompileCodeLen));
        if (idxBY) {
            _CodeAddByte(OP_LOAD);
            _CodeAddPtr(cv_ptr);
            TraceByte(nothing_middle);
            _CodeAddByte(OP_DUP);
            _CodeAddByte(0);
            _CodeAddByte(OP_DUP);
            _CodeAddByte(idx - idxBY + 2);
            _CodeAddByte(OP_ADD);
            TraceByte(other_middle);
            _CodeAddByte(OP_POP);
            _CodeAddByte(1);
        } else {
            _CodeAddByte(OP_LOAD);
            _CodeAddPtr(cv_ptr);
            TraceByte(nothing_middle);
            _CodeAddByte(OP_INC);
            TraceByte(other_middle);
        }
    }

    /* --- end of loop, add a jump to the beggining --- */
    _CodeAddByte(OP_JMP);
    _CodeAddWord(body_p);
    CODEFIXUP(fix_leave, (context->compileCompileCodeLen));
    lc = DQPop(&(context->compile_Loop)); /* delete loop control */
    FREE(lc);

    if (dotype & (DO_TO | DO_FOR | DO_BY)) {
        _CodeAddByte(OP_POP); /* pop the last value from stack */
        _CodeAddByte(idx);
    }
    (context->nextsymbsymbolstat) = old_statement;
} /* C_do */

/* -------------------------------------------------------------- */
/*  DROP name [name]... ;                                         */
/*      drop (reset) the named variables or group of variables.   */
/*                                                                */
/*  *** if an exposed variable is named, the variable itself      */
/*      in the older generation will be dropped!                  */
/* -------------------------------------------------------------- */
static void
C_drop(void) {
    Context *context = (Context *) CMSGetPG();
    while (1) {
        if ((context->nextsymbsymbol) == ident_sy) {
            _CodeAddByte(OP_DROP);
            _CodeAddPtr(SYMBOLADD2LITS);
            TraceByte(variable_middle);
            nextsymbol();
        } else if ((context->nextsymbsymbol) == le_parent) {
            nextsymbol();
            if ((context->nextsymbsymbol) != ident_sy)
                (context->lstring_Lerror)(ERR_SYMBOL_EXPECTED, 1,
                                          &(context->nextsymbsymbolstr));
            _CodeAddByte(OP_LOAD);
            _CodeAddPtr(SYMBOLADD2LITS);
            TraceByte(variable_middle);
            nextsymbol();
            _CodeAddByte(OP_COPY2TMP);
            _mustbe(ri_parent, ERR_UNMATCHED_PARAN, 0);
            _CodeAddByte(OP_DROPIND);
            TraceByte(variable_middle);
        } else
            break;
    }
} /* C_drop */

/* -------------------------------------------------------------- */
/*  EXIT [expr] ;                                                 */
/*      leave the program (with return data, EXPR). EXIT is       */
/*      the same as RETURN except that all internal routines      */
/*      are terminated                                            */
/* -------------------------------------------------------------- */
static void
C_exit(void) {
    Context *context = (Context *) CMSGetPG();
    if ((context->nextsymbsymbol) == semicolon_sy) {
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr(&((context->rexxzeroStr)->key));
        TraceByte(nothing_middle);
    } else
        C_expr(exp_normal);
    _CodeAddByte(OP_EXIT);
} /* C_exit */

/* -------------------------------------------------------------- */
/*  IF expr [;] THEN [;] instr ;                                  */
/*             [ELSE [;] instr];                                  */
/*      if EXPR evaluates to "1", execute the instruction         */
/*      following the THEN. Otherwise, (EXPR evaluates to         */
/*      "0") skip that instruction and execute the one            */
/*      following the ELSE clause, if present.                    */
/* -------------------------------------------------------------- */
static void
C_if(void) {
    enum stat_type old_statement;
    size_t nxt, end;
    Context *context = (Context *) CMSGetPG();

    old_statement = (context->nextsymbsymbolstat);
    (context->nextsymbsymbolstat) = in_if_init_st;
    C_expr(exp_normal);

    SKIP_SEMICOLONS;

    _CodeAddByte(OP_JF);
    nxt = _CodeAddWord(0);

    CreateClause();
    _mustbe(then_sy, ERR_THEN_EXPECTED, 1);
    (context->nextsymbsymbolstat) = in_if_st;

    SKIP_SEMICOLONS;

    (context->nextsymbsymbolstat) = normal_st;
    C_instr(FALSE);

    SKIP_SEMICOLONS;
    if (identCMP("ELSE")) {
        (context->nextsymbsymbolstat) = in_if_st;
        _CodeAddByte(OP_JMP);
        end = _CodeAddWord(0);
        CODEFIXUP(nxt, (context->compileCompileCodeLen));
        CreateClause();

        nextsymbol();
        SKIP_SEMICOLONS;

        C_instr(FALSE);
        nxt = end;
    }
    CODEFIXUP(nxt, (context->compileCompileCodeLen));
    (context->nextsymbsymbolstat) = old_statement;
    (context->compile_checked_semicolon) = TRUE;
} /* C_if */

/* -------------------------------------------------------------- */
/*  INTERPRET  expr ;                                             */
/*      evaluate EXPR and then execute the resultant string as    */
/*      if it was part of the original program.                   */
/* -------------------------------------------------------------- */
static void
C_interpret(void) {
    C_expr(exp_normal);
    _CodeAddByte(OP_INTERPRET);
} /* C_interpret */

/* -------------------------------------------------------------- */
/*  ITERATE   [name|num] ;                                        */
/*      start next iteration of the innermost repetitive loop     */
/*      (or loop with control variable name).                     */
/* -------------------------------------------------------------- */
static void
C_iterate(void) {
    LoopCtrl *lc = NULL;
    DQueueElem *elem;
    word pop = 0;
    Context *context = (Context *) CMSGetPG();

    if (!(context->compile_Loop).items)
        (context->lstring_Lerror)(ERR_INVALID_LEAVE, 0);

    if ((context->nextsymbsymbol) == ident_sy) {
        elem = (context->compile_Loop).tail;
        while (1) {
            if (!elem)
                (context->lstring_Lerror)(ERR_INVALID_LEAVE, 0);
            lc = (LoopCtrl *) elem->dat;
            if (lc->ctrlvar) {
                if (Lstrcmp(&(context->nextsymbsymbolstr), lc->ctrlvar)) {
                    pop += lc->noofvars;
                } else
                    break;
            }
            elem = elem->prev;
        }
        nextsymbol();
        if (pop) {
            _CodeAddByte(OP_POP);
            _CodeAddByte(pop);
        }
    } else if ((context->nextsymbsymbol) == semicolon_sy)
        lc = DQPEEK(&(context->compile_Loop));
    else
        (context->lstring_Lerror)(ERR_SYMBOL_EXPECTED, 2,
                                  &(context->nextsymbsymbolstr));
    _CodeAddByte(OP_JMP);
    _CodeAddWord(lc->Citerate);
} /* C_iterate */

/* -------------------------------------------------------------- */
/*  LEAVE     [name|num] ;                                        */
/*      terminate innermost loop (or loop with control            */
/*      variable name).                                           */
/* -------------------------------------------------------------- */
static void
C_leave(void) {
    LoopCtrl *lc = NULL;
    DQueueElem *elem;
    word pop = 0;
    Context *context = (Context *) CMSGetPG();

    if (!(context->compile_Loop).items)
        (context->lstring_Lerror)(ERR_INVALID_LEAVE, 0);

    if ((context->nextsymbsymbol) == ident_sy) {
        elem = (context->compile_Loop).tail;
        while (1) {
            if (!elem)
                (context->lstring_Lerror)(ERR_INVALID_LEAVE, 0);
            lc = (LoopCtrl *) elem->dat;
            if (lc->ctrlvar) {
                if (Lstrcmp(&(context->nextsymbsymbolstr), lc->ctrlvar)) {
                    pop += lc->noofvars;
                } else
                    break;
            }
            elem = elem->prev;
        }
        nextsymbol();
        if (pop) {
            _CodeAddByte(OP_POP);
            _CodeAddByte(pop);
        }
    } else if ((context->nextsymbsymbol) == semicolon_sy)
        lc = DQPEEK(&(context->compile_Loop));
    else
        (context->lstring_Lerror)(ERR_SYMBOL_EXPECTED, 2,
                                  &(context->nextsymbsymbolstr));
    _CodeAddByte(OP_JMP);
    _CodeAddWord(lc->Cleave);
} /* C_leave */

/* -------------------------------------------------------------- */
/*  LOWER name [name]... ;                                        */
/*      translate the values of the specified individual          */
/*      variables to uppercase.                                   */
/* -------------------------------------------------------------- */
static void
C_lower(void) {
    void *sym;
    Context *context = (Context *) CMSGetPG();

    while ((context->nextsymbsymbol) == ident_sy) {
        sym = SYMBOLADD2LITS;
        _CodeAddByte(OP_LOAD);
        _CodeAddPtr(sym);
        TraceByte(variable_middle);
        _CodeAddByte(OP_LOWER);
        _CodeAddByte(OP_POP);
        _CodeAddByte(1);
        nextsymbol();
    }
} /* C_lower */

/* -------------------------------------------------------------- */
/*  NOP  ;                                                        */
/*      dummy instruction, has no side-effects.                   */
/* -------------------------------------------------------------- */
static void
C_nop(void) {
    _CodeAddByte(OP_NOP);
} /* C_nop */

/* -------------------------------------------------------------- */
/*  NUMERIC   DIGITS [expr]  |                                    */
/*            FORM   [SCIENTIFIC | ENGINEERING] |                 */
/*            FUZZ   [expr]  ;                                    */
/*   o  DIGITS, carry out arithmetic operations to EXPR           */
/*      significant digits.                                       */
/*   o  FORM, set new exponential format.                         */
/*   o  FUZZ, ignore up to EXPR least significant digits          */
/*      during arithmetic comparisons.                            */
/* -------------------------------------------------------------- */
static void
C_numeric(void) {
    Context *context = (Context *) CMSGetPG();
    if (identCMP("DIGITS")) {
        nextsymbol();
        C_expr(exp_normal);
        _CodeAddByte(OP_STOREOPT);
        _CodeAddByte(digits_opt);
    } else if (identCMP("FORM")) {
        nextsymbol();
        if ((context->nextsymbsymbol) == semicolon_sy ||
            identCMP("SCIENTIFIC")) {
            _CodeAddByte(OP_PUSH);
            _CodeAddPtr(&((context->rexxzeroStr)->key));
            TraceByte(nothing_middle);
        } else if (identCMP("ENGINEERING")) {
            _CodeAddByte(OP_PUSH);
            _CodeAddPtr(&((context->rexxoneStr)->key));
            TraceByte(nothing_middle);
        } else
            (context->lstring_Lerror)(ERR_INV_SUBKEYWORD, 11,
                                      &(context->nextsymbsymbolstr));

        if ((context->nextsymbsymbol) != semicolon_sy) nextsymbol();

        _CodeAddByte(OP_STOREOPT);
        _CodeAddByte(form_opt);
    } else if (identCMP("FUZZ")) {
        nextsymbol();
        C_expr(exp_normal);
        _CodeAddByte(OP_STOREOPT);
        _CodeAddByte(fuzz_opt);
    } else
        (context->lstring_Lerror)(ERR_INV_SUBKEYWORD, 15,
                                  &(context->nextsymbsymbolstr));
} /* C_numeric */

/* -------------------------------------------------------------- */
/*  PARSE   [UPPER]  + ARG               | [template] ;           */
/*                   | AUTHOR            |                        */
/*                   | EXTERNAL          |                        */
/*                   | LINEIN            |                        */
/*                   | NUMERIC           |                        */
/*                   | PULL              |                        */
/*                   | SOURCE            |                        */
/*                   | VALUE [expr] WITH |                        */
/*                   | VAR name          |                        */
/*                   + VERSION           +                        */
/*   o  ARG, parses the argument string(s) to the program,        */
/*      subroutine, or function. UPPER first translates the       */
/*      strings to uppercase. See also the ARG instruction        */
/*   o  EXTERNAL, read and parse the next string from the         */
/*      terminal input buffer (system external queue).            */
/*   o  NUMERIC, parse the current NUMERIC settings.              */
/*   o  PULL, read and parse the next string from the             */
/*      program stack (system data queue). See the PULL           */
/*      instruction.                                              */
/*   o  SOURCE, parse the program source description e.g.         */
/*      "CMS COMMAND FRED EXEC A fred CMS".                       */
/*   o  VALUE, parse the value of EXPR.                           */
/*   o  VAR, parse the value of NAME.                             */
/*   o  VERSION, parse the data describing the language level     */
/*      and the date of the interpreter.                          */
/* -------------------------------------------------------------- */
static void
C_parse(void) {
    int toupper = FALSE;
    enum stat_type old_statement;
    int ai;
    int with_chk = FALSE;
    Context *context = (Context *) CMSGetPG();

    if (identCMP("UPPER")) {
        toupper = TRUE;
        nextsymbol();
    }

    if (identCMP("ARG")) {
        ai = 0;
        do {
            nextsymbol();
            _CodeAddByte(OP_LOADARG);
            _CodeAddByte(ai);
            _CodeAddByte(OP_COPY2TMP);
            if (toupper)
                _CodeAddByte(OP_UPPER);
            C_template();
            ai++;
        } while ((context->nextsymbsymbol) == comma_sy);
    } else { /* everything else needs only on parse */
        if (identCMP("EXTERNAL") || identCMP("LINEIN")) {
            nextsymbol();
            _CodeAddByte(OP_RX_EXTERNAL);
        } else if (identCMP("NUMERIC")) {
            nextsymbol();
            _CodeAddByte(OP_PUSHTMP);
            _CodeAddByte(OP_PUSHTMP);
            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(digits_opt);
            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(fuzz_opt);
            _CodeAddByte(OP_BCONCAT);
            TraceByte(nothing_middle);
            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(form_opt);
            _CodeAddByte(OP_BCONCAT);
            TraceByte(nothing_middle);
        } else if (identCMP("PULL")) {
            nextsymbol();
            _CodeAddByte(OP_RX_PULL);
        } else if (identCMP("VAR")) {
            nextsymbol();
            if ((context->nextsymbsymbol) != ident_sy)
                (context->lstring_Lerror)(ERR_INVALID_TEMPLATE, 0);

            _CodeAddByte(OP_LOAD);
            _CodeAddPtr(SYMBOLADD2LITS);
            TraceByte(variable_middle);
            nextsymbol();
            _CodeAddByte(OP_COPY2TMP);
        } else if (identCMP("SOURCE")) {
            nextsymbol();
            _CodeAddByte(OP_PUSHTMP);
            _CodeAddByte(OP_PUSHTMP);
            _CodeAddByte(OP_PUSHTMP);
            _CodeAddByte(OP_PUSHTMP);

            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(os_opt);

            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(calltype_opt);
            _CodeAddByte(OP_BCONCAT);
            TraceByte(nothing_middle);

            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(filename_opt);
            _CodeAddByte(OP_BCONCAT);
            TraceByte(nothing_middle);

            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(prgname_opt);
            _CodeAddByte(OP_BCONCAT);
            TraceByte(nothing_middle);

            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(shell_opt);
            _CodeAddByte(OP_BCONCAT);
            TraceByte(nothing_middle);
        } else if (identCMP("VALUE")) {
            old_statement = (context->nextsymbsymbolstat);
            (context->nextsymbsymbolstat) = in_parse_value_st;
            nextsymbol();
            C_expr(exp_tmp);
            (context->nextsymbsymbolstat) = old_statement;
            _mustbe(with_sy, ERR_INVALID_TEMPLATE, 3);
            with_chk = TRUE;
        } else if (identCMP("AUTHOR")) {
            nextsymbol();
            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(author_opt);
        } else if (identCMP("VERSION")) {
            nextsymbol();
            _CodeAddByte(OP_LOADOPT);
            _CodeAddByte(version_opt);
        } else
            (context->lstring_Lerror)(ERR_INV_SUBKEYWORD, 12 + toupper,
                                      &(context->nextsymbsymbolstr));

        /* --- Common Code --- */
        if (toupper)
            _CodeAddByte(OP_UPPER);

        /* skip WITH if exist */
        if (identCMP("WITH") && !with_chk)
            nextsymbol();
        C_template();
    }
} /* C_parse */

/* -------------------------------------------------------------- */
/*  PROCEDURE [EXPOSE name|(var) [name|(var)]...] ;               */
/*      start a new generation of variables within an internal    */
/*      routine. Optionally named variables or groups of          */
/*      variables from an earlier generation may be exposed       */
/* -------------------------------------------------------------- */
static void
C_procedure(void) {
    byte exposed = 0;
    size_t pos;
    Context *context = (Context *) CMSGetPG();

    _CodeAddByte(OP_PROC);
    pos = _CodeAddByte(0);
    if (identCMP("EXPOSE")) {
        nextsymbol();
        while (1) {
            if ((context->nextsymbsymbol) == ident_sy) {
                _CodeAddPtr(SYMBOLADD2LITS);
                exposed++;
                nextsymbol();
            } else if ((context->nextsymbsymbol) == le_parent) {
                nextsymbol();
                if ((context->nextsymbsymbol) != ident_sy)
                    (context->lstring_Lerror)(ERR_STRING_EXPECTED, 7,
                                              &(context->nextsymbsymbolstr));
                /* mark an indirect call */
                _CodeAddPtr(NULL);
                _CodeAddPtr(SYMBOLADD2LITS);
                exposed++;
                nextsymbol();
                _mustbe(ri_parent, ERR_UNMATCHED_PARAN, 0);
            } else
                break;
        }
        CODEFIXUPB(pos, exposed); /* Patch reference */
    }
    MUSTBE_SEMICOLON;
} /* C_procedure */

/* -------------------------------------------------------------- */
/*  PULL [template] ;                                             */
/*      read the next string from the program stack (system       */
/*      data queue), uppercase it, and parse it into variables.   */
/*      If the queue is empty, then data is read from the         */
/*      terminal input buffer. Short for PARSE UPPER PULL.        */
/* -------------------------------------------------------------- */
static void
C_pull(void) {
    _CodeAddByte(OP_RX_PULL);
    _CodeAddByte(OP_UPPER);
    C_template();
} /* C_pull */

/* -------------------------------------------------------------- */
/*  PUSH [expr];                                                  */
/*      push EXPR onto head of the system queue (stack LIFO)      */
/* -------------------------------------------------------------- */
static void
C_push(void) {
    Context *context = (Context *) CMSGetPG();
    if ((context->nextsymbsymbol) == semicolon_sy) {
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr(&((context->rexxnullStr)->key));
        TraceByte(nothing_middle);
    } else
        C_expr(exp_normal);
    _CodeAddByte(OP_RX_PUSH);
} /* C_push */

/* -------------------------------------------------------------- */
/*  QUEUE [expr];                                                 */
/*      add EXPR to the tail of the system queue (stack FIFO)     */
/* -------------------------------------------------------------- */
static void
C_queue(void) {
    Context *context = (Context *) CMSGetPG();
    if ((context->nextsymbsymbol) == semicolon_sy) {
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr(&((context->rexxnullStr)->key));
        TraceByte(nothing_middle);
    } else
        C_expr(exp_normal);
    _CodeAddByte(OP_RX_QUEUE);
} /* C_queue */

/* -------------------------------------------------------------- */
/*  RETURN [expr] ;                                               */
/*      evaluate EXOR and then return the value to the caller.    */
/*      (RETURN has the same effect as EXIT if it is not in an    */
/*      internal routine.)                                        */
/* -------------------------------------------------------------- */
static void
C_return(void) {
    Context *context = (Context *) CMSGetPG();
    if ((context->nextsymbsymbol) == semicolon_sy)
        _CodeAddByte(OP_RETURN);
    else {
        C_expr(exp_normal);
        _CodeAddByte(OP_RETURNF);
    }
} /* C_return */

/* -------------------------------------------------------------- */
/*  SAY [expr];                                                   */
/*      evaluate EXPR and then display the result on the user's   */
/*      console.                                                  */
/* -------------------------------------------------------------- */
static void
C_say(void) {
    C_expr(exp_normal);
    _CodeAddByte(OP_SAY);
} /* C_say */

/* -------------------------------------------------------------- */
/*  SELECT ;                                                      */
/*     WHEN expr [;] THEN [;] inst ;                              */
/*   [ WHEN expr [;] THEN [;] inst ; ]                            */
/*   [ OTHERWISE [;] [inst]... ];                                 */
/*  END ;                                                         */
/*      then WHEN expressions are evaluated in sequence until     */
/*      one results in "1". INSTR, immediately following it, is   */
/*      executed and control leaves the construct. If no          */
/*      EXPR evaluated to "1", control passes to the              */
/*      instructions following the OTHERWISE expression           */
/*      that must then be present.                                */
/* -------------------------------------------------------------- */
static void
C_select(void) {
    enum stat_type old_statement;
    size_t nxt, jmp2end, end;
    int otherwise = FALSE, when = FALSE;
    Context *context = (Context *) CMSGetPG();

    MUSTBE_SEMICOLON;

    old_statement = (context->nextsymbsymbolstat);

    /* Skip jump to the end */
    _CodeAddByte(OP_JMP);
    nxt = _CodeAddWord(0);

    /* add a jump to the end of the structure */
    jmp2end = _CodeAddByte(OP_JMP);
    end = _CodeAddWord(0);

    (context->compileCompileNesting)++;
    for (;;) {
        SKIP_SEMICOLONS;
        if ((context->nextsymbsymbol) == ident_sy) {
            if (!CMP("WHEN")) {
                when = TRUE;
                if (otherwise)
                    (context->lstring_Lerror)(ERR_WHEN_UNCEPTED, 0);
                CODEFIXUP(nxt, (context->compileCompileCodeLen));
                (context->nextsymbsymbolstat) = in_if_init_st;
                CreateClause();
                nextsymbol();
                C_expr(exp_normal);

                SKIP_SEMICOLONS;
                _CodeAddByte(OP_JF);
                nxt = _CodeAddWord(0);
                CreateClause();
                _mustbe(then_sy, ERR_THEN_EXPECTED, 0);
                (context->nextsymbsymbolstat) = in_if_st;

                SKIP_SEMICOLONS;
                C_instr(FALSE);
                _CodeAddByte(OP_JMP);
                _CodeAddWord(jmp2end);
            } else if (!CMP("OTHERWISE")) {
                if (!when)
                    (context->lstring_Lerror)(ERR_WHEN_EXCEPTED, 1,
                                              &(context->nextsymbsymbolstr));
                otherwise = TRUE;
                CODEFIXUP(nxt, (context->compileCompileCodeLen));
                CreateClause();
                nextsymbol();
                (context->nextsymbsymbolstat) = in_do_st;
                for (;;) {
                    SKIP_SEMICOLONS;
                    if (C_instr(TRUE)) /* find END */
                        break;
                }
            } else if (!CMP("END")) {
                if (!when)
                    (context->lstring_Lerror)(ERR_WHEN_EXCEPTED, 1,
                                              &(context->nextsymbsymbolstr));
                if (!otherwise)
                    CODEFIXUP(nxt, (context->compileCompileCodeLen));
                break;
            } else
                (context->lstring_Lerror)(ERR_WHEN_EXCEPTED, ((when) ? 2 : 1),
                                          &(context->nextsymbsymbolstr));
        } else
            (context->lstring_Lerror)(ERR_WHEN_EXCEPTED, 1,
                                      &(context->nextsymbsymbolstr));
    }
    (context->compileCompileNesting)--;
    nextsymbol();
    if ((context->nextsymbsymbol) == ident_sy)
        (context->lstring_Lerror)(ERR_UNMATCHED_END, 4,
                                  &(context->nextsymbsymbolstr));
    CODEFIXUP(end, (context->compileCompileCodeLen));
    (context->nextsymbsymbolstat) = old_statement;
} /* C_select */

/* -------------------------------------------------------------- */
/*  SIGNAL [name] |                                               */
/*         [VALUE] expr |                                         */
/*         <ON | OFF> + ERROR    + [NAME name]  ;                 */
/*                    | FAILURE  |                                */
/*                    | HALT     |                                */
/*                    | NOVALUE  |                                */
/*                    | NOTREADY |                                */
/*                    + SYNTAX   +                                */
/*   o  NAME, jump to the label NAME specified. Any pending       */
/*      instructions, DO ... END, IF, SELECT, and INTERPRET       */
/*      are terminated.                                           */
/*   o  VALUE, may be used for an evaluated label name.           */
/*   o  ON|OFF, enable or disable exception traps. CONDITION      */
/*      must be ERROR, FAILURE, HALT, NOVALUE, NOTREADY or SYNTAX.*/
/*      Control passes to the label of the condition name         */
/*      if the event occurs while ON.                             */
/* -------------------------------------------------------------- */
static void
C_signal(void) {
    int value;
    int cnd = 0;
    void *ptr = NULL;
    Context *context = (Context *) CMSGetPG();

    if ((context->nextsymbsymbol) == ident_sy) {
        if (!CMP("OFF") || (!CMP("ON"))) {
            value = 0;
            if (!CMP("ON")) value = 1;
            nextsymbol();
            if (identCMP("ERROR") ||
                identCMP("FAILURE") ||
                identCMP("HALT") ||
                identCMP("NOVALUE") ||
                identCMP("NOTREADY") ||
                identCMP("SYNTAX")) {
                _AddLabel(FT_LABEL, UNKNOWN_LABEL);
                ptr = SYMBOLADD2LITS_KEY;
                nextsymbol();
                cnd = (value) ? set_signal_opt : unset_signal_opt;
            } else
                (context->lstring_Lerror)(ERR_INV_SUBKEYWORD, 4 - value,
                                          &(context->nextsymbsymbolstr));

            /* must handle correct the name */
            if (identCMP("NAME")) {
                nextsymbol(); /* skip name */
                if (value) {
                    _CodeAddByte(OP_PUSH);
                    _CodeAddPtr(SYMBOLADD2LITS_KEY);
                    TraceByte(nothing_middle);
                    cnd = set_signal_name_opt;
                }
                nextsymbol();
            }
            _CodeAddByte(OP_PUSH);
            _CodeAddPtr(ptr);
            TraceByte(nothing_middle);

            _CodeAddByte(OP_STOREOPT);
            _CodeAddByte(cnd);

            if (cnd == set_signal_name_opt) {
                _CodeAddByte(OP_POP);
                _CodeAddByte(1);
            }
        } else if (!CMP("VALUE")) {
            nextsymbol();
            C_expr(exp_normal);
            _CodeAddByte(OP_SIGNALVAL);
        } else {
            _CodeAddByte(OP_SIGNAL);
            _CodeAddPtr(_AddLabel(FT_LABEL, UNKNOWN_LABEL));
            nextsymbol();
        }
    } else
        (context->lstring_Lerror)(ERR_STRING_EXPECTED, 4,
                                  &(context->nextsymbsymbolstr));
} /* C_signal */

/* -------------------------------------------------------------- */
/*        + +   +      + + +                   + +                */
/*        | | ? | ?... | | | All               | |;               */
/*  TRACE | |   | !... | | | Commands          | |                */
/*        | |   +      + | | Errors            | |                */
/*        | |              | Failure           | |                */
/*        | |   +      + | | Intermediates     | |                */
/*        | | ! | ?... | | | Labels            | |                */
/*        | |   | !... | | | Normal            | |                */
/*        | +   +      + + | Off               | |                */
/*        |                | Results           | |                */
/*        |                | Scan              | |                */
/*        |                +                   + |                */
/*        | (number)                             |                */
/*        +                                      +                */
/*                                                                */
/*   ______________                                               */
/*                                                                */
/*  Or, alternatively:                                            */
/*  TRACE [ string ] ;                                            */
/*                                                                */
/*      if OPTION is numeric, then (if negative) inhibit tracing  */
/*      for a number of clauses, else (if positive) inhibit debug */
/*      mode for a number of clauses. Otherwise, trace            */
/*      according to first character or OPTION:                   */
/*                                                                */
/*      A   (All) trace all clauses.                              */
/*      C   (Commands) trace all commands.                        */
/*      E   (Error) trace commands with non-zero return codes     */
/*          after execution.                                      */
/*      I   (Intermediates) trace intermediates evaluation        */
/*          results and name substitutions also.                  */
/*      L   (Labels) trace only labels.                           */
/*      N   (Negative or Normal) trace commands with              */
/*          negative return codes after execution (default        */
/*          setting).                                             */
/*      O   (Off) no trace.                                       */
/*      R   (Results) trace all clauses and exceptions.           */
/*      S   (Scan) display rest of program without any            */
/*          execution (shows control nesting).                    */
/*      !   turn command inhibition on or off, and trace          */
/*          according to next character.                          */
/*     null restores the default tracing actions.                 */
/*                                                                */
/*     TRACE VALUE expr may be used for an evaluated              */
/*     trace setting.                                             */
/* -------------------------------------------------------------- */
static void
C_trace(void) {
    Context *context = (Context *) CMSGetPG();
    if (identCMP("VALUE")) {
        nextsymbol();
        C_expr(exp_tmp);
    } else if ((context->nextsymbsymbol) == ident_sy ||
               (context->nextsymbsymbol) == literal_sy) {
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr(SYMBOLADD2LITS_KEY);
        TraceByte(nothing_middle);
        nextsymbol();
    } else
        (context->lstring_Lerror)(ERR_STRING_EXPECTED, 6,
                                  &(context->nextsymbsymbolstr));

    _CodeAddByte(OP_STOREOPT);
    _CodeAddByte(trace_opt);
} /* C_trace */

/* -------------------------------------------------------------- */
/*  UPPER name [name]... ;                                        */
/*      translate the values of the specified individual          */
/*      variables to uppercase.                                   */
/* -------------------------------------------------------------- */
static void
C_upper(void) {
    void *sym;
    Context *context = (Context *) CMSGetPG();

    while ((context->nextsymbsymbol) == ident_sy) {
        sym = SYMBOLADD2LITS;
        _CodeAddByte(OP_LOAD);
        _CodeAddPtr(sym);
        TraceByte(variable_middle);
        _CodeAddByte(OP_UPPER);
        _CodeAddByte(OP_POP);
        _CodeAddByte(1);
        nextsymbol();
    }
} /* C_upper */

/* -------------------------------------------------------------- */
/*  [expr] ;                                                      */
/* Execute a host command according to environment                */
/* -------------------------------------------------------------- */
static void
C_HostCmd(void) {
    _CodeAddByte(OP_LOADOPT);
    _CodeAddByte(environment_opt);
    C_expr(exp_normal);
    _CodeAddByte(OP_SYSTEM);
} /* C_HostCmd */

/* -------------------------------------------------------------- */
/*  name = [expr]                                                 */
/*       is an assignment: the variable name is set to the value  */
/*       of expr.                                                 */
/* -------------------------------------------------------------- */
static int
C_chk4assign(void) {
    Context *context = (Context *) CMSGetPG();
    if (*(context->nextsymbsymbolptr) == '=') {
        if ((context->nextsymbsymbol) == literal_sy) {
            /* produce an error */
            if (IN_RANGE('0', LSTR((context->nextsymbsymbolstr))[0], '9'))
                (context->lstring_Lerror)(ERR_INVALID_START,
                                          (_Lisnum(
                                                  &(context
                                                          ->nextsymbsymbolstr)) !=
                                           LSTRING_TY) ? 1 : 2,
                                          &(context->nextsymbsymbolstr));
            else if (LSTR((context->nextsymbsymbolstr))[0] == '.')
                (context->lstring_Lerror)(ERR_INVALID_START, 3,
                                          &(context->nextsymbsymbolstr));
        }
        return TRUE;
    } else
        return FALSE;
} /* C_chk4assign */

/* ------ assigment ------- */
static void
C_assign(void) {
    void *var;
    bool stem;
    Context *context = (Context *) CMSGetPG();

    var = SYMBOLADD2LITS;
    stem = !(context->nextsymbsymbolhasdot) &&
           (LSTR((context->nextsymbsymbolstr))[
                    LLEN((context->nextsymbsymbolstr)) - 1] == '.');
    nextsymbol();
    _CodeAddByte(OP_CREATE); /* CREATE  */
    _CodeAddPtr(var); /* the variable */
    _mustbe(eq_sy, ERR_INVALID_EXPRESSION, 0);
    if (C_expr(exp_assign)) {
        _CodeAddByte(OP_POP);
        _CodeAddByte(1);
    }
    if (stem) {
        _CodeAddByte(OP_ASSIGNSTEM);
        _CodeAddPtr(var);
    }
} /* C_assign */

/* --------------- C_instr --------------- */
static bool
C_instr(bool until_end) {
    int first, middle, last, cmp, found;
    Context *context = (Context *) CMSGetPG();

    /* -- create a new clause ptr -- */
    (context->compileCompileNesting)++;
    (context->compile_checked_semicolon) = FALSE;
    if ((context->nextsymbsymbol) == exit_sy) {
        if ((context->compile_str_interpreted))
            _CodeAddByte(OP_INTER_END);
        else {
            CreateClause();
            _CodeAddByte(OP_IEXIT);
        }
        (context->compileCompileNesting)--;
        longjmp((context->rexx_error_trap), 1); /* Everything is Ok */
    }

    if (!identCMP("END")) CreateClause();

    if ((context->nextsymbsymbol) == label_sy)
        (context->lstring_Lerror)(ERR_UNEXPECTED_LABEL, 0);

    if (C_chk4assign())
        C_assign();
    else if ((context->nextsymbsymbol) == ident_sy) {
        if (!CMP("END")) {
            (context->compileCompileNesting)--;
            if (until_end)  /* Semicolon is NOT deleted */
                return TRUE;
            else
                (context->lstring_Lerror)(ERR_UNMATCHED_END, 0);
        }

        /* Binary search for the instruction */
        first = found = 0;
        last = DIMENSION(statements_list) - 1;
        while (first <= last) {
            middle = (first + last) / 2;
            if ((cmp = CMP(statements_list[middle].name)) == 0) {
                found = 1;
                break;
            } else if (cmp < 0)
                last = middle - 1;
            else
                first = middle + 1;
        }

        if (found) {
            if (statements_list[middle].func != C_error)
                nextsymbol();
            (*statements_list[middle].func)();
        } else
            C_HostCmd();
    } else
        C_HostCmd();

    if (!(context->compile_checked_semicolon))  /* if noone has checked for a */
        MUSTBE_SEMICOLON; /* semicolon then check it now */

    (context->compileCompileNesting)--;
    return FALSE;
} /* C_instr */

/* ------------- RxInitCompile -------------- */
void __CDECL
RxInitCompile(RxFile *rxf, PLstr src) {
    int i;
    Context *context = (Context *) CMSGetPG();

    (context->compile_str_interpreted) = (src != NULL);

    /* copy to our static variables */
    (context->compileCompileCode) = (context->rexx_code);
    (context->compileCompileRxFile) = rxf;

    /* Initialise (context->compile_Loop) Queue for LEAVE & ITERATE jmp points */
    DQINIT((context->compile_Loop));

    /* Initialize code string */
    Lfx((context->compileCompileCode), CODE_INC);
    (context->compileCompileCodeLen) = LLEN(*(context->compileCompileCode));
    (context->compileCompileCodePtr) =
            (byte *) LSTR(*(context->compileCompileCode)) +
            (context->compileCompileCodeLen);

    if ((context->compileCompileClause) == NULL) {
        (context->compileCompileCurClause) = 0;
        (context->compileCompileClauseItems) = CLAUSE_INC;
        (context->compileCompileClause) = (Clause *) MALLOC(
                CLAUSE_INC * sizeof(Clause), "Clause");
    }

    /* initialise nesting */
    (context->compileCompileNesting) = 0;

    /* Initialise next (context->nextsymbsymbol) */
    if ((context->compile_str_interpreted)) InitNextsymbol(src);
    else InitNextsymbol(&(rxf->file));

} /* RxInitCompile */

/* -------------- RxCompile ----------------- */
int __CDECL
RxCompile(void) {
    int jc;
    Context *context = (Context *) CMSGetPG();

    /* --- trap eof --- */
    if ((jc = setjmp((context->rexx_error_trap))) != 0) {
        /* Set the length of the Code and exit */
        LLEN(*(context->compileCompileCode)) = (context->compileCompileCodeLen);
        goto CompEnd;
    }

    /* ---- main loop ---- */
    /* will exit only with a longjmp */
    while (1) {
        SKIP_SEMICOLONS;
        /* labels are accepted only in main loop */
/*** WARNING labels can be also with DOT .   test.label ******/
        if ((context->nextsymbsymbol) == label_sy) {
            (context->compileCompileNesting)++;
            /* --- if prev was NEWCLAUSE */
            if ((context->compileCompileCurClause) &&
                (context->compileCompileClause)[
                        (context->compileCompileCurClause) - 1].code ==
                (context->compileCompileCodeLen) - CLAUSESTEP)
                _AddLabel(FT_LABEL,
                          (context->compileCompileCodeLen) - CLAUSESTEP);
            else
                _AddLabel(FT_LABEL, (context->compileCompileCodeLen));
            CreateClause();
            nextsymbol();
            SKIP_SEMICOLONS;
            if (identCMP("PROCEDURE")) {
                nextsymbol();
                C_procedure();
            }
            (context->compileCompileNesting)--;
        } else
            C_instr(FALSE);
    }

    CompEnd:
    /* Check here for various errors */
    if ((context->compile_Loop).items > 0)
        DQFlush(&(context->compile_Loop), FREE);

    /* mark the end of clauses */
    (context->compileCompileClause)[(context->compileCompileCurClause)].ptr = 0;
    (context->compileCompileClause)[(context->compileCompileCurClause)].line =
            0;
    (context->compileCompileClause)[(context->compileCompileCurClause)].code =
            0;
    (context->compileCompileClause)[(context->compileCompileCurClause)].fptr =
            0;

    /* ---- Mark the End of compilation ----- */
    (context->nextsymbsymbolptr) = NULL;  /* mark end of compilation */
    (context->compileCompileCodePtr) = NULL;

    if (jc == 1) (context->rexxrxReturnCode) = 0;

    return (context->rexxrxReturnCode);
} /* RxCompile */
