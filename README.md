# unicodename

A little command line program to look up the name or label of Unicode code points. It can be used in interactive mode or with arguments.

It requires [UnicodeData.txt](https://www.unicode.org/Public/UNIDATA/UnicodeData.txt) from the Unicode Database, and will use [NameAliases.txt](https://www.unicode.org/Public/UNIDATA/NameAliases.txt) if it has been provided. You must provide a directory that contains these files while compiling. (See the Makefile.) If the program does not find UnicodeData.txt in the directory that you provided, then in interactive mode you will be prompted to supply the correct directory; in argument mode, program will fail and exit unless the correct directory is supplied as an argument.

If given arguments, the program will read any valid options and attempt to interpret non-option arguments as code points, sort them, and return either their names or the text "error".

Options:
* `-d`, `--decimal`: code points are in decimal base
* `-f`, `--directory`: here, provide the directory in which to find UnicodeData.txt and NameAliases.txt
* `-x`, `--hexadecimal`: code points are in hexadecimal base (default)

`--decimal` and `--hexadecimal` override each other. The last one is used.

The first directory provided as argument to `--directory` is used.

TODO:
* By default, don't sort the code points; output their names in the order in which they were provided. Command-line option for sorted list.
* Option to look up the names of the code points in a string.
* Less memory allocation?