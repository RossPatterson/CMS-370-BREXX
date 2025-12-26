#ifndef __BREXX_OPTIONS_H__
#define __BREXX_OPTIONS_H__

typedef enum opt_values {
    OPT_NONE                             =             0UL,
    // Individual options:
    OPT_STORAGE_DECIMAL                  =             1UL,
#if ALLOW_DECNUMBER
    OPT_DECIMAL_MATH                     =             2UL,
#endif
    // Composite values:
    OPT_COMPAT_BREXX_1_0 = OPT_STORAGE_DECIMAL,
    OPT_COMPAT_BREXX_1_1 = OPT_NONE
    // OPT_COMPAT_BREXX_x_y = OPT_aaa + OPT_bbb + ...,
} OptValues;


#ifdef __CMS__
#define ParseOptions _optprs
#endif

#define CHECK_OPT(option) ((context->interpre_options) && (option))

void __CDECL ParseOptions(const PLstr value);

#endif