#include "context.h"

#ifdef __CMS__
#include <cmssys.h>
#endif

static int iprint(const char *name, const int argc, const int trace,
                  const CIPTYPE *code, const int offset);

void __CDECL
disasm() {
    int offset;
    CIPTYPE *code;
    CIPTYPE inst;

    Context *context = (Context *) CMSGetPG();
    code = (CIPTYPE *) LSTR(*(context->rexx_code));
    printf("address opcode     operation   args\n");
    for (offset = 0; offset < context->compileCompileCodeLen; offset++) {
        inst = code[offset];
        printf("%04d    ", offset);
        if (inst < 0 | inst > 255) printf("0x%08x ", inst);
        else printf("%03d        ", inst);
#define P(name, argc, trace) offset = iprint(name, argc, trace, code, offset)
        switch (inst) {
            case OP_NEWCLAUSE:   P("NEWCLAUSE",   0, 0); break;
            case OP_NOP:         P("NOP",         0, 0); break;
            case OP_PUSH:        P("PUSH",        1, 1); break;
            case OP_PUSHTMP:     P("PUSHTMP",     0, 0); break;
            case OP_POP:         P("POP",         1, 0); break;
            case OP_DUP:         P("DUP",         1, 0); break;
            case OP_COPY:        P("COPY",        0, 0); break;
            case OP_COPY2TMP:    P("COPY2TMP",    0, 0); break;
            case OP_PATCH:       P("PATCH",       2, 0); break;
            case OP_RAISE:       P("RAISE",       3, 0); break;
            case OP_LOADARG:     P("LOADARG",     1, 0); break;
            case OP_LOADOPT:     P("LOADOPT",     1, 0); break;
            case OP_STOREOPT:    P("STOREOPT",    1, 0); break;
            case OP_LOAD:        P("LOAD",        1, 1); break;
            case OP_CREATE:      P("CREATE",      1, 0); break;
            case OP_DROP:        P("DROP",        1, 1); break;
            case OP_DROPIND:     P("DROPIND",     0, 1); break;
            case OP_ASSIGNSTEM:  P("ASSIGNSTEM",  1, 0); break;
            case OP_BYINIT:      P("BYINIT",      1, 0); break;
            case OP_FORINIT:     P("FORINIT",     0, 0); break;
            case OP_DECFOR:      P("DECFOR",      1, 0); break;
            case OP_TOINT:       P("TOINT",       0, 0); break;
            case OP_LOWER:       P("LOWER",       0, 0); break;
            case OP_UPPER:       P("UPPER",       0, 0); break;
            case OP_SIGNAL:      P("SIGNAL",      1, 0); break;
            case OP_SIGNALVAL:   P("SIGNALVAL",   1, 0); break;
            case OP_JMP:         P("JMP",         1, 0); break;
            case OP_JF:          P("JF",          1, 0); break;
            case OP_JT:          P("JT",          1, 0); break;
            case OP_CALL:        P("CALL",        6, 1); break;
            case OP_RETURN:      P("RETURN",      0, 0); break;
            case OP_RETURNF:     P("RETURNF",     0, 0); break;
            case OP_INTERPRET:   P("INTERPRET",   1, 0); break;
            case OP_INTER_END:   P("INTER_END",   0, 0); break;
            case OP_PROC:        P("PROC",        2, 1); break;
            case OP_SAY:         P("SAY",         0, 0); break;
            case OP_SYSTEM:      P("SYSTEM",      0, 0); break;
            case OP_EXIT:        P("EXIT",        0, 0); break;
            case OP_IEXIT:       P("IEXIT",       0, 0); break;
            case OP_PARSE:       P("PARSE",       0, 0); break;
            case OP_PVAR:        P("PVAR",        0, 0); break;
            case OP_PDOT:        P("PDOT",        0, 0); break;
            case OP_TR_SPACE:    P("TR_SPACE",    0, 0); break;
            case OP_TR_LIT:      P("TR_LIT",      0, 0); break;
            case OP_TR_ABS:      P("TR_ABS",      0, 0); break;
            case OP_TR_REL:      P("TR_REL",      0, 0); break;
            case OP_TR_END:      P("TR_END",      0, 0); break;
            case OP_RX_QUEUE:    P("RX_QUEUE",    0, 0); break;
            case OP_RX_PUSH:     P("RX_PUSH",     0, 0); break;
            case OP_RX_PULL:     P("RX_PULL",     0, 0); break;
            case OP_RX_EXTERNAL: P("RX_EXTERNAL", 0, 0); break;
            case OP_EQ:          P("EQ",          0, 1); break;
            case OP_NE:          P("NE",          0, 1); break;
            case OP_GT:          P("GT",          0, 1); break;
            case OP_GE:          P("GE",          0, 1); break;
            case OP_LT:          P("LT",          0, 1); break;
            case OP_LE:          P("LE",          0, 1); break;
            case OP_DEQ:         P("DEQ",         0, 1); break;
            case OP_DNE:         P("DNE",         0, 1); break;
            case OP_DGT:         P("DGT",         0, 1); break;
            case OP_DGE:         P("DGE",         0, 1); break;
            case OP_DLT:         P("DLT",         0, 1); break;
            case OP_DLE:         P("DLE",         0, 1); break;
            case OP_TEQ:         P("TEQ",         0, 1); break;
            case OP_TNE:         P("TNE",         0, 1); break;
            case OP_TDEQ:        P("TDEQ",        0, 1); break;
            case OP_TDNE:        P("TDNE",        0, 1); break;
            case OP_TGT:         P("TGT",         0, 1); break;
            case OP_TGE:         P("TGE",         0, 1); break;
            case OP_TLT:         P("TLT",         0, 1); break;
            case OP_TLE:         P("TLE",         0, 1); break;
            case OP_NOT:         P("NOT",         0, 1); break;
            case OP_AND:         P("AND",         0, 1); break;
            case OP_OR:          P("OR",          0, 1); break;
            case OP_XOR:         P("XOR",         0, 1); break;
            case OP_CONCAT:      P("CONCAT",      0, 1); break;
            case OP_BCONCAT:     P("BCONCAT",     0, 1); break;
            case OP_NEG:         P("NEG",         0, 1); break;
            case OP_PLUS:        P("PLUS",        0, 1); break;
            case OP_INC:         P("INC",         0, 1); break;
            case OP_DEC:         P("DEC",         0, 1); break;
            case OP_ADD:         P("ADD",         0, 1); break;
            case OP_SUB:         P("SUB",         0, 1); break;
            case OP_MUL:         P("MUL",         0, 1); break;
            case OP_DIV:         P("DIV",         0, 1); break;
            case OP_IDIV:        P("IDIV",        0, 1); break;
            case OP_MOD:         P("MOD",         0, 1); break;
            case OP_POW:         P("POW",         0, 1); break;
            default:             P("",            0, 0);
        }
#undef P
        printf("\n");
    }
}
static int iprint(const char *name, const int argc, const int trace,
                  const CIPTYPE *code, const int offset) {
    int argn, argval, retval;

    printf("%-11s", name);
    for (argn = 1; argn <= argc; argn++) {
        printf(" %d=", argn);
        argval = code[offset + argn];
        if (argval < 0 | argval > 8192) {
            printf("0x%08x", argval);
        } else {
            printf("%04d", argval);
        }
    }
    retval = offset + argc;
    if (trace) {
       printf(" trace=0x%02x", code[++retval]);
    }
    return retval;
}
