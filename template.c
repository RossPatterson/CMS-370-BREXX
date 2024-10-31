/*
 * $Id: template.c,v 1.7 2008/07/15 07:40:25 bnv Exp $
 * $Log: template.c,v $
 * Revision 1.7  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.6  2004/08/16 15:29:30  bnv
 * Changed: name of mnemonic operands from xxx_mn to O_XXX
 *
 * Revision 1.5  2003/08/04 01:29:58  bnv
 * Insert TMP before the NEG!
 *
 * Revision 1.4  2002/06/11 12:37:38  bnv
 * Added: CDECL
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

#include "lerror.h"
#include "lstring.h"

#include "rexx.h"
#include "trace.h"
#include "compile.h"
#include <cmssys.h>

/* ----------- vrefp --------- */
/* variable reference position */
static void
vrefp(void) {
    Context *context = (Context *) CMSGetPG();
    nextsymbol(); /* skip left parenthesis */

    if ((context->nextsymbsymbol) != ident_sy)
        (context->lstring_Lerror)(ERR_STRING_EXPECTED, 7,
                                  &(context->nextsymbsymbolstr));

    _CodeAddByte(OP_LOAD);
    _CodeAddPtr(SYMBOLADD2LITS);
    TraceByte(nothing_middle);
    nextsymbol();

    _mustbe(ri_parent, ERR_INV_VAR_REFERENCE, 1);
} /* vrefp */

/* -------- position ------- */
/* variable reference position */
static void
position(void) {
    int type;
    Context *context = (Context *) CMSGetPG();

    if ((context->nextsymbsymbol) == le_parent)
        vrefp();
    else if ((context->nextsymbsymbol) == literal_sy) {
        type = _Lisnum(&(context->nextsymbsymbolstr));
        if (type == LREAL_TY)
            (context->lstring_Lerror)(ERR_INVALID_INTEGER, 4,
                                      &(context->nextsymbsymbolstr));
        else if (type == LSTRING_TY)
            (context->lstring_Lerror)(ERR_INVALID_TEMPLATE, 2,
                                      &(context->nextsymbsymbolstr));
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr(SYMBOLADD2LITS_KEY);
        TraceByte(nothing_middle);
        nextsymbol();
    } else
        (context->lstring_Lerror)(ERR_INVALID_TEMPLATE, 2,
                                  &(context->nextsymbsymbolstr));
    _CodeAddByte(OP_TOINT);
} /* position */

static void
wordparse_begin(CTYPE *jump_past_wordparse, CTYPE *wp_begin_dest) {
    Context *context = (Context *) CMSGetPG();

    /* The first target after a trigger is the beginning of the WORDPARSE(data) function in the Rexx standard. */
    /* Create a jump to the next trigger, and remember where we are so we can jump back here after processing it. */
    if (*jump_past_wordparse == 0) {
        _CodeAddByte(OP_JMP);
        *jump_past_wordparse = _CodeAddWord(0);
        CODEFIXUP(*jump_past_wordparse, (context->compileCompileCodeLen)); /* Will get replaced by wordparse_end(), otherwise a NOP */
        *wp_begin_dest = (context->compileCompileCodeLen);
    }
}

static void
wordparse_end(CTYPE *jump_past_trigger, CTYPE *jump_past_wordparse_dest) {
    Context *context = (Context *) CMSGetPG();

    /* The first trigger after a target is the end of the WORDPARSE(data) function in the Rexx standard. */
    /* Create a jump from the last target to after this trigger and patch the pre-target jump to come here. */
    if (*jump_past_wordparse_dest != 0) {
        _CodeAddByte(OP_JMP);
        *jump_past_trigger = _CodeAddWord(0);
        CODEFIXUP(*jump_past_wordparse_dest, (context->compileCompileCodeLen));
        *jump_past_wordparse_dest = 0;
    }
}

static void
wordparse_invoke(CTYPE *wp_begin_dest, CTYPE *jump_past_trigger) {
     Context *context = (Context *) CMSGetPG();

    /* After a trigger, the Rexx standard calls the WORDPARSE(data) function to parse the string into the targets. */
    /* Create a jump back to the first target and patch the post-target jump to come here. */
    if ((*wp_begin_dest != 0) & (*jump_past_trigger != 0)) {
        _CodeAddByte(OP_JMP);
        _CodeAddWord(*wp_begin_dest);
        *wp_begin_dest = 0;
        CODEFIXUP(*jump_past_trigger, (context->compileCompileCodeLen));
        *jump_past_trigger = 0;
    }
}

/* -------------------------------------------------------------- */
/*  template_list := template | [template] ',' [template_list]    */
/*  template   := (trigger | target | Msg38.1)+                   */
/*  target     := VAR_SYMBOL | '.'                                */
/*  trigger    := pattern | positional                            */
/*  pattern    := STRING | vrefp                                  */
/*  vrefp      := '(' (VAR_SYMBOL | Msg19.7) (')' | Msg46.1)      */
/*  positional := absolute_pos | relative_pos                     */
/*  absolute_pos := NUMBER | '=' position                         */
/*  position   := NUMBER | vrefp  | Msg38.2                       */
/*  relative_pos := ('+' | '-') position                          */
/* -------------------------------------------------------------- */
void __CDECL
C_template(void) {
    void *target_ptr = NULL;
    bool trigger, dot = FALSE;
    bool sign;
    int type;
    CTYPE pos;
    CTYPE pre_target_jump=0, post_target_jump=0, post_trigger_jump=0, post_trigger_dest=0;
    Context *context = (Context *) CMSGetPG();

    _CodeAddByte(OP_PARSE);
    while (((context->nextsymbsymbol) != semicolon_sy) &&
           ((context->nextsymbsymbol) != comma_sy)) {
        trigger = FALSE;
        switch ((context->nextsymbsymbol)) {
            case ident_sy:
            case dot_sy:
                wordparse_begin(&pre_target_jump, &post_trigger_dest);
                if (target_ptr || dot) {
                    /* trigger space */
                    trigger = TRUE;
                    _CodeAddByte(OP_TR_SPACE);
                    /* do not go to next (context->nextsymbsymbol) */
                } else {
                    if ((context->nextsymbsymbol) == ident_sy)
                        target_ptr = SYMBOLADD2LITS;
                    else
                        dot = TRUE;
                    nextsymbol();
                }
                break;

            case minus_sy:
            case plus_sy:
                wordparse_end(&post_target_jump, &pre_target_jump);
                trigger = TRUE;
                sign = ((context->nextsymbsymbol) == minus_sy);
                nextsymbol();
                pos = (context->compileCompileCodeLen);
                position();
                if (sign) {
                    _CodeInsByte(pos, OP_PUSHTMP);
                    _CodeAddByte(OP_NEG);
                    TraceByte(nothing_middle);
                }
                else {
                    _CodeInsByte(pos, OP_PUSHTMP);
                    _CodeAddByte(OP_PLUS);
                    TraceByte(nothing_middle);
                }
                _CodeAddByte(OP_TR_REL);
                wordparse_invoke(&post_trigger_dest, &post_target_jump);
                break;

            case literal_sy:
                wordparse_end(&post_target_jump, &pre_target_jump);
                trigger = TRUE;

                if ((context->nextsymbsymbolisstr)) {
                    _CodeAddByte(OP_PUSH);
                    _CodeAddPtr(SYMBOLADD2LITS_KEY);
                    TraceByte(nothing_middle);
                    _CodeAddByte(OP_TR_LIT);
                    nextsymbol();
                } else {
                    type = _Lisnum(&(context->nextsymbsymbolstr));
                    if (type == LREAL_TY)
                        (context->lstring_Lerror)(ERR_INVALID_INTEGER, 4,
                                                  &(context
                                                          ->nextsymbsymbolstr));
                    else if (type == LSTRING_TY)
                        (context->lstring_Lerror)(ERR_INVALID_TEMPLATE, 1,
                                                  &(context
                                                          ->nextsymbsymbolstr));

                    _CodeAddByte(OP_PUSH);
                    _CodeAddPtr(SYMBOLADD2LITS_KEY);
                    TraceByte(nothing_middle);
                    _CodeAddByte(OP_TOINT);
                    _CodeAddByte(OP_TR_ABS);
                    nextsymbol();
                }
                wordparse_invoke(&post_trigger_dest, &post_target_jump);
                break;

            case le_parent:
                wordparse_end(&post_target_jump, &pre_target_jump);
                trigger = TRUE;
                vrefp();
                _CodeAddByte(OP_TR_LIT);
                wordparse_invoke(&post_trigger_dest, &post_target_jump);
                break;

            case eq_sy:
                wordparse_end(&post_target_jump, &pre_target_jump);
                trigger = TRUE;
                nextsymbol();
                position();
                _CodeAddByte(OP_TOINT);
                _CodeAddByte(OP_TR_ABS);
                wordparse_invoke(&post_trigger_dest, &post_target_jump);
                break;

            default:
                (context->lstring_Lerror)(ERR_INVALID_TEMPLATE, 0);
        } /* end of switch */
        if (trigger) {
            if (target_ptr) {
                _CodeAddByte(OP_CREATE);
                _CodeAddPtr(target_ptr);
                _CodeAddByte(OP_PVAR);
                TraceByte(other_middle);
                target_ptr = NULL;
            } else if (dot) {
                _CodeAddByte(OP_PDOT);
                TraceByte(dot_middle);
                dot = FALSE;
            }
        }
    } /* end of while */

    wordparse_end(&post_target_jump, &pre_target_jump);
    if (target_ptr) { /* assign the remaining part */
        _CodeAddByte(OP_TR_END);
        wordparse_invoke(&post_trigger_dest, &post_target_jump);
        _CodeAddByte(OP_CREATE);
        _CodeAddPtr(target_ptr);
        _CodeAddByte(OP_PVAR);
        TraceByte(other_middle);
    } else if (dot) {
        _CodeAddByte(OP_TR_END);
        wordparse_invoke(&post_trigger_dest, &post_target_jump);
        _CodeAddByte(OP_PDOT);
        TraceByte(dot_middle);
    }

    _CodeAddByte(OP_POP);
    _CodeAddByte(1);
} /* C_template */
