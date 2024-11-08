#ifndef brexx_options_h
#define brexx_options_h

typedef struct OptList_st {
    int count;     /* Number of option words */
    char *list[];  /* List of option words */
    char *words;   /* Word contents */
} OptList;

void __CDECL ParseOptions(const PLstr value);

int __CDECL CheckOption(char *value);

#endif