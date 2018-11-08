# unicodename

A little command line program to look up the name or label of Unicode code points. It requires [UnicodeData.txt](https://www.unicode.org/Public/UNIDATA/UnicodeData.txt) from the Unicode Database, and will use [NameAliases.txt](https://www.unicode.org/Public/UNIDATA/NameAliases.txt) and [DerivedGeneralCategory.txt](https://unicode.org/Public/UNIDATA/extracted/DerivedGeneralCategory.txt) if they have been provided. They can be put in the current directory, in which case define `UNICODE_DATA_IN_CURRENT_DIR` while compiling. Otherwise, the program will prompt you to enter the directory in which it can find these files.

If given arguments, the program will attempt to interpret them as code points in hexadecimal base, sort them, and return their names.