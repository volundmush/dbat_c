#pragma once


#if 0
#include "config.h"

#include <sys/types.h> /* For size_t */
#endif

#ifndef HAVE_SNPRINTF
int snprintf(char *str, size_t count, const char *fmt, ...) __attribute__ ((format(printf, 3, 4)));
#endif /* !HAVE_SNPRINTF */

#ifndef HAVE_VSNPRINTF
int vsnprintf(char *str, size_t count, const char *fmt, va_list args);
#endif /* !HAVE_SNPRINTF */
