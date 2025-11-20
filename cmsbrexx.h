/* Stuff that's just for CMS */

#ifndef __CMSBREXX_H
#define __CMSBREXX_H

int __CDECL __HOSTCM(const PLstr cmd, const PLstr env);

int __CDECL HOSTFNC(const char *name, const int argc, char **argv,
         const int *lenv, const int calltype, char **result,
         const char *filetype);

#endif
