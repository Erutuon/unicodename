// #include <ctype.h>
#include <stdint.h> // for uint32_t

typedef uint32_t unichar;

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

size_t read_line (FILE * f, char * const buf, const size_t len);

void free_codepoint_names(char * * codepoint_names, size_t count);

char * * get_codepoint_names (
	FILE * Unicode_Data_txt, FILE * Name_Aliases_txt, unichar * const codepoints,
	const size_t count, char * * codepoint_names);