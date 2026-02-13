/*
 * utf8.h - UTF-8 encoding utilities
 *
 * Zero-dependency, cross-platform UTF-8 string handling.
 * Provides byte-level queries, codepoint decode/encode, display width
 * calculation (including CJK wide characters), validation, and safe
 * truncation for buffer boundaries.
 */

#ifndef UTF8_H
#define UTF8_H

/* === Byte-level queries === */

/* TRUE if byte is a UTF-8 lead byte (0xxxxxxx or 11xxxxxx) */
#define utf8_is_lead( b ) ( ( (unsigned char)(b) & 0xC0 ) != 0x80 )

/* TRUE if byte is a UTF-8 continuation byte (10xxxxxx) */
#define utf8_is_cont( b ) ( ( (unsigned char)(b) & 0xC0 ) == 0x80 )

/* Expected sequence length for a lead byte (1-4), or 0 if invalid */
int utf8_seq_len( unsigned char lead_byte );

/* === Codepoint decode/encode === */

/* Decode one codepoint from *str, advance *str past it.
 * Returns the codepoint, or 0xFFFD (replacement char) on invalid input.
 * Stops at '\0'. */
unsigned int utf8_decode( const char **str );

/* Encode one codepoint into buf (must have room for 5 bytes).
 * Returns number of bytes written (1-4), or 0 on invalid codepoint. */
int utf8_encode( unsigned int codepoint, char *buf );

/* === String-level queries === */

/* Count Unicode codepoints in a NUL-terminated UTF-8 string */
int utf8_strlen( const char *str );

/* Display column width of a raw UTF-8 string (no color code awareness) */
int utf8_display_width( const char *str );

/* Display column width, skipping MUD color codes (#X and #xNNN) */
int utf8_visible_width( const char *str );

/* === Display width (wcwidth equivalent) === */

/* Column width of a Unicode codepoint:
 *  0 for zero-width (combining marks, ZWJ, variation selectors)
 *  1 for normal characters
 *  2 for CJK wide / fullwidth characters
 * -1 for non-printable control characters */
int utf8_wcwidth( unsigned int codepoint );

/* === Validation === */

/* TRUE if the first 'len' bytes form valid UTF-8.
 * If len < 0, scans until NUL. */
bool utf8_is_valid( const char *str, int len );

/* === Safe operations === */

/* Largest byte count <= max_bytes that does not split a UTF-8 sequence.
 * Use before chunked writes to prevent mid-character splits. */
int utf8_truncate( const char *str, int max_bytes );

/* Pad str to exactly 'width' display columns with trailing spaces.
 * Writes into buf (up to bufsize bytes including NUL). */
void utf8_pad_right( char *buf, int bufsize, const char *str, int width );

/* === Confusable character support === */

/* Entry mapping a Unicode codepoint to its ASCII canonical form.
 * Used for skeleton normalization to detect profanity bypass via
 * lookalike characters (e.g., Cyrillic 'с' U+0441 -> Latin 'c'). */
typedef struct {
	unsigned int codepoint;     /* Unicode codepoint */
	char canonical[8];          /* ASCII canonical form, NUL-terminated */
} CONFUSABLE_ENTRY;

/* Confusable table (populated by db_game_load_confusables at boot) */
extern CONFUSABLE_ENTRY *confusable_table;
extern int confusable_count;

/* === Name validation support === */

/* TRUE if codepoint is valid in a player name.
 * Allows: ASCII letters, Latin Extended, Cyrillic, CJK, Kana, Hangul.
 * Rejects: digits, punctuation, symbols, emoji, control, zero-width. */
bool utf8_is_name_char( unsigned int codepoint );

/* Determine the script group of a codepoint for mixed-script detection.
 * Returns: 0=common/unknown, 1=Latin, 2=Cyrillic, 3=East Asian (CJK/Kana/Hangul) */
int utf8_char_script( unsigned int codepoint );

/* Normalize a name to its ASCII skeleton for profanity detection.
 * Replaces confusable Unicode chars with ASCII equivalents, lowercases ASCII.
 * Fullwidth ASCII (U+FF01-FF5E) is converted algorithmically.
 * Returns byte length of output (NUL-terminated). */
int utf8_skeletonize( const char *input, char *output, int max_out );

#endif /* UTF8_H */
