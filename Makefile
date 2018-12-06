CFLAGS = -Wall -O2 $(MYCFLAGS) -I.

# To set directory in which UnicodeData.txt and NameAliases.txt are found, use this command,
# but replace <your Unicode data directory> with the directory name.
# It doesn't need a trailing slash.
# make 'MYCFLAGS=-DUCD_DIRECTORY=\"<your Unicode data directory>\"'

ifeq ($(OS), Windows_NT)
EXE_EXT = .exe
endif

EXE ?= unicodename$(EXE_EXT)

INSTALL_DIR ?= /usr/local/bin

$(EXE): main.o unicodename.o aliases.o rasprintf.o
	$(CC) $(CFLAGS) rasprintf.o aliases.o unicodename.o main.o -o $(EXE)

unicodename.o: unicodename.c unicodename.h aliases.h common.h rasprintf.h
aliases.o: aliases.c aliases.h common.h rasprintf.h
rasprintf.o: rasprintf.c rasprintf.h
main.o: main.c common.h unicodename.h rasprintf.h

install:
	mv unicodename $(INSTALL_DIR)

clean:
	rm -f ./*.o ./$(EXE)
