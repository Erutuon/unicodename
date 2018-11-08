#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

char * rasprintf (char * s, const char * format, ...) {
	va_list args;
	va_start(args, format);
	
	int len = vsnprintf(NULL, 0, format, args);
  
	va_end(args);
	
	char * printed = realloc(s, len + 1);
	
	if (printed != NULL) {
		va_start(args, format);
		
		if (!((unsigned) vsnprintf(printed, len + 1, format, args) < len + 1))
			free(printed), printed = NULL;
		
		va_end(args);
	}
	else { free(s); perror("Not enough memory"); }
	
	return printed;
}