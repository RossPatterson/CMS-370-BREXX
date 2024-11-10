#ifndef __BREXX_OPTIONS_H__
#define __BREXX_OPTIONS_H__

typedef struct OptList_st {
    int count;    /* Number of option words */
    char **list;  /* List of option words */
    char *words;  /* Word contents */
} OptList;

#define ParseOptions _optprs
#define CheckOption _optchk

void __CDECL ParseOptions(const PLstr value);

int __CDECL CheckOption(char *value);

#endif