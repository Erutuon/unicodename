// This contains things that are shared between two or more of
// main.c, unicodename.c, and aliases.c.
#ifndef UNICODENAME_COMMON_H
#define UNICODENAME_COMMON_H

#include <stdlib.h> // for realloc, free
#include <stdio.h> // for vsnprintf, perror

#include "rasprintf.h"

#define MEM_ERR "Not enough memory"
#define MEM_ERR_RETURN_NULL(pointer) if (pointer == NULL) { perror(MEM_ERR); return NULL; }

#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))

#define CODEPOINT_VALID(cp) BETWEEN((cp), 0, 0x10FFFF)

#endif