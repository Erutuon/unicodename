#ifndef ALIASES_H
#define ALIASES_H

#include <stdbool.h>

typedef struct aliases_list {
	char * * list;
	int length, size;
} aliases_list;

void aliases_list_free (aliases_list * * aliases);

// return newly allocated aliases_list
aliases_list * aliases_list_new (void);

// add alias to aliases.
// alias must be an allocated null-terminated string.
bool aliases_list_add (aliases_list * aliases, char * alias);
#endif