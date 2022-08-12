//
// Created by basti on 10/22/2021.
//

#ifndef HAVE_LIBBSD
#include "stringutils.h"
#include <string.h>

size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
  strncpy(dest, source, totalsize - 1);	/* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}

#endif