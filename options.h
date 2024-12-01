#ifndef __BREXX_OPTIONS_H__
#define __BREXX_OPTIONS_H__

typedef enum opt_values {
    OPT_STORAGE_DECIMAL = 1UL,
} OptValues;


#ifdef __CMS__
#define ParseOptions _optprs
#endif

#define CHECK_OPT(option) ((context->interpre_options) && (option))

void __CDECL ParseOptions(const PLstr value);

#endif