#ifndef RASPRINTF_H
#define RASPRINTF_H

char * rasprintf (char * s, const char * format, ...);

#define ASPRINTF(...) (rasprintf(NULL, __VA_ARGS__))

#endif