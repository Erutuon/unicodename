/*
 *  Prints name of codepoint, or aliases in the case of a control character.
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#define MINGW_HAS_SECURE_API 1 // so that _printf_p is defined
#include <stdio.h>

// Define UNICODE_DATA_IN_CURRENT_DIR if you've put UnicodeData.txt and
// NameAliases.txt in the current directory.
#ifndef UCD_DIRECTORY
# ifdef UNICODE_DATA_IN_CURRENT_DIR
#  define UCD_DIRECTORY                "./"
# else
#  define UCD_DIRECTORY                "C:/Users/Gabriel/Documents/Unicode/11.0.0/"
# endif
#endif

#define UNICODE_DATA_PATH              "UnicodeData.txt"
#define NAME_ALIASES_PATH              "NameAliases.txt"

// Or use DerivedName.txt? Doesn't indicate control codes, surrogates, etc.

#define PROMPT "> "

#define NAME_OUTPUT_FORMAT            "U+%2$X (decimal %2$d): %1$s\n"
// #define NAME_OUTPUT_FORMAT            "%1$s\n"

#define UNICODE_DATA_CODEPOINT_FORMAT "%04X"

#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))

#define CODEPOINT_VALID(cp) BETWEEN((cp), 0, 0x10FFFF)
#define CODEPOINT_STR_LEN    7 // "XXXXXX"

// Noncharacters: U+FDD0-U+FDEF and all codepoints ending in FFFE or FFFF.
// https://www.unicode.org/faq/private_use.html#nonchar4
#define IS_NONCHARACTER(codepoint) \
	(BETWEEN((codepoint), 0xFDD0, 0xFDEF) || ((codepoint) & 0xFFFE) == 0xFFFE)

#define STR_EQ(str1, str2) (strcmp((str1), (str2)) == 0)
#define STR_INCLUDES(str1, str2) (strstr((str1), (str2)) != NULL)
#define FREE0(pointer) ((pointer) != NULL ? free(pointer), (pointer) = NULL : NULL)
#define MEM_ERR "Not enough memory"
#define MEM_ERR_RETURN_NULL(pointer) if (pointer == NULL) { perror(MEM_ERR); return NULL; }
#define ARR_LEN(arr) (sizeof (arr) / sizeof *(arr))
#define ASPRINTF(...) (rasprintf(NULL, __VA_ARGS__))
#define FOPEN_ERR(filepath) \
	(fprintf(stderr, "Failed to open %s: %s\n", filepath, strerror(errno)), \
	 fflush(stderr))

char default_UCD_directory[] = UCD_DIRECTORY;
char * UCD_directory;

enum Unicode_data_fields {
	UNICODE_DATA_CODEPOINT_FIELD               = 1,
	UNICODE_DATA_NAME                          = 2,
	UNICODE_DATA_GENERAL_CATEGORY              = 3,
	UNICODE_DATA_CANONICAL_COMBINING_CLASS     = 4,
	UNICODE_DATA_BIDI_CLASS                    = 5,
	UNICODE_DATA_DECOMPOSITION_TYPE_OR_MAPPING = 6,
	UNICODE_DATA_NUMERIC_TYPE_DECIMAL          = 7,
	UNICODE_DATA_NUMERIC_TYPE_DIGIT            = 8,
	UNICODE_DATA_NUMERIC_TYPE_NUMERIC          = 9,
	UNICODE_DATA_BIDI_MIRRORED                 = 10,
	UNICODE_DATA_UNICODE_1_NAME                = 11,
	UNICODE_DATA_ISO_COMMENT                   = 12,
	UNICODE_DATA_SIMPLE_UPPERCASE_MAPPING      = 13,
	UNICODE_DATA_SIMPLE_LOWERCASE_MAPPING      = 14,
	UNICODE_DATA_SIMPLE_TITLECASE_MAPPING      = 15
};

typedef uint32_t unichar;

FILE * Unicode_Data_txt = NULL, * Name_Aliases_txt = NULL;

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
	else { free(s); perror(MEM_ERR); }
	
	va_end(args);
	
	return printed;
}

static size_t clear_line (FILE * f) {
	char c;
	size_t i = 0;
	
	if (f == NULL) return 0; // ??
	
	while (c = fgetc(f), c != EOF && c != '\n') ++i;
	
	return i;
}

// buf has space for len chars plus null terminator.
// Using fgets would make it harder to determine when call clear_line.
static size_t read_line (FILE * f, char * const buf, const size_t len) {
	char c;
	size_t i = 0;
	
	if (f == NULL) return -1;
	
	while (i < len && (c = fgetc(f), c != EOF) && c != '\n')
		buf[i++] = c;
	buf[i] = '\0';
	
	if (c == EOF) {
		if (ferror(f))
			perror(NULL), fclose(f), exit(1);
		else
			return EOF;
	}
	else if (c != '\n') clear_line(f);
	
	return i;
}

// Requires global UCD_directory not to be NULL.
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

// Returns true if data entry for codepoint was found.
static bool get_data_entry (FILE * data_file, const unichar codepoint,
							char * const data_line, const size_t data_line_len,
							bool start_over) {
	static char cur_data_line[BUFSIZ + 1];
	unichar cur_codepoint;
	
	if (data_line_len > BUFSIZ + 1) {
		perror("data_line_len should not be greater than BUFSIZ + 1");
		return NULL;
	}
	
	if (start_over)
		rewind(data_file);
	
	while (read_line(data_file, cur_data_line, data_line_len) != EOF
			&& sscanf(cur_data_line, "%x", &cur_codepoint) == 1
			&& cur_codepoint < codepoint);
	
	/* printf("%X\n", cur_codepoint);*/
	if (cur_codepoint >= codepoint) {
		char * first_semicolon = strchr(cur_data_line, ';');
		if (first_semicolon != NULL) {
			char * second_field = first_semicolon + 1;
			if (cur_codepoint == codepoint || second_field[0] == '<'
				&& STR_INCLUDES(second_field, ", Last")) // Determine if codepoint belongs to a range.
				goto copy_line;
		} // unlikely
		else printf("No semicolon in line for U+%X:\n%s\n", cur_codepoint, cur_data_line);
		
	}
	return false;
	
copy_line:
	strcpy(data_line, cur_data_line); // TODO: Use strncpy for safety.
	return true;
}

static char * get_data_field (char * const codepoint_data_entry, const int field) {
	if (codepoint_data_entry == NULL)
		return NULL;
	
	const char * field_start = codepoint_data_entry, * field_end = NULL,
		* next_semicolon;
	int i = 0;
	
	while (i < field) {
		++i;
		next_semicolon = strchr(field_end != NULL
			? field_end + 1 : codepoint_data_entry, ';');
		
		if (field_end != NULL) field_start = field_end + 1;
		
		if (next_semicolon != NULL) field_end = next_semicolon;
		else { // end of entry
			field_end = codepoint_data_entry + strlen(codepoint_data_entry) - 1;
			break;
		}
	}
	if (i == field && field_end - field_start > 0) {
		char * field_value = malloc(--field_end - field_start + 2);
		
		MEM_ERR_RETURN_NULL(field_value);
		
		strncpy(field_value, field_start, field_end - field_start + 1);
		field_value[field_end - field_start + 1] = '\0';
		
		return field_value;
	}
	else return NULL;
}

#define SYLLABLE_BASE  0xAC00
#define FIRST_HANGUL_SYLLABLE SYLLABLE_BASE
#define LAST_HANGUL_SYLLABLE  0xD7A3
#define IS_HANGUL_SYLLABLE(codepoint) \
	(BETWEEN((codepoint), FIRST_HANGUL_SYLLABLE, LAST_HANGUL_SYLLABLE))

// Jamo.txt
static const char * const leads[] = {
	"G", "GG", "N", "D", "DD", "R", "M", "B", "BB", "S", "SS", "", "J", "JJ",
	"C", "K", "T", "P", "H"
};

static const char * const vowels[] = {
	"A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O", "WA", "WAE", "OE",
	"YO", "U", "WEO", "WE", "WI", "YU", "EU", "YI", "I"
};

static const char * const trails[] = {
	"", "G", "GG", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM", "LB", "LS",
	"LT", "LP", "LH", "M", "B", "BS", "S", "SS", "NG", "J", "C", "K", "T", "P",
	"H"
};

#define VOWEL_COUNT     ARR_LEN(vowels)
#define TRAIL_COUNT     ARR_LEN(trails)
#define FINAL_COUNT    (VOWEL_COUNT * TRAIL_COUNT)

// Result is undefined if codepoint is not in fact a Hangul syllable.
// Hangul Syllable Decomposition and Hangul Name Generation in
// https://www.unicode.org/versions/Unicode10.0.0/ch03.pdf
static char * get_Hangul_syllable_name (unichar codepoint) {
	int syllable_index =  codepoint - SYLLABLE_BASE;
	int lead_index     =  syllable_index / FINAL_COUNT;
	int vowel_index    = (syllable_index % FINAL_COUNT) / TRAIL_COUNT;
	int trail_index    =  syllable_index % TRAIL_COUNT;
	
	if (lead_index  >= sizeof leads
	 || vowel_index >= sizeof vowels
	 || trail_index >= sizeof trails) {
		printf("Hangul Syllable name getting failed.\n"); return NULL;
	}
	
	return ASPRINTF("HANGUL SYLLABLE %s%s%s",
		leads[lead_index], vowels[vowel_index], trails[trail_index]);
}

typedef struct aliases_list {
	char * * list;
	int length, size;
} aliases_list;

#define FREE_AND_NULL(mem) (free(mem), (mem) = NULL)
#define IF_NOT_NULL_FREE_AND_NULL(mem) ((mem) != NULL ? FREE_AND_NULL(mem) : NULL)

static void aliases_list_free (aliases_list * * aliases) {
	if (*aliases != NULL) {
		if ((*aliases)->list != NULL) {
			for (int i = 0; i < (*aliases)->length; ++i)
				IF_NOT_NULL_FREE_AND_NULL((*aliases)->list[i]);
			
			FREE_AND_NULL((*aliases)->list);
		}
		FREE_AND_NULL(*aliases);
	}
}

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

static aliases_list * aliases_list_new () {
	aliases_list * aliases = malloc(sizeof *aliases);
	MEM_ERR_RETURN_NULL(aliases);
	
	aliases->list = NULL;
	if (!aliases_list_realloc_aliases(aliases, 2)) {
		free(aliases); perror(MEM_ERR); return NULL;
	}
	
	aliases->length = 0;
	
	return aliases;
}

static bool aliases_list_expand (aliases_list * aliases) {
	if (aliases->size > INT_MAX / 2) {
		fprintf(stderr, "Integer overflow");
		return false;
	}
	return aliases_list_realloc_aliases(aliases, aliases->size * 2);
}

// alias must be an allocated null-terminated string.
static bool aliases_list_add (aliases_list * aliases, char * alias) {
	++aliases->length;
	if (aliases->length > aliases->size && !aliases_list_expand(aliases))
		return false;
	aliases->list[aliases->length - 1] = alias;
	return true;
}

// Requires global variable Name_Aliases_txt.
static aliases_list * get_aliases (unichar codepoint) {
	aliases_list * aliases = NULL;
	char data_line[BUFSIZ + 1];
	char * alias;
	unichar cur_codepoint;
	
	if (Name_Aliases_txt == NULL) {
		puts("Name_Aliases_txt is NULL.");
		return NULL;
	}
	
	rewind(Name_Aliases_txt);
	
	while (read_line(Name_Aliases_txt, data_line, BUFSIZ) != EOF) {
		if (isxdigit(data_line[0])) {
			if (sscanf(data_line, "%x", &cur_codepoint) != 1) {
				printf("Error scanning line '%s'", data_line); return NULL;
			}
			
			if (cur_codepoint == codepoint) {
				if (aliases == NULL) {
					aliases = aliases_list_new();
					if (aliases == NULL) return NULL;
				}
				alias = get_data_field(data_line, 2);
				if (alias == NULL || !aliases_list_add(aliases, alias)) {
					aliases_list_free(&aliases); return NULL;
				}
			}
			else if (cur_codepoint > codepoint) break;
		}
	}
	
	return aliases;
}

static char * print_aliases_list (const unichar codepoint) {
	aliases_list * aliases = get_aliases(codepoint);
	if (aliases == NULL) return NULL;
	
	size_t total_length = 0;
	for (int i = 0; i < aliases->length; ++i)
		total_length += strlen(aliases->list[i]);
	total_length += (aliases->length - 1) * 2; // space for ", " between each alias
	
	char * printed = malloc(total_length + 1); // null terminator
	MEM_ERR_RETURN_NULL(printed);
	
	printed[0] = '\0'; // Ensure strcat doesn't think there's already a string here.
	for (int i = 0; i < aliases->length; ++i) {
		if (i != 0) strcat(printed, ", ");
		strcat(printed, aliases->list[i]);
	}
	
	aliases_list_free(&aliases);
	
	return printed;
}

typedef struct name_pattern {
	const unichar low, high;
	const char * const format;
} name_pattern;

// The names and labels here are described in chapter 4 of the Unicode specification:
// https://www.unicode.org/versions/Unicode11.0.0/ch04.pdf
static const name_pattern name_patterns[] = {
	{ 0x000000, 0x00001F, "<control-%04X>" },
	{ 0x00007F, 0x00009F, "<control-%04X>" },
	{ 0x003400, 0x004DB5, "CJK UNIFIED IDEOGRAPH-%04X" },
	{ 0x004E00, 0x009FEF, "CJK UNIFIED IDEOGRAPH-%04X" },
	{ 0x00D800, 0x00DFFF, "<surrogate-%04X>" },
	{ 0x00E000, 0x00F8FF, "<private-use-%04X>" },
	{ 0x00F900, 0x00FA6D, "CJK COMPATIBILITY IDEOGRAPH-%04X" },
	{ 0x00FA70, 0x00FAD9, "CJK COMPATIBILITY IDEOGRAPH-%04X" },
	{ 0x017000, 0x0187F1, "TANGUT IDEOGRAPH-%04X" },
	{ 0x01B170, 0x01B2FB, "NUSHU CHARACTER-%04X" },
	{ 0x020000, 0x02A6D6, "CJK UNIFIED IDEOGRAPH-%04X" },
	{ 0x02A700, 0x02B734, "CJK UNIFIED IDEOGRAPH-%04X" },
	{ 0x02B740, 0x02B81D, "CJK UNIFIED IDEOGRAPH-%04X" },
	{ 0x02B820, 0x02CEA1, "CJK UNIFIED IDEOGRAPH-%04X" },
	{ 0x02CEB0, 0x02EBE0, "CJK UNIFIED IDEOGRAPH-%04X" },
	{ 0x02F800, 0x02FA1D, "CJK COMPATIBILITY IDEOGRAPH-%04X" },
	{ 0x0F0000, 0x0FFFFD, "<private-use-%04X>" },
	{ 0x100000, 0x1FFFFD, "<private-use-%04X>" }
};

static char * get_name_by_rule (const unichar codepoint) {
	if (IS_HANGUL_SYLLABLE(codepoint))
		return get_Hangul_syllable_name(codepoint);
	else if (IS_NONCHARACTER(codepoint))
		return ASPRINTF("<noncharacter-%04X>", codepoint);
	else {
		for (int i = 0; i < ARR_LEN(name_patterns); ++i) {
			const name_pattern * patt = &name_patterns[i];
			if (codepoint < patt->low) break;
			else if (codepoint <= patt->high) {
				return ASPRINTF(patt->format, codepoint);
			}
		}
	}
	return NULL;
}

#define CAST_AND_DEREFERENCE_CODEPOINT(cp) (*(unichar *) cp)
static int comp_codepoints(const void * p1, const void * p2) {
	unichar a = CAST_AND_DEREFERENCE_CODEPOINT(p1),
		    b = CAST_AND_DEREFERENCE_CODEPOINT(p2);
	return (a > b) - (a < b);
}
#undef CAST_AND_DEREFERENCE_CODEPOINT

// Look up names of all code points in array, storing NULL if code point
// was invalid or no name was found. Return list of names or NULL.
// List as well as all non-NULL names must be freed.
static char * * get_codepoint_names (unichar * const codepoints,
									 const size_t count,
									 char * * codepoint_names) {
	static char data_line[BUFSIZ + 1];
	qsort(codepoints, count, sizeof *codepoints, comp_codepoints);
	codepoint_names = realloc(codepoint_names, sizeof (char *) * count);
	
	for (int i = 0; i < count; ++i) {
		const unichar codepoint = codepoints[i];
		char * codepoint_name = NULL;
		if (CODEPOINT_VALID(codepoint)
				&& (codepoint_name = get_name_by_rule(codepoint), codepoint_name == NULL)) {
			if (get_data_entry(Unicode_Data_txt, codepoint, data_line, BUFSIZ, i == 0)) {
				codepoint_name = get_data_field(data_line, UNICODE_DATA_NAME);
				
				if (codepoint_name != NULL) {
					if (codepoint_name[0] == '<') {
						// Should already have printed name above!
						puts("???");
					}
					else {
						char * aliases = print_aliases_list(codepoint);
						if (aliases != NULL) {
							char * name_with_aliases =
								ASPRINTF("%s (%s)", codepoint_name, aliases);
							FREE0(codepoint_name), FREE0(aliases);
							codepoint_name = name_with_aliases;
						}
					}
				}
			}
			else codepoint_name = ASPRINTF("<reserved-%04X>", codepoint);
		}
		codepoint_names[i] = codepoint_name;
	}
	
	return codepoint_names;
}

static bool get_directory (char * default_directory, const char * const filename,
						   const char * message, char * * directory_var,
						   FILE * * file_var) {
	FILE * file = NULL;
	static char buf[BUFSIZ + 1];
	char * filepath = NULL, * directory = NULL, * new_directory;
	size_t len;
	
	// str_dup?
	directory = ASPRINTF("%s", default_directory);
	if (directory == NULL) goto mem_err;
	
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
			FOPEN_ERR(filepath);
			puts(message);
			
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
			new_directory = rasprintf(directory, "%s", buf);
			if (new_directory == NULL) goto mem_err;
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
static bool open_Unicode_data (void) {
	if (!get_directory(UCD_DIRECTORY, UNICODE_DATA_PATH,
			"Enter directory for Unicode Character Database.",
			&UCD_directory, &Unicode_Data_txt))
		return false;
	
	if (!open_UCD_file(NAME_ALIASES_PATH, &Name_Aliases_txt))
		printf("Aliases will not be printed.\n");
	// No error if NameAliases.txt can't be found.
	
	return true;
}

static void free_codepoint_names(char * * codepoint_names, size_t count) {
	for (int i = 0; i < count; ++i) {
		FREE0(codepoint_names[i]);
	}
	FREE0(codepoint_names);
}

static void do_prompt (void) {
	unichar codepoint;
	char * * codepoint_names = NULL;
	
	setvbuf(stdout, NULL, _IOLBF, 0);
	
	puts("To exit, press enter.");

	while (codepoint = read_codepoint(), codepoint != -1) {
		codepoint_names = get_codepoint_names(&codepoint, 1, codepoint_names);
		
		if (codepoint_names != NULL)
			_printf_p(NAME_OUTPUT_FORMAT, codepoint_names[0], codepoint);
		else
			printf("Codepoint U+%X does not have a name.\n", codepoint);
	}
	free_codepoint_names(codepoint_names, 1);
}

int main (int argc, const char * * argv) {
	if (!open_Unicode_data()) exit(EXIT_FAILURE); // Open Unicode_Data_txt and optionally Name_Aliases_txt.
	
	if (argc > 1) {
		unichar codepoint;
		size_t codepoint_count = argc - 1;
		unichar * codepoints = malloc(codepoint_count * sizeof *codepoints);
		for (int i = 1; i < argc; ++i) {
			if (sscanf(argv[i], "%x", &codepoint) != 1)
				codepoints[i - 1] = -1;
			else {
				codepoints[i - 1] = codepoint;
			}
		}
		char * * codepoint_names = get_codepoint_names(codepoints, codepoint_count, NULL);
		if (codepoint_names == NULL)
			goto close_files;
		
		for (int i = 0; i < codepoint_count; ++i)
			puts(codepoint_names[i] != NULL ? codepoint_names[i] : "error");
		
		free_codepoint_names(codepoint_names, codepoint_count);
	}
	else do_prompt();
	
close_files:
	if (fclose(Unicode_Data_txt) || Name_Aliases_txt != NULL && fclose(Name_Aliases_txt))
		perror("Failed to close file");
	
	return EXIT_SUCCESS;
}