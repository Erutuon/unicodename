/*
 *  Prints name of codepoint, or aliases in the case of a control character.
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#ifdef _WIN32
#define MINGW_HAS_SECURE_API 1 // so that _printf_p is defined
#endif
#include <stdio.h>

#include "common.h"
#include "unicodename.h"

// Define UNICODE_DATA_IN_CURRENT_DIR if you've put UnicodeData.txt and
// NameAliases.txt in the current directory.
#ifndef UCD_DIRECTORY
#  ifdef UNICODE_DATA_IN_CURRENT_DIR
#    define UCD_DIRECTORY  "./"
#  else
#    error "Define UCD_DIRECTORY to the path for the directory that contains UnicodeData.txt and NameAliases.txt."
#  endif
#endif

// Need printf with positional arguments.
#ifdef _WIN32 // Windows
#  define my_printf _printf_p
#else // Unix, hopefully
#  define my_printf printf
#endif

#define UNICODE_DATA_PATH  "UnicodeData.txt"
#define NAME_ALIASES_PATH  "NameAliases.txt"

// Or use DerivedName.txt? Doesn't indicate control codes, surrogates, etc.

#define PROMPT "> "

#define NAME_OUTPUT_FORMAT            "U+%2$X (decimal %2$d): %1$s\n"
// #define NAME_OUTPUT_FORMAT            "%1$s\n"

#define CODEPOINT_STR_LEN    7 // "XXXXXX"

#define FREE0(pointer) ((pointer) != NULL ? free(pointer), (pointer) = NULL : NULL)
#define FOPEN_ERR(filepath) \
	(fprintf(stderr, "Failed to open %s: %s\n", filepath, strerror(errno)), \
	 fflush(stderr))

static int decimal = 0;

static char * UCD_directory;
const char * default_UCD_directory = UCD_DIRECTORY;

static FILE * Unicode_Data_txt = NULL, * Name_Aliases_txt = NULL;

static bool open_UCD_file(const char * filename, FILE * * out) {
	char * filepath = NULL;
	FILE * datafile = NULL;
	
	if (UCD_directory == NULL) return false;
	
	filepath = ASPRINTF("%s%s", UCD_directory, filename);
	
	if (filepath != NULL) {
		datafile = fopen(filepath, "r");
		
		if (datafile != NULL) *out = datafile;
		else FOPEN_ERR(filepath);
		
		free(filepath);
	}
	else perror(MEM_ERR);
	
	return datafile != NULL;
}

static unichar read_codepoint () {
	unichar codepoint;
	size_t i;
	static char codepoint_str[CODEPOINT_STR_LEN];
	
read_input:
	while (1) {
		printf("Hexadecimal codepoint to look up the name of?\n" PROMPT);
		
		// Newline on its own triggers exit.
		if (read_line(stdin, codepoint_str, CODEPOINT_STR_LEN - 1) == 0)
			return -1;
		
		i = 0;
		while (isxdigit(codepoint_str[i])) ++i;
		
		if (codepoint_str[i] != '\0') { // Not all characters are hexadecimal.
			puts("Please enter a hexadecimal number."); continue;
		}
		else break;
	}
	
	if (sscanf(codepoint_str, "%x", &codepoint) != 1) { // Should not happen.
		char msg[BUFSIZ];
		sprintf(msg, "Error converting %s to integer", codepoint_str);
		perror(msg);
		return -1;
	}
	
	if (!CODEPOINT_VALID(codepoint)) {
		puts("Maximum codepoint is U+10FFFF.");
		goto read_input;
	}
	
	return codepoint;
}


// str must have enough space for slash and null terminator after last
// character in string that isn't a slash or null terminator.
#ifdef _WIN32
#define WINDIRCHAR(c) ((c) == '\\')
#else
#define WINDIRCHAR(c) (0)
#endif

#define CHAR_TO_SKIP(c) ((c) == '/' || WINDIRCHAR(c) || (c) == '\0')

static void ensure_final_slash(char * str) {
	size_t i = strlen(str);
	while (CHAR_TO_SKIP(str[i])) --i;
	str[i + 1] = '/'; str[i + 2] = '\0';
}

static bool get_directory (const char * default_directory, const char * const filename,
						   const char * message, char * * directory_var,
						   FILE * * file_var, bool crash_if_not_default) {
	FILE * file = NULL;
	static char buf[BUFSIZ + 1];
	char * filepath = NULL, * directory = NULL, * new_directory;
	size_t len;
	
	// str_dup?
	directory = ASPRINTF("%s//", default_directory);
	if (directory == NULL) goto mem_err;
	ensure_final_slash(directory);
	
	filepath = ASPRINTF("%s%s", directory, filename);
	if (filepath == NULL) goto mem_err;
	
	while (true) {
		file = fopen(filepath, "r");
		
		if (file != NULL) {
			*file_var = file;
			*directory_var = directory;
			goto success;
		}
		else {
			if (crash_if_not_default)
				exit(EXIT_FAILURE);
			else {
				FOPEN_ERR(filepath);
				puts(message);
			}
			
			len = read_line(stdin, buf, BUFSIZ);
			if (len == (size_t) EOF) {
				perror("Error reading line."); continue;
			}
			else if (len == 0) return false;
			else if (buf[len - 1] != '/') {
				if (len < BUFSIZ - 1) buf[len] = '/', buf[len + 1] = '\0';
				else { // unlikely
					printf("Path too long"); goto mem_err;
				}
			}
			
			// Make str_dup?
			new_directory = rasprintf(directory, "%s//", buf);
			if (new_directory == NULL) goto mem_err;
			ensure_final_slash(new_directory);
			directory = new_directory;
			
			filepath = rasprintf(filepath, "%s%s", directory, filename);
			if (filepath == NULL) goto mem_err;
		}
	}
	
mem_err:
	perror(MEM_ERR);
	
	FREE0(directory);
success:
	FREE0(filepath);
	
	return file != NULL && directory != NULL;
}

// Sets global variables Unicode_Data_txt and Name_Aliases_txt.
// Returns true if Unicode_Data_txt could be found.
static bool open_Unicode_data (bool crash_if_not_default) {
	if (!get_directory(default_UCD_directory, UNICODE_DATA_PATH,
			"Enter directory for Unicode Character Database.",
			&UCD_directory, &Unicode_Data_txt, crash_if_not_default))
		return false;
	
	if (!open_UCD_file(NAME_ALIASES_PATH, &Name_Aliases_txt))
		printf("Aliases will not be printed.\n");
	// No error if NameAliases.txt can't be found.
	
	return true;
}

static void do_prompt (void) {
	unichar codepoint;
	char * * codepoint_names = NULL;
	
	setvbuf(stdout, NULL, _IOLBF, 0);
	
	puts("To exit, press enter.");

	while (codepoint = read_codepoint(), codepoint != -1) {
		codepoint_names = get_codepoint_names(Unicode_Data_txt, Name_Aliases_txt,
											  &codepoint, 1, codepoint_names);
		
		if (codepoint_names != NULL)
			my_printf(NAME_OUTPUT_FORMAT, codepoint_names[0], codepoint);
		else
			printf("Codepoint U+%X does not have a name.\n", codepoint);
	}
	free_codepoint_names(codepoint_names, 1);
}

static int read_options (int argc, char * const * argv) {
	static const struct option options[] = {
		{ "directory", required_argument, NULL, 'f' },
		{ "decimal", optional_argument, &decimal, 1 },
		{ "hexadecimal", optional_argument, &decimal, 0 }
	};
	
	int c;
	int option_index = 0;
	const char * directory = NULL;
	opterr = 0;
	while ((c = getopt_long(argc, argv, "f:dx", options, &option_index)) != -1) {
		switch (c) {
			case 'd': case 'x':
				decimal = c == 'd';
				break;
			case 'f':
				if (directory == NULL)
					directory = optarg;
				break;
		}
	}
	
	if (directory != NULL)
		default_UCD_directory = directory;
	
	return optind;
}

// TODO: allow Unicode data directory to be specified with command line arg.
// TODO: allow code points to be input in decimal.
int main (int argc, char * const * argv) {
	if (argc > 1) {
		unichar codepoint;
		int first_codepoint_index = read_options(argc, argv);
		// Open Unicode_Data_txt and optionally Name_Aliases_txt.
		// Exit if directory is not correct.
		open_Unicode_data(true);
		size_t codepoint_count = argc - first_codepoint_index;
		unichar * codepoints = malloc(codepoint_count * sizeof *codepoints);
		if (!(codepoint_count > 0)) {
			fputs("no codepoints provided", stderr);
			return EXIT_FAILURE;
		}
		for (int i = 0; i < codepoint_count; ++i) {
			codepoints[i] = (sscanf(argv[first_codepoint_index + i],
									decimal ? "%d" : "%x", &codepoint) == 1)
							? codepoint : -1;
		}
		char * * codepoint_names = get_codepoint_names(
				Unicode_Data_txt, Name_Aliases_txt, codepoints, codepoint_count, NULL);
		free(codepoints);
		if (codepoint_names == NULL)
			goto close_files;
		
		for (int i = 0; i < codepoint_count; ++i)
			puts(codepoint_names[i] != NULL ? codepoint_names[i] : "error");
		
		free_codepoint_names(codepoint_names, codepoint_count);
	}
	else {
		if (!open_Unicode_data(false)) exit(EXIT_FAILURE); // Open Unicode_Data_txt and optionally Name_Aliases_txt.
		do_prompt();
	}
	
close_files:
	if (UCD_directory != default_UCD_directory)
		free(UCD_directory);
	if (fclose(Unicode_Data_txt) || (Name_Aliases_txt != NULL && fclose(Name_Aliases_txt)))
		perror("Failed to close file");
	
	return EXIT_SUCCESS;
}