/*
 *  Prints name of code point, or aliases in the case of a control character.
 */

#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "common.h"
#include "unicodename.h"
#include "aliases.h"

#define STR_INCLUDES(str1, str2) (strstr((str1), (str2)) != NULL)
#define FREE0(pointer) ((pointer) != NULL ? free(pointer), (pointer) = NULL : NULL)
#define ARR_LEN(arr) (sizeof (arr) / sizeof *(arr))

// CODE POINT-RELATED STUFF

// Noncharacters: U+FDD0-U+FDEF and code points whose value in hexadecimal
// base ends in FFFE or FFFF.
// https://www.unicode.org/faq/private_use.html#nonchar4
#define IS_NONCHARACTER(codepoint) \
	(BETWEEN((codepoint), 0xFDD0, 0xFDEF) || ((codepoint) & 0xFFFE) == 0xFFFE)

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

typedef struct name_pattern {
	const unichar low, high;
	const char * const format;
} name_pattern;

// These names and labels are described in chapter 4 of the Unicode specification:
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

// END CODE POINT-RELATED STUFF

static size_t clear_line (FILE * f) {
	char c;
	size_t i = 0;
	
	if (f == NULL) return 0; // ??
	
	while (c = fgetc(f), c != EOF && c != '\n') ++i;
	
	return i;
}

// buf has space for len chars plus null terminator.
// Using fgets would make it harder to determine when call clear_line.
size_t read_line (FILE * f, char * const buf, const size_t len) {
	char c = EOF; // to silence "uninitialized" error
	size_t i = 0;
	
	if (f == NULL) return -1;
	
	while (i < len && (c = fgetc(f), c != EOF) && c != '\n')
		buf[i++] = c;
	buf[i] = '\0';
	
	if (c == EOF) {
		if (ferror(f)) perror(NULL), fclose(f), exit(1);
		else return EOF;
	}
	else if (c != '\n') clear_line(f);
	
	return i;
}

// Returns true if data entry for code point was found.
static bool get_data_entry (FILE * data_file,
							const unichar codepoint,
							char * const data_line,
							const size_t data_line_len,
							bool start_over) {
	static char cur_data_line[BUFSIZ + 1];
	unichar cur_codepoint;
	
	if (data_line_len > BUFSIZ + 1) {
		perror("data_line_len should not be greater than BUFSIZ + 1");
		return NULL;
	}
	
	if (start_over) rewind(data_file);
	
	while (read_line(data_file, cur_data_line, data_line_len) != EOF
			&& sscanf(cur_data_line, "%x", &cur_codepoint) == 1
			&& cur_codepoint < codepoint);
	
	if (cur_codepoint >= codepoint) {
		char * first_semicolon = strchr(cur_data_line, ';');
		if (first_semicolon != NULL) {
			char * second_field = first_semicolon + 1;
			if (cur_codepoint == codepoint
					// Determine if code point belongs to a range.
					|| (second_field[0] == '<'
					&& STR_INCLUDES(second_field, ", Last"))) {
				strncpy(data_line, cur_data_line, data_line_len);
				return true;
			}
		} // unlikely
		else fprintf(stderr, "No semicolon in line for U+%X:\n%s\n",
			cur_codepoint, cur_data_line);
	}
	return false;
}

static char * get_data_field (char * const codepoint_data_entry,
							  const unsigned int field) {
	if (codepoint_data_entry == NULL)
		return NULL;
	
	const char * field_start = codepoint_data_entry,
		* field_end = NULL,
		* next_semicolon;
	size_t field_len = 0;
	int i = 0;
	
	while (i < field) {
		++i;
		if (field_end != NULL) field_start = field_end + 1;
		next_semicolon = strchr(field_start, ';');
		
		if (next_semicolon != NULL) field_end = next_semicolon;
		else { // end of entry
			field_end = codepoint_data_entry + strlen(codepoint_data_entry) - 1;
			break;
		}
	}
	if (i == field && (field_len = field_end - field_start) > 0) {
		char * field_value = malloc(field_len + 1);
		
		MEM_ERR_RETURN_NULL(field_value);
		
		strncpy(field_value, field_start, field_len);
		field_value[field_len] = '\0';
		
		return field_value;
	}
	return NULL;
}

// Result is undefined if code point is not a Hangul syllable.
// Hangul Name Generation is described in chapter 3 of the Unicode specification:
// https://www.unicode.org/versions/Unicode10.0.0/ch03.pdf
static char * get_Hangul_syllable_name (unichar codepoint) {
	int syllable_index =  codepoint - SYLLABLE_BASE;
	int lead_index     =  syllable_index / FINAL_COUNT;
	int vowel_index    = (syllable_index % FINAL_COUNT) / TRAIL_COUNT;
	int trail_index    =  syllable_index % TRAIL_COUNT;
	
	if (lead_index  >= sizeof leads
	 || vowel_index >= sizeof vowels
	 || trail_index >= sizeof trails) {
		fputs("Hangul Syllable name getting failed.", stderr); return NULL;
	}
	
	return ASPRINTF("HANGUL SYLLABLE %s%s%s",
		leads[lead_index], vowels[vowel_index], trails[trail_index]);
}

static aliases_list * get_aliases (FILE * Name_Aliases_txt,
								   const unichar codepoint,
								   bool start_over) {
	aliases_list * aliases = NULL;
	char data_line[BUFSIZ + 1];
	char * alias;
	unichar cur_codepoint;
	
	if (Name_Aliases_txt == NULL) {
		fputs("Name_Aliases_txt is NULL.", stderr);
		return NULL;
	}
	
	if (start_over)
		rewind(Name_Aliases_txt);
	
	while (read_line(Name_Aliases_txt, data_line, BUFSIZ) != EOF) {
		if (isxdigit(data_line[0])) {
			if (sscanf(data_line, "%x", &cur_codepoint) != 1) {
				fprintf(stderr, "Error scanning line '%s'", data_line); return NULL;
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

static char * print_aliases_list (aliases_list * aliases) {
	if (aliases == NULL) return NULL;
	
	size_t total_length = 0;
	for (int i = 0; i < aliases->length; ++i)
		total_length += strlen(aliases->list[i]);
	total_length += (aliases->length - 1) * 2 + 1; // space for ", " between each alias and null terminator
	
	char * printed = malloc(total_length);
	MEM_ERR_RETURN_NULL(printed);
	
	printed[0] = '\0'; // Ensure strcat doesn't think there's already a string here.
	for (int i = 0; i < aliases->length; ++i) {
		if (i != 0) strcat(printed, ", ");
		strcat(printed, aliases->list[i]);
	}
	
	aliases_list_free(&aliases);
	
	return printed;
}

static char * get_name_by_rule (const unichar codepoint) {
	if (IS_HANGUL_SYLLABLE(codepoint))
		return get_Hangul_syllable_name(codepoint);
	else if (IS_NONCHARACTER(codepoint))
		return ASPRINTF("<noncharacter-%04X>", codepoint);
	else {
		for (int i = 0; i < ARR_LEN(name_patterns); ++i) {
			const name_pattern * patt = &name_patterns[i];
			if (codepoint < patt->low) break;
			else if (codepoint <= patt->high)
				return ASPRINTF(patt->format, codepoint);
		}
	}
	return NULL;
}

#define CAST_AND_DEREFERENCE_CODEPOINT(cp) (*(unichar *) (cp))
static int comp_codepoints(const void * p1, const void * p2) {
	unichar a = CAST_AND_DEREFERENCE_CODEPOINT(p1),
		    b = CAST_AND_DEREFERENCE_CODEPOINT(p2);
	return (a > b) - (a < b);
}
#undef CAST_AND_DEREFERENCE_CODEPOINT

// Look up names of all code points in array, storing NULL if code point
// was invalid or no name was found. Return list of names or NULL.
// List as well as all non-NULL names must be freed.
char * * get_codepoint_names (FILE * Unicode_Data_txt,
							  FILE * Name_Aliases_txt,
							  unichar * const codepoints,
							  const size_t count,
							  char * * codepoint_names) {
	static char data_line[BUFSIZ + 1];
	qsort(codepoints, count, sizeof *codepoints, comp_codepoints);
	codepoint_names = realloc(codepoint_names, sizeof (char *) * count);
	
	bool scanned_aliases = false;
	
	for (int i = 0; i < count; ++i) {
		const unichar codepoint = codepoints[i];
		char * codepoint_name = NULL;
		if (CODEPOINT_VALID(codepoint)) {
			if ((codepoint_name = get_name_by_rule(codepoint)) == NULL
				&& get_data_entry(Unicode_Data_txt, codepoint, data_line, BUFSIZ, i == 0))
				codepoint_name = get_data_field(data_line, UNICODE_DATA_NAME);
			
			if (codepoint_name == NULL)
				codepoint_name = ASPRINTF("<reserved-%04X>", codepoint);
			else {
				char * aliases = print_aliases_list(
					get_aliases(Name_Aliases_txt, codepoint, scanned_aliases));
				scanned_aliases = true;
				if (aliases != NULL) {
					char * name_with_aliases =
						ASPRINTF("%s (%s)", codepoint_name, aliases);
					FREE0(codepoint_name), FREE0(aliases);
					codepoint_name = name_with_aliases;
				}
			}
		}
		codepoint_names[i] = codepoint_name;
	}
	
	return codepoint_names;
}

void free_codepoint_names(char * * codepoint_names, size_t count) {
	for (int i = 0; i < count; ++i) FREE0(codepoint_names[i]);
	FREE0(codepoint_names);
}