#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

char * rasprintf (char * s, const char * format, ...) {
	va_list args;
	va_start(args, format);
	
	// Need to copy args?
	
	int len = vsnprintf(NULL, 0, format, args);
	
	char * printed = realloc(s, len + 1);
	
	if (printed != NULL) {
		if (!((unsigned) vsnprintf(printed, len + 1, format, args) < len + 1))
			free(printed), printed = NULL;
	}
	else { free(s); perror("Not enough memory"); }
	
	va_end(args);
	
	return printed;
}