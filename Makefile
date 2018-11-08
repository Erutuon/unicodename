CFLAGS = -Wall -O2 $(MYCFLAGS) -I.
LDFLAGS = -Wall -O2 $(MYLDFLAGS)

ifeq ($(OS), Windows_NT)
EXE = unicodename.exe
else # assume Unix
EXE ?= unicodename
endif

$(EXE): main.o unicodename.o aliases.o rasprintf.o
	$(CC) $(CFLAGS) rasprintf.o aliases.o unicodename.o main.o -o $(EXE)

unicodename.o: unicodename.c unicodename.h aliases.h common.h rasprintf.h
aliases.o: aliases.c aliases.h common.h rasprintf.h
rasprintf.o: rasprintf.c rasprintf.h
main.o: main.c common.h unicodename.h rasprintf.h

clean:
	rm -f ./*.o ./unicodename.exe