#ifndef STRTONUM_H
#define STRTONUM_H

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#define	INVALID		1
#define	TOOSMALL	2
#define	TOOLARGE	3


long long
strtonum(const char *numstr, long long minval, long long maxval,
    const char **errstrp);

#endif