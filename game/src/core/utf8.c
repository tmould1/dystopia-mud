/*
 * utf8.c - UTF-8 encoding utilities
 *
 * Zero-dependency, cross-platform UTF-8 string handling.
 * All functions operate on raw byte arrays and use no platform-specific APIs.
 *
 * UTF-8 encoding summary:
 *   U+0000..U+007F    0xxxxxxx                            (1 byte,  ASCII)
 *   U+0080..U+07FF    110xxxxx 10xxxxxx                   (2 bytes)
 *   U+0800..U+FFFF    1110xxxx 10xxxxxx 10xxxxxx          (3 bytes, includes CJK)
 *   U+10000..U+10FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (4 bytes, emoji etc.)
 *
 * Key safety property: no valid multi-byte sequence contains bytes in the
 * ASCII range (0x00-0x7F), so existing code that splits on spaces, newlines,
 * tildes, or '#' color codes is inherently UTF-8-safe.
 */

#include "merc.h"
#include "utf8.h"

/* ======================================================================
 * Byte-level queries
 * ====================================================================== */

/*
 * Return expected byte length of a UTF-8 sequence from its lead byte.
 * Returns 1 for ASCII, 2-4 for multi-byte, 0 for invalid lead bytes
 * (bare continuation bytes 0x80-0xBF, or overlong 0xC0-0xC1, or 0xF5+).
 */
int utf8_seq_len( unsigned char lead ) {
	if ( lead < 0x80 ) return 1;		 /* ASCII */
	if ( lead < 0xC2 ) return 0;		 /* continuation or overlong */
	if ( lead < 0xE0 ) return 2;		 /* 110xxxxx */
	if ( lead < 0xF0 ) return 3;		 /* 1110xxxx */
	if ( lead < 0xF5 ) return 4;		 /* 11110xxx */
	return 0;							 /* 0xF5+ invalid */
}

/* ======================================================================
 * Codepoint decode / encode
 * ====================================================================== */

/*
 * Decode one UTF-8 codepoint from *str and advance *str past it.
 * Returns 0xFFFD (Unicode replacement character) on any invalid sequence.
 * Handles: overlong encodings, surrogates, out-of-range codepoints.
 */
unsigned int utf8_decode( const char **str ) {
	const unsigned char *p = (const unsigned char *) *str;
	unsigned int cp;
	int expect;

	if ( *p == '\0' )
		return 0;

	if ( p[0] < 0x80 ) {
		/* ASCII fast path */
		*str += 1;
		return p[0];
	}

	expect = utf8_seq_len( p[0] );
	if ( expect == 0 ) {
		/* Invalid lead byte - skip one byte */
		*str += 1;
		return 0xFFFD;
	}

	/* Decode based on sequence length */
	switch ( expect ) {
	case 2:
		if ( !utf8_is_cont( p[1] ) ) { *str += 1; return 0xFFFD; }
		cp = ( (unsigned int)( p[0] & 0x1F ) << 6 )
		   | (unsigned int)( p[1] & 0x3F );
		break;
	case 3:
		if ( !utf8_is_cont( p[1] ) ) { *str += 1; return 0xFFFD; }
		if ( !utf8_is_cont( p[2] ) ) { *str += 1; return 0xFFFD; }
		cp = ( (unsigned int)( p[0] & 0x0F ) << 12 )
		   | ( (unsigned int)( p[1] & 0x3F ) << 6 )
		   | (unsigned int)( p[2] & 0x3F );
		/* Reject overlong (< U+0800) and surrogates (U+D800-U+DFFF) */
		if ( cp < 0x0800 || ( cp >= 0xD800 && cp <= 0xDFFF ) ) {
			*str += 1;
			return 0xFFFD;
		}
		break;
	case 4:
		if ( !utf8_is_cont( p[1] ) ) { *str += 1; return 0xFFFD; }
		if ( !utf8_is_cont( p[2] ) ) { *str += 1; return 0xFFFD; }
		if ( !utf8_is_cont( p[3] ) ) { *str += 1; return 0xFFFD; }
		cp = ( (unsigned int)( p[0] & 0x07 ) << 18 )
		   | ( (unsigned int)( p[1] & 0x3F ) << 12 )
		   | ( (unsigned int)( p[2] & 0x3F ) << 6 )
		   | (unsigned int)( p[3] & 0x3F );
		/* Reject overlong (< U+10000) or out of range (> U+10FFFF) */
		if ( cp < 0x10000 || cp > 0x10FFFF ) {
			*str += 1;
			return 0xFFFD;
		}
		break;
	default:
		*str += 1;
		return 0xFFFD;
	}

	*str += expect;
	return cp;
}

/*
 * Encode a Unicode codepoint into UTF-8.
 * buf must have room for at least 5 bytes (4 max + NUL).
 * Returns number of bytes written (1-4), or 0 if codepoint is invalid.
 * Does NOT NUL-terminate.
 */
int utf8_encode( unsigned int cp, char *buf ) {
	if ( cp < 0x80 ) {
		buf[0] = (char) cp;
		return 1;
	}
	if ( cp < 0x800 ) {
		buf[0] = (char)( 0xC0 | ( cp >> 6 ) );
		buf[1] = (char)( 0x80 | ( cp & 0x3F ) );
		return 2;
	}
	if ( cp < 0x10000 ) {
		/* Reject surrogates */
		if ( cp >= 0xD800 && cp <= 0xDFFF )
			return 0;
		buf[0] = (char)( 0xE0 | ( cp >> 12 ) );
		buf[1] = (char)( 0x80 | ( ( cp >> 6 ) & 0x3F ) );
		buf[2] = (char)( 0x80 | ( cp & 0x3F ) );
		return 3;
	}
	if ( cp <= 0x10FFFF ) {
		buf[0] = (char)( 0xF0 | ( cp >> 18 ) );
		buf[1] = (char)( 0x80 | ( ( cp >> 12 ) & 0x3F ) );
		buf[2] = (char)( 0x80 | ( ( cp >> 6 ) & 0x3F ) );
		buf[3] = (char)( 0x80 | ( cp & 0x3F ) );
		return 4;
	}
	return 0; /* out of range */
}

/* ======================================================================
 * Display width (wcwidth equivalent)
 *
 * Uses compact range tables searched with binary search.
 * Derived from Unicode 15.1 East_Asian_Width and General_Category data.
 * ====================================================================== */

typedef struct {
	unsigned int first;
	unsigned int last;
} unicode_range;

/*
 * Zero-width character ranges:
 * - Combining diacritical marks
 * - Zero-width spaces/joiners
 * - Variation selectors
 * - Hangul Jamo medial vowels and final consonants (for composed display)
 */
static const unicode_range zero_width_ranges[] = {
	{ 0x0300, 0x036F },   /* Combining Diacritical Marks */
	{ 0x0483, 0x0489 },   /* Combining Cyrillic */
	{ 0x0591, 0x05BD },   /* Hebrew combining */
	{ 0x05BF, 0x05BF },
	{ 0x05C1, 0x05C2 },
	{ 0x05C4, 0x05C5 },
	{ 0x05C7, 0x05C7 },
	{ 0x0610, 0x061A },   /* Arabic combining */
	{ 0x064B, 0x065F },
	{ 0x0670, 0x0670 },
	{ 0x06D6, 0x06DC },
	{ 0x06DF, 0x06E4 },
	{ 0x06E7, 0x06E8 },
	{ 0x06EA, 0x06ED },
	{ 0x0711, 0x0711 },   /* Syriac */
	{ 0x0730, 0x074A },
	{ 0x07A6, 0x07B0 },   /* Thaana */
	{ 0x07EB, 0x07F3 },   /* NKo */
	{ 0x0816, 0x0819 },   /* Samaritan */
	{ 0x081B, 0x0823 },
	{ 0x0825, 0x0827 },
	{ 0x0829, 0x082D },
	{ 0x0859, 0x085B },   /* Mandaic */
	{ 0x08D3, 0x08E1 },   /* Arabic Extended-A */
	{ 0x08E3, 0x0902 },
	{ 0x093A, 0x093A },   /* Devanagari */
	{ 0x093C, 0x093C },
	{ 0x0941, 0x0948 },
	{ 0x094D, 0x094D },
	{ 0x0951, 0x0957 },
	{ 0x0962, 0x0963 },
	{ 0x0981, 0x0981 },   /* Bengali */
	{ 0x09BC, 0x09BC },
	{ 0x09C1, 0x09C4 },
	{ 0x09CD, 0x09CD },
	{ 0x09E2, 0x09E3 },
	{ 0x0A01, 0x0A02 },   /* Gurmukhi */
	{ 0x0A3C, 0x0A3C },
	{ 0x0A41, 0x0A42 },
	{ 0x0A47, 0x0A48 },
	{ 0x0A4B, 0x0A4D },
	{ 0x0A51, 0x0A51 },
	{ 0x0A70, 0x0A71 },
	{ 0x0A75, 0x0A75 },
	{ 0x0A81, 0x0A82 },   /* Gujarati */
	{ 0x0ABC, 0x0ABC },
	{ 0x0AC1, 0x0AC5 },
	{ 0x0AC7, 0x0AC8 },
	{ 0x0ACD, 0x0ACD },
	{ 0x0AE2, 0x0AE3 },
	{ 0x0AFA, 0x0AFF },
	{ 0x0B01, 0x0B01 },   /* Oriya */
	{ 0x0B3C, 0x0B3C },
	{ 0x0B3F, 0x0B3F },
	{ 0x0B41, 0x0B44 },
	{ 0x0B4D, 0x0B4D },
	{ 0x0B56, 0x0B56 },
	{ 0x0B62, 0x0B63 },
	{ 0x0B82, 0x0B82 },   /* Tamil */
	{ 0x0BC0, 0x0BC0 },
	{ 0x0BCD, 0x0BCD },
	{ 0x0C00, 0x0C00 },   /* Telugu */
	{ 0x0C3E, 0x0C40 },
	{ 0x0C46, 0x0C48 },
	{ 0x0C4A, 0x0C4D },
	{ 0x0C55, 0x0C56 },
	{ 0x0C62, 0x0C63 },
	{ 0x0C81, 0x0C81 },   /* Kannada */
	{ 0x0CBC, 0x0CBC },
	{ 0x0CBF, 0x0CBF },
	{ 0x0CC6, 0x0CC6 },
	{ 0x0CCC, 0x0CCD },
	{ 0x0CE2, 0x0CE3 },
	{ 0x0D00, 0x0D01 },   /* Malayalam */
	{ 0x0D3B, 0x0D3C },
	{ 0x0D41, 0x0D44 },
	{ 0x0D4D, 0x0D4D },
	{ 0x0D62, 0x0D63 },
	{ 0x0DCA, 0x0DCA },   /* Sinhala */
	{ 0x0DD2, 0x0DD4 },
	{ 0x0DD6, 0x0DD6 },
	{ 0x0E31, 0x0E31 },   /* Thai */
	{ 0x0E34, 0x0E3A },
	{ 0x0E47, 0x0E4E },
	{ 0x0EB1, 0x0EB1 },   /* Lao */
	{ 0x0EB4, 0x0EBC },
	{ 0x0EC8, 0x0ECD },
	{ 0x0F18, 0x0F19 },   /* Tibetan */
	{ 0x0F35, 0x0F35 },
	{ 0x0F37, 0x0F37 },
	{ 0x0F39, 0x0F39 },
	{ 0x0F71, 0x0F7E },
	{ 0x0F80, 0x0F84 },
	{ 0x0F86, 0x0F87 },
	{ 0x0F8D, 0x0F97 },
	{ 0x0F99, 0x0FBC },
	{ 0x0FC6, 0x0FC6 },
	{ 0x1160, 0x11FF },   /* Hangul Jamo medial/final (composed in syllable blocks) */
	{ 0x135D, 0x135F },   /* Ethiopic combining */
	{ 0x1712, 0x1714 },   /* Tagalog */
	{ 0x1732, 0x1734 },   /* Hanunoo */
	{ 0x1752, 0x1753 },   /* Buhid */
	{ 0x1772, 0x1773 },   /* Tagbanwa */
	{ 0x17B4, 0x17B5 },   /* Khmer */
	{ 0x17B7, 0x17BD },
	{ 0x17C6, 0x17C6 },
	{ 0x17C9, 0x17D3 },
	{ 0x17DD, 0x17DD },
	{ 0x180B, 0x180E },   /* Mongolian free variation selectors + vowel separator */
	{ 0x1885, 0x1886 },   /* Mongolian combining */
	{ 0x18A9, 0x18A9 },
	{ 0x1920, 0x1922 },   /* Limbu */
	{ 0x1927, 0x1928 },
	{ 0x1932, 0x1932 },
	{ 0x1939, 0x193B },
	{ 0x1A17, 0x1A18 },   /* Buginese */
	{ 0x1A1B, 0x1A1B },
	{ 0x1A56, 0x1A56 },   /* Tai Tham */
	{ 0x1A58, 0x1A5E },
	{ 0x1A60, 0x1A60 },
	{ 0x1A62, 0x1A62 },
	{ 0x1A65, 0x1A6C },
	{ 0x1A73, 0x1A7C },
	{ 0x1A7F, 0x1A7F },
	{ 0x1AB0, 0x1ABE },   /* Combining Diacritical Marks Extended */
	{ 0x1B00, 0x1B03 },   /* Balinese */
	{ 0x1B34, 0x1B34 },
	{ 0x1B36, 0x1B3A },
	{ 0x1B3C, 0x1B3C },
	{ 0x1B42, 0x1B42 },
	{ 0x1B6B, 0x1B73 },
	{ 0x1B80, 0x1B81 },   /* Sundanese */
	{ 0x1BA2, 0x1BA5 },
	{ 0x1BA8, 0x1BA9 },
	{ 0x1BAB, 0x1BAD },
	{ 0x1BE6, 0x1BE6 },   /* Batak */
	{ 0x1BE8, 0x1BE9 },
	{ 0x1BED, 0x1BED },
	{ 0x1BEF, 0x1BF1 },
	{ 0x1C2C, 0x1C33 },   /* Lepcha */
	{ 0x1C36, 0x1C37 },
	{ 0x1CD0, 0x1CD2 },   /* Vedic Extensions */
	{ 0x1CD4, 0x1CE0 },
	{ 0x1CE2, 0x1CE8 },
	{ 0x1CED, 0x1CED },
	{ 0x1CF4, 0x1CF4 },
	{ 0x1CF8, 0x1CF9 },
	{ 0x1DC0, 0x1DF9 },   /* Combining Diacritical Marks Supplement */
	{ 0x1DFB, 0x1DFF },
	{ 0x200B, 0x200F },   /* Zero Width Space, ZWNJ, ZWJ, LRM, RLM */
	{ 0x202A, 0x202E },   /* Bidi control characters */
	{ 0x2060, 0x2064 },   /* Word Joiner, Invisible Plus, etc. */
	{ 0x2066, 0x206F },   /* Bidi isolates and other format chars */
	{ 0x20D0, 0x20F0 },   /* Combining Diacritical Marks for Symbols */
	{ 0x2CEF, 0x2CF1 },   /* Coptic combining */
	{ 0x2D7F, 0x2D7F },   /* Tifinagh combining */
	{ 0x2DE0, 0x2DFF },   /* Cyrillic Extended-A combining */
	{ 0xA66F, 0xA672 },   /* Cyrillic combining */
	{ 0xA674, 0xA67D },
	{ 0xA69E, 0xA69F },
	{ 0xA6F0, 0xA6F1 },   /* Bamum combining */
	{ 0xA802, 0xA802 },   /* Syloti Nagri */
	{ 0xA806, 0xA806 },
	{ 0xA80B, 0xA80B },
	{ 0xA825, 0xA826 },
	{ 0xA8C4, 0xA8C5 },   /* Saurashtra */
	{ 0xA8E0, 0xA8F1 },   /* Devanagari Extended */
	{ 0xA8FF, 0xA8FF },
	{ 0xA926, 0xA92D },   /* Kayah Li */
	{ 0xA947, 0xA951 },   /* Rejang */
	{ 0xA980, 0xA982 },   /* Javanese */
	{ 0xA9B3, 0xA9B3 },
	{ 0xA9B6, 0xA9B9 },
	{ 0xA9BC, 0xA9BD },
	{ 0xAA29, 0xAA2E },   /* Cham */
	{ 0xAA31, 0xAA32 },
	{ 0xAA35, 0xAA36 },
	{ 0xAA43, 0xAA43 },
	{ 0xAA4C, 0xAA4C },
	{ 0xAA7C, 0xAA7C },   /* Myanmar Extended-B */
	{ 0xAAB0, 0xAAB0 },   /* Tai Viet */
	{ 0xAAB2, 0xAAB4 },
	{ 0xAAB7, 0xAAB8 },
	{ 0xAABE, 0xAABF },
	{ 0xAAC1, 0xAAC1 },
	{ 0xAAEC, 0xAAED },   /* Meetei Mayek Extensions */
	{ 0xAAF6, 0xAAF6 },
	{ 0xABE5, 0xABE5 },   /* Meetei Mayek */
	{ 0xABE8, 0xABE8 },
	{ 0xABED, 0xABED },
	{ 0xFB1E, 0xFB1E },   /* Hebrew point Judeo-Spanish */
	{ 0xFE00, 0xFE0F },   /* Variation Selectors */
	{ 0xFE20, 0xFE2F },   /* Combining Half Marks */
	{ 0xFEFF, 0xFEFF },   /* BOM / ZWNBSP */
	{ 0xFFF9, 0xFFFB },   /* Interlinear annotation anchors */
	{ 0x101FD, 0x101FD },  /* Phaistos Disc combining */
	{ 0x102E0, 0x102E0 },  /* Coptic Epact combining */
	{ 0x10376, 0x1037A },  /* Old Permic combining */
	{ 0x10A01, 0x10A03 },  /* Kharoshthi */
	{ 0x10A05, 0x10A06 },
	{ 0x10A0C, 0x10A0F },
	{ 0x10A38, 0x10A3A },
	{ 0x10A3F, 0x10A3F },
	{ 0x10AE5, 0x10AE6 },  /* Manichaean */
	{ 0x10D24, 0x10D27 },  /* Hanifi Rohingya */
	{ 0x10F46, 0x10F50 },  /* Sogdian */
	{ 0x11001, 0x11001 },  /* Brahmi */
	{ 0x11038, 0x11046 },
	{ 0x1107F, 0x11081 },  /* Kaithi */
	{ 0x110B3, 0x110B6 },
	{ 0x110B9, 0x110BA },
	{ 0x11100, 0x11102 },  /* Chakma */
	{ 0x11127, 0x1112B },
	{ 0x1112D, 0x11134 },
	{ 0x11173, 0x11173 },  /* Mahajani */
	{ 0x11180, 0x11181 },  /* Sharada */
	{ 0x111B6, 0x111BE },
	{ 0x111C9, 0x111CC },
	{ 0x1122F, 0x11231 },  /* Khojki */
	{ 0x11234, 0x11234 },
	{ 0x11236, 0x11237 },
	{ 0x1123E, 0x1123E },
	{ 0x112DF, 0x112DF },  /* Khudawadi */
	{ 0x112E3, 0x112EA },
	{ 0x11300, 0x11301 },  /* Grantha */
	{ 0x1133B, 0x1133C },
	{ 0x11340, 0x11340 },
	{ 0x11366, 0x1136C },
	{ 0x11370, 0x11374 },
	{ 0x11438, 0x1143F },  /* Newa */
	{ 0x11442, 0x11444 },
	{ 0x11446, 0x11446 },
	{ 0x1145E, 0x1145E },
	{ 0x114B3, 0x114B8 },  /* Tirhuta */
	{ 0x114BA, 0x114BA },
	{ 0x114BF, 0x114C0 },
	{ 0x114C2, 0x114C3 },
	{ 0x115B2, 0x115B5 },  /* Siddham */
	{ 0x115BC, 0x115BD },
	{ 0x115BF, 0x115C0 },
	{ 0x115DC, 0x115DD },
	{ 0x11633, 0x1163A },  /* Modi */
	{ 0x1163D, 0x1163D },
	{ 0x1163F, 0x11640 },
	{ 0x116AB, 0x116AB },  /* Takri */
	{ 0x116AD, 0x116AD },
	{ 0x116B0, 0x116B5 },
	{ 0x116B7, 0x116B7 },
	{ 0x1171D, 0x1171F },  /* Ahom */
	{ 0x11722, 0x11725 },
	{ 0x11727, 0x1172B },
	{ 0x1182F, 0x11837 },  /* Dogra */
	{ 0x11839, 0x1183A },
	{ 0x11A01, 0x11A0A },  /* Zanabazar Square */
	{ 0x11A33, 0x11A38 },
	{ 0x11A3B, 0x11A3E },
	{ 0x11A47, 0x11A47 },
	{ 0x11A51, 0x11A56 },  /* Soyombo */
	{ 0x11A59, 0x11A5B },
	{ 0x11A8A, 0x11A96 },
	{ 0x11A98, 0x11A99 },
	{ 0x11C30, 0x11C36 },  /* Bhaiksuki */
	{ 0x11C38, 0x11C3D },
	{ 0x11C3F, 0x11C3F },
	{ 0x11C92, 0x11CA7 },  /* Marchen */
	{ 0x11CAA, 0x11CB0 },
	{ 0x11CB2, 0x11CB3 },
	{ 0x11CB5, 0x11CB6 },
	{ 0x11D31, 0x11D36 },  /* Masaram Gondi */
	{ 0x11D3A, 0x11D3A },
	{ 0x11D3C, 0x11D3D },
	{ 0x11D3F, 0x11D45 },
	{ 0x11D47, 0x11D47 },
	{ 0x11D90, 0x11D91 },  /* Gunjala Gondi */
	{ 0x11D95, 0x11D95 },
	{ 0x11D97, 0x11D97 },
	{ 0x11EF3, 0x11EF4 },  /* Makasar */
	{ 0x16AF0, 0x16AF4 },  /* Bassa Vah */
	{ 0x16B30, 0x16B36 },  /* Pahawh Hmong */
	{ 0x1BC9D, 0x1BC9E },  /* Duployan */
	{ 0x1D167, 0x1D169 },  /* Musical Symbols combining */
	{ 0x1D17B, 0x1D182 },
	{ 0x1D185, 0x1D18B },
	{ 0x1D1AA, 0x1D1AD },
	{ 0x1D242, 0x1D244 },  /* Combining Greek Musical */
	{ 0x1DA00, 0x1DA36 },  /* Signwriting */
	{ 0x1DA3B, 0x1DA6C },
	{ 0x1DA75, 0x1DA75 },
	{ 0x1DA84, 0x1DA84 },
	{ 0x1DA9B, 0x1DA9F },
	{ 0x1DAA1, 0x1DAAF },
	{ 0x1E000, 0x1E006 },  /* Glagolitic Supplement */
	{ 0x1E008, 0x1E018 },
	{ 0x1E01B, 0x1E021 },
	{ 0x1E023, 0x1E024 },
	{ 0x1E026, 0x1E02A },
	{ 0x1E130, 0x1E136 },  /* Nyiakeng Puachue Hmong */
	{ 0x1E2EC, 0x1E2EF },  /* Wancho */
	{ 0x1E8D0, 0x1E8D6 },  /* Mende Kikakui */
	{ 0xE0001, 0xE0001 },  /* Language Tag */
	{ 0xE0020, 0xE007F },  /* Tag components */
	{ 0xE0100, 0xE01EF },  /* Variation Selectors Supplement */
};

#define ZERO_WIDTH_COUNT (int)( sizeof( zero_width_ranges ) / sizeof( zero_width_ranges[0] ) )

/*
 * Wide (double-column) character ranges:
 * East Asian Wide and Fullwidth characters per Unicode EAW property.
 */
static const unicode_range wide_ranges[] = {
	{ 0x1100, 0x115F },   /* Hangul Jamo initial consonants */
	{ 0x231A, 0x231B },   /* Watch, Hourglass (emoji) */
	{ 0x2329, 0x232A },   /* Left/Right-Pointing Angle Bracket */
	{ 0x23E9, 0x23F3 },   /* Various emoji */
	{ 0x23F8, 0x23FA },
	{ 0x25FD, 0x25FE },   /* Square emoji */
	{ 0x2614, 0x2615 },   /* Umbrella, Hot Beverage */
	{ 0x2648, 0x2653 },   /* Zodiac signs */
	{ 0x267F, 0x267F },   /* Wheelchair */
	{ 0x2693, 0x2693 },   /* Anchor */
	{ 0x26A1, 0x26A1 },   /* High Voltage */
	{ 0x26AA, 0x26AB },   /* Circles */
	{ 0x26BD, 0x26BE },   /* Soccer, Baseball */
	{ 0x26C4, 0x26C5 },   /* Snowman, Sun behind cloud */
	{ 0x26CE, 0x26CE },   /* Ophiuchus */
	{ 0x26D4, 0x26D4 },   /* No Entry */
	{ 0x26EA, 0x26EA },   /* Church */
	{ 0x26F2, 0x26F3 },   /* Fountain, Golf */
	{ 0x26F5, 0x26F5 },   /* Sailboat */
	{ 0x26FA, 0x26FA },   /* Tent */
	{ 0x26FD, 0x26FD },   /* Fuel Pump */
	{ 0x2702, 0x2702 },   /* Scissors */
	{ 0x2705, 0x2705 },   /* Check Mark */
	{ 0x2708, 0x270D },   /* Various symbols */
	{ 0x270F, 0x270F },   /* Pencil */
	{ 0x2712, 0x2712 },   /* Black Nib */
	{ 0x2714, 0x2714 },   /* Check Mark */
	{ 0x2716, 0x2716 },   /* X Mark */
	{ 0x271D, 0x271D },   /* Latin Cross */
	{ 0x2721, 0x2721 },   /* Star of David */
	{ 0x2728, 0x2728 },   /* Sparkles */
	{ 0x2733, 0x2734 },   /* Eight Spoked Asterisk, Eight Pointed Star */
	{ 0x2744, 0x2744 },   /* Snowflake */
	{ 0x2747, 0x2747 },   /* Sparkle */
	{ 0x274C, 0x274C },   /* Cross Mark */
	{ 0x274E, 0x274E },   /* Cross Mark */
	{ 0x2753, 0x2755 },   /* Question/Exclamation marks */
	{ 0x2757, 0x2757 },   /* Exclamation Mark */
	{ 0x2763, 0x2764 },   /* Heart Exclamation, Heart */
	{ 0x2795, 0x2797 },   /* Plus, Minus, Division */
	{ 0x27A1, 0x27A1 },   /* Right Arrow */
	{ 0x27B0, 0x27B0 },   /* Curly Loop */
	{ 0x27BF, 0x27BF },   /* Double Curly Loop */
	{ 0x2934, 0x2935 },   /* Arrows */
	{ 0x2B05, 0x2B07 },   /* Arrows */
	{ 0x2B1B, 0x2B1C },   /* Squares */
	{ 0x2B50, 0x2B50 },   /* Star */
	{ 0x2B55, 0x2B55 },   /* Circle */
	{ 0x2E80, 0x303E },   /* CJK Radicals Supplement through CJK Symbols */
	{ 0x3041, 0x33BF },   /* Hiragana, Katakana, Bopomofo, CJK Compatibility */
	{ 0x3400, 0x4DBF },   /* CJK Unified Ideographs Extension A */
	{ 0x4E00, 0xA4CF },   /* CJK Unified Ideographs through Yi Radicals */
	{ 0xA960, 0xA97F },   /* Hangul Jamo Extended-A */
	{ 0xAC00, 0xD7A3 },   /* Hangul Syllables */
	{ 0xF900, 0xFAFF },   /* CJK Compatibility Ideographs */
	{ 0xFE10, 0xFE19 },   /* Vertical Forms */
	{ 0xFE30, 0xFE6F },   /* CJK Compatibility Forms + Small Form Variants */
	{ 0xFF01, 0xFF60 },   /* Fullwidth Forms */
	{ 0xFFE0, 0xFFE6 },   /* Fullwidth Sign */
	{ 0x16FE0, 0x16FFF },  /* Ideographic Symbols */
	{ 0x17000, 0x187FF },  /* Tangut */
	{ 0x18800, 0x18AFF },  /* Tangut Components */
	{ 0x1B000, 0x1B12F },  /* Kana Supplement + Extended-A */
	{ 0x1B130, 0x1B16F },  /* Kana Extended-B + Small Kana */
	{ 0x1B170, 0x1B2FF },  /* Nushu */
	{ 0x1F004, 0x1F004 },  /* Mahjong Tile Red Dragon */
	{ 0x1F0CF, 0x1F0CF },  /* Playing Card Black Joker */
	{ 0x1F18E, 0x1F18E },  /* Negative Squared AB */
	{ 0x1F191, 0x1F19A },  /* Squared CL..Squared VS */
	{ 0x1F1E0, 0x1F1FF },  /* Regional Indicators (flag pairs) */
	{ 0x1F200, 0x1F202 },  /* Enclosed Ideographic Supplement */
	{ 0x1F210, 0x1F23B },
	{ 0x1F240, 0x1F248 },
	{ 0x1F250, 0x1F251 },
	{ 0x1F260, 0x1F265 },
	{ 0x1F300, 0x1F320 },  /* Miscellaneous Symbols and Pictographs */
	{ 0x1F32D, 0x1F335 },
	{ 0x1F337, 0x1F37C },
	{ 0x1F37E, 0x1F393 },
	{ 0x1F3A0, 0x1F3CA },
	{ 0x1F3CF, 0x1F3D3 },
	{ 0x1F3E0, 0x1F3F0 },
	{ 0x1F3F4, 0x1F3F4 },
	{ 0x1F3F8, 0x1F43E },
	{ 0x1F440, 0x1F440 },
	{ 0x1F442, 0x1F4FC },
	{ 0x1F4FF, 0x1F53D },
	{ 0x1F54B, 0x1F54E },
	{ 0x1F550, 0x1F567 },
	{ 0x1F57A, 0x1F57A },
	{ 0x1F595, 0x1F596 },
	{ 0x1F5A4, 0x1F5A4 },
	{ 0x1F5FB, 0x1F64F },
	{ 0x1F680, 0x1F6C5 },
	{ 0x1F6CC, 0x1F6CC },
	{ 0x1F6D0, 0x1F6D2 },
	{ 0x1F6D5, 0x1F6D7 },
	{ 0x1F6EB, 0x1F6EC },
	{ 0x1F6F4, 0x1F6FC },
	{ 0x1F7E0, 0x1F7EB },
	{ 0x1F90C, 0x1F93A },
	{ 0x1F93C, 0x1F945 },
	{ 0x1F947, 0x1F978 },
	{ 0x1F97A, 0x1F9CB },
	{ 0x1F9CD, 0x1F9FF },
	{ 0x1FA00, 0x1FA53 },
	{ 0x1FA60, 0x1FA6D },
	{ 0x1FA70, 0x1FA74 },
	{ 0x1FA78, 0x1FA7A },
	{ 0x1FA80, 0x1FA86 },
	{ 0x1FA90, 0x1FAA8 },
	{ 0x1FAB0, 0x1FAB6 },
	{ 0x1FAC0, 0x1FAC2 },
	{ 0x1FAD0, 0x1FAD6 },
	{ 0x20000, 0x2FFFD },  /* CJK Unified Ideographs Extension B+ */
	{ 0x30000, 0x3FFFD },  /* CJK Unified Ideographs Extension G+ */
};

#define WIDE_COUNT (int)( sizeof( wide_ranges ) / sizeof( wide_ranges[0] ) )

/*
 * Binary search a sorted range table.
 */
static bool in_range_table( unsigned int cp, const unicode_range *table, int count ) {
	int lo = 0, hi = count - 1;

	if ( cp < table[0].first || cp > table[hi].last )
		return FALSE;

	while ( lo <= hi ) {
		int mid = ( lo + hi ) / 2;
		if ( cp < table[mid].first )
			hi = mid - 1;
		else if ( cp > table[mid].last )
			lo = mid + 1;
		else
			return TRUE;
	}
	return FALSE;
}

/*
 * Return the display column width of a Unicode codepoint.
 *   0 = zero-width (combining, ZWJ, etc.)
 *   1 = normal width
 *   2 = CJK wide / fullwidth
 *  -1 = non-printable control character
 */
int utf8_wcwidth( unsigned int cp ) {
	/* C0/C1 control characters */
	if ( cp < 0x20 || ( cp >= 0x7F && cp < 0xA0 ) )
		return -1;

	/* DEL through non-breaking space - handled above */

	/* Fast ASCII path */
	if ( cp < 0x7F )
		return 1;

	/* Soft hyphen */
	if ( cp == 0x00AD )
		return 1;

	/* Zero-width check (must be before wide check) */
	if ( in_range_table( cp, zero_width_ranges, ZERO_WIDTH_COUNT ) )
		return 0;

	/* Wide character check */
	if ( in_range_table( cp, wide_ranges, WIDE_COUNT ) )
		return 2;

	/* Everything else is single-width */
	return 1;
}

/* ======================================================================
 * String-level queries
 * ====================================================================== */

/*
 * Count Unicode codepoints in a NUL-terminated UTF-8 string.
 * Invalid sequences each count as one codepoint (replacement char).
 */
int utf8_strlen( const char *str ) {
	int count = 0;

	if ( str == NULL )
		return 0;

	while ( *str != '\0' ) {
		utf8_decode( &str );
		count++;
	}
	return count;
}

/*
 * Calculate display column width of a raw UTF-8 string.
 * Does not skip color codes. For color-aware width, use utf8_visible_width().
 */
int utf8_display_width( const char *str ) {
	int width = 0;
	int w;

	if ( str == NULL )
		return 0;

	while ( *str != '\0' ) {
		unsigned int cp = utf8_decode( &str );
		w = utf8_wcwidth( cp );
		if ( w > 0 )
			width += w;
	}
	return width;
}

/*
 * Calculate display column width of a UTF-8 string, skipping MUD color codes.
 * Handles both standard (#X) and extended (#xNNN) color codes.
 *
 * This replaces both visible_strlen() and col_str_len() for UTF-8 strings.
 */
int utf8_visible_width( const char *str ) {
	int width = 0;
	const char *p = str;
	int w;

	if ( str == NULL )
		return 0;

	while ( *p != '\0' ) {
		if ( *p == '#' && *( p + 1 ) != '\0' ) {
			char next = *( p + 1 );

			/* Check for extended color code #xNNN (e.g., #x196) */
			if ( ( next == 'x' || next == 'X' )
				&& p[2] >= '0' && p[2] <= '9'
				&& p[3] >= '0' && p[3] <= '9'
				&& p[4] >= '0' && p[4] <= '9' ) {
				p += 5;
				continue;
			}

			/* Check for true color #tRRGGBB or #TRRGGBB (8 chars total) */
			if ( ( next == 't' || next == 'T' )
				&& isxdigit( (unsigned char) p[2] )
				&& isxdigit( (unsigned char) p[3] )
				&& isxdigit( (unsigned char) p[4] )
				&& isxdigit( (unsigned char) p[5] )
				&& isxdigit( (unsigned char) p[6] )
				&& isxdigit( (unsigned char) p[7] ) ) {
				p += 8;
				continue;
			}

			/* Standard color code #X (e.g., #R, #G, #n) */
			if ( next == '#' || next == '-' ) {
				/* ## = literal #, #- = literal ~ : these produce visible chars */
				width++;
				p += 2;
				continue;
			}

			/* All other # codes are non-visible formatting */
			p += 2;
			continue;
		}

		/* Regular character - decode and measure */
		{
			unsigned int cp = utf8_decode( &p );
			w = utf8_wcwidth( cp );
			if ( w > 0 )
				width += w;
		}
	}
	return width;
}

/* ======================================================================
 * Validation
 * ====================================================================== */

/*
 * Validate that a byte sequence is well-formed UTF-8.
 * If len < 0, scans until NUL terminator.
 * Returns TRUE if all bytes form valid, complete UTF-8 sequences.
 */
bool utf8_is_valid( const char *str, int len ) {
	const unsigned char *p = (const unsigned char *) str;
	const unsigned char *end;
	int seq;

	if ( str == NULL )
		return FALSE;

	end = ( len < 0 ) ? NULL : p + len;

	while ( end ? ( p < end ) : ( *p != '\0' ) ) {
		if ( *p < 0x80 ) {
			p++;
			continue;
		}

		seq = utf8_seq_len( *p );
		if ( seq == 0 )
			return FALSE;

		/* Check we have enough bytes */
		if ( end && p + seq > end )
			return FALSE;
		if ( !end ) {
			int j;
			for ( j = 1; j < seq; j++ ) {
				if ( p[j] == '\0' )
					return FALSE;
			}
		}

		/* Validate continuation bytes */
		{
			int j;
			for ( j = 1; j < seq; j++ ) {
				if ( !utf8_is_cont( p[j] ) )
					return FALSE;
			}
		}

		/* Decode and check for overlong / surrogates / out-of-range */
		{
			unsigned int cp = 0;
			switch ( seq ) {
			case 2:
				cp = ( (unsigned int)( p[0] & 0x1F ) << 6 )
				   | (unsigned int)( p[1] & 0x3F );
				if ( cp < 0x80 ) return FALSE; /* overlong */
				break;
			case 3:
				cp = ( (unsigned int)( p[0] & 0x0F ) << 12 )
				   | ( (unsigned int)( p[1] & 0x3F ) << 6 )
				   | (unsigned int)( p[2] & 0x3F );
				if ( cp < 0x0800 ) return FALSE;
				if ( cp >= 0xD800 && cp <= 0xDFFF ) return FALSE;
				break;
			case 4:
				cp = ( (unsigned int)( p[0] & 0x07 ) << 18 )
				   | ( (unsigned int)( p[1] & 0x3F ) << 12 )
				   | ( (unsigned int)( p[2] & 0x3F ) << 6 )
				   | (unsigned int)( p[3] & 0x3F );
				if ( cp < 0x10000 || cp > 0x10FFFF ) return FALSE;
				break;
			}
		}

		p += seq;
	}
	return TRUE;
}

/* ======================================================================
 * Safe operations
 * ====================================================================== */

/*
 * Find the largest byte count <= max_bytes that does not split a
 * UTF-8 multi-byte sequence. For use with chunked socket writes.
 *
 * Scans backward from position max_bytes to find a safe split point.
 * Returns max_bytes if it already falls on a character boundary.
 */
int utf8_truncate( const char *str, int max_bytes ) {
	int pos;

	if ( str == NULL || max_bytes <= 0 )
		return 0;

	pos = max_bytes;

	/* If we're at a lead byte or ASCII, we're on a boundary already */
	if ( utf8_is_lead( (unsigned char) str[pos] ) )
		return pos;

	/* We're on a continuation byte - back up to the lead byte */
	while ( pos > 0 && utf8_is_cont( (unsigned char) str[pos] ) )
		pos--;

	/* Now pos is on a lead byte. Check if the full sequence fits. */
	if ( pos >= 0 ) {
		int seq = utf8_seq_len( (unsigned char) str[pos] );
		if ( pos + seq <= max_bytes )
			return pos + seq; /* full sequence fits */
	}

	/* Sequence extends past max_bytes, so exclude it */
	return pos;
}

/*
 * Pad a UTF-8 string to exactly 'width' display columns with trailing spaces.
 * If the string is already wider than 'width', it is copied without padding.
 * Output is NUL-terminated and fits within bufsize bytes.
 */
void utf8_pad_right( char *buf, int bufsize, const char *str, int width ) {
	int str_width;
	int str_bytes;
	int pad;
	int i;

	if ( buf == NULL || bufsize <= 0 )
		return;

	if ( str == NULL )
		str = "";

	str_width = utf8_visible_width( str );
	str_bytes = (int) strlen( str );

	/* Copy the string (as much as fits) */
	if ( str_bytes >= bufsize - 1 ) {
		str_bytes = utf8_truncate( str, bufsize - 1 );
		memcpy( buf, str, str_bytes );
		buf[str_bytes] = '\0';
		return;
	}

	memcpy( buf, str, str_bytes );

	/* Add padding spaces */
	pad = width - str_width;
	if ( pad < 0 )
		pad = 0;

	for ( i = 0; i < pad && str_bytes + i < bufsize - 1; i++ )
		buf[str_bytes + i] = ' ';

	buf[str_bytes + i] = '\0';
}

/* ======================================================================
 * Confusable character support
 * ====================================================================== */

/* Global confusable table -- populated by db_game_load_confusables() */
CONFUSABLE_ENTRY *confusable_table = NULL;
int confusable_count = 0;

/* ======================================================================
 * Name validation support
 * ====================================================================== */

/*
 * Return TRUE if a Unicode codepoint is valid in a player name.
 *
 * Allowed ranges:
 *   ASCII letters:     U+0041-005A, U+0061-007A
 *   Latin Extended:    U+00C0-024F (accented Latin, excludes U+00D7, U+00F7)
 *   Cyrillic:          U+0400-04FF
 *   CJK Unified:       U+4E00-9FFF
 *   CJK Extension A:   U+3400-4DBF
 *   Hiragana:          U+3040-309F
 *   Katakana:          U+30A0-30FF
 *   Hangul Syllables:  U+AC00-D7AF
 */
bool utf8_is_name_char( unsigned int cp ) {
	/* ASCII letters */
	if ( ( cp >= 'A' && cp <= 'Z' ) || ( cp >= 'a' && cp <= 'z' ) )
		return TRUE;

	/* Latin Extended (skip multiplication U+00D7 and division U+00F7 signs) */
	if ( cp >= 0x00C0 && cp <= 0x024F && cp != 0x00D7 && cp != 0x00F7 )
		return TRUE;

	/* Cyrillic */
	if ( cp >= 0x0400 && cp <= 0x04FF )
		return TRUE;

	/* CJK Unified Ideographs */
	if ( cp >= 0x4E00 && cp <= 0x9FFF )
		return TRUE;

	/* CJK Extension A */
	if ( cp >= 0x3400 && cp <= 0x4DBF )
		return TRUE;

	/* Hiragana */
	if ( cp >= 0x3040 && cp <= 0x309F )
		return TRUE;

	/* Katakana */
	if ( cp >= 0x30A0 && cp <= 0x30FF )
		return TRUE;

	/* Hangul Syllables */
	if ( cp >= 0xAC00 && cp <= 0xD7AF )
		return TRUE;

	return FALSE;
}

/*
 * Determine the script group of a codepoint for mixed-script detection.
 * Returns:
 *   0 = common/unknown (not a valid name character)
 *   1 = Latin (ASCII a-z/A-Z + Latin Extended U+00C0-024F)
 *   2 = Cyrillic (U+0400-04FF)
 *   3 = East Asian (CJK + Hiragana + Katakana + Hangul)
 */
int utf8_char_script( unsigned int cp ) {
	if ( ( cp >= 'A' && cp <= 'Z' ) || ( cp >= 'a' && cp <= 'z' ) )
		return 1;
	if ( cp >= 0x00C0 && cp <= 0x024F )
		return 1;
	if ( cp >= 0x0400 && cp <= 0x04FF )
		return 2;
	if ( ( cp >= 0x3040 && cp <= 0x30FF )   /* Hiragana + Katakana */
		|| ( cp >= 0x3400 && cp <= 0x4DBF )  /* CJK Extension A */
		|| ( cp >= 0x4E00 && cp <= 0x9FFF )  /* CJK Unified */
		|| ( cp >= 0xAC00 && cp <= 0xD7AF ) ) /* Hangul */
		return 3;
	return 0;
}

/*
 * Normalize a string to its ASCII "skeleton" for profanity detection.
 *
 * Processing for each codepoint:
 *   1. Fullwidth ASCII (U+FF01-FF5E) -> ASCII equivalent, lowercased
 *   2. Confusable table match -> write canonical (already lowercase)
 *   3. ASCII -> write lowercase
 *   4. Non-ASCII not in table -> encode back as UTF-8 unchanged
 *
 * Returns byte length of the NUL-terminated output.
 */
int utf8_skeletonize( const char *input, char *output, int max_out ) {
	const char *p = input;
	int out = 0;

	if ( !input || !output || max_out <= 0 )
		return 0;

	while ( *p != '\0' && out < max_out - 1 ) {
		unsigned int cp = utf8_decode( &p );

		/* Fullwidth ASCII: U+FF01-FF5E -> 0x21-0x7E */
		if ( cp >= 0xFF01 && cp <= 0xFF5E ) {
			char ascii = (char)( cp - 0xFEE0 );
			if ( ascii >= 'A' && ascii <= 'Z' )
				ascii += ( 'a' - 'A' );
			output[out++] = ascii;
			continue;
		}

		/* Binary search confusable table */
		if ( confusable_table != NULL && confusable_count > 0 ) {
			int lo = 0, hi = confusable_count - 1;
			bool found = FALSE;

			while ( lo <= hi ) {
				int mid = ( lo + hi ) / 2;

				if ( confusable_table[mid].codepoint == cp ) {
					const char *can = confusable_table[mid].canonical;
					while ( *can != '\0' && out < max_out - 1 )
						output[out++] = *can++;
					found = TRUE;
					break;
				}
				if ( confusable_table[mid].codepoint < cp )
					lo = mid + 1;
				else
					hi = mid - 1;
			}

			if ( found )
				continue;
		}

		/* ASCII: write lowercase */
		if ( cp < 0x80 ) {
			char ch = (char) cp;
			if ( ch >= 'A' && ch <= 'Z' )
				ch += ( 'a' - 'A' );
			output[out++] = ch;
			continue;
		}

		/* Non-ASCII not in table: encode back as UTF-8 */
		{
			char buf[5];
			int len = utf8_encode( cp, buf );
			int j;
			for ( j = 0; j < len && out < max_out - 1; j++ )
				output[out++] = buf[j];
		}
	}

	output[out] = '\0';
	return out;
}
