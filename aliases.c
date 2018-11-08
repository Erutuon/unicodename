#include <stdlib.h>
#include <stdio.h>
#include <limits.h> // for INT_MAX

#include "common.h"
#include "aliases.h"

#define FREE_AND_NULL(mem) (free(mem), (mem) = NULL)
#define IF_NOT_NULL_FREE_AND_NULL(mem) ((mem) != NULL ? FREE_AND_NULL(mem) : NULL)

static bool aliases_list_realloc_aliases (aliases_list * aliases, int size) {
	void * old_aliases_list = aliases->list;
	aliases->list = realloc(aliases->list, size * sizeof *aliases->list);
	if (aliases->list != NULL) {
		aliases->size = size; return true;
	}
	else {
		perror(MEM_ERR); free(old_aliases_list); aliases_list_free(&aliases);
		return false;
	}
}

static bool aliases_list_expand (aliases_list * aliases) {
	if (aliases->size > INT_MAX / 2) {
		fprintf(stderr, "Integer overflow");
		return false;
	}
	return aliases_list_realloc_aliases(aliases, aliases->size * 2);
}

aliases_list * aliases_list_new () {
	aliases_list * aliases = malloc(sizeof *aliases);
	MEM_ERR_RETURN_NULL(aliases);
	
	aliases->list = NULL;
	if (!aliases_list_realloc_aliases(aliases, 2)) {
		free(aliases); perror(MEM_ERR); return NULL;
	}
	
	aliases->length = 0;
	
	return aliases;
}

bool aliases_list_add (aliases_list * aliases, char * alias) {
	++aliases->length;
	if (aliases->length > aliases->size && !aliases_list_expand(aliases))
		return false;
	aliases->list[aliases->length - 1] = alias;
	return true;
}

void aliases_list_free (aliases_list * * aliases) {
	if (*aliases != NULL) {
		if ((*aliases)->list != NULL) {
			for (int i = 0; i < (*aliases)->length; ++i)
				IF_NOT_NULL_FREE_AND_NULL((*aliases)->list[i]);
			
			FREE_AND_NULL((*aliases)->list);
		}
		FREE_AND_NULL(*aliases);
	}
}