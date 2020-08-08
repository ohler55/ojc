// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "oj.h"
#include "intern.h"

#define USE_THREAD_LIMIT	100000

// Give better performance with indented JSON but worse with unindented.
//#define SPACE_JUMP

enum {
    SKIP_CHAR		= 'a',
    SKIP_NEWLINE	= 'b',
    VAL_NULL		= 'c',
    VAL_TRUE		= 'd',
    VAL_FALSE		= 'e',
    VAL_NEG		= 'f',
    VAL0		= 'g',
    VAL_DIGIT		= 'h',
    VAL_QUOTE		= 'i',
    OPEN_ARRAY		= 'k',
    OPEN_OBJECT		= 'l',
    CLOSE_ARRAY		= 'm',
    CLOSE_OBJECT	= 'n',
    AFTER_COMMA		= 'o',
    KEY_QUOTE		= 'p',
    COLON_COLON		= 'q',
    NUM_SPC		= 'r',
    NUM_NEWLINE		= 's',
    NUM_DOT		= 't',
    NUM_COMMA		= 'u',
    NUM_FRAC		= 'v',
    FRAC_E		= 'w',
    EXP_SIGN		= 'x',
    EXP_DIGIT		= 'y',
    STR_QUOTE		= 'z',
    NEG_DIGIT		= '-',
    STR_SLASH		= 'A',
    ESC_OK		= 'B',
    U_OK		= 'E',
    TOKEN_OK		= 'F',
    NUM_CLOSE_OBJECT	= 'G',
    NUM_CLOSE_ARRAY	= 'H',
    NUM_DIGIT		= 'N',
    NUM_ZERO		= 'O',
    STR_OK		= 'R',
    ESC_U		= 'U',
    CHAR_ERR		= '.',
    DONE		= 'X',
};

typedef struct _ojValidator {
    const char		*map;
    const char		*next_map;
    struct _ojErr	err;
    int			depth;
    int			ri;
    // TBD change this stack to be expandable
    char		stack[256];
} *ojValidator;

typedef struct _ReadBlock {
    pthread_mutex_t	mu;
    ojStatus		status;
    bool		eof;
    byte		buf[16385];
} *ReadBlock;

typedef struct _ReadCtx {
    struct _ReadBlock	blocks[4];
    ReadBlock		bend;
    int			fd;
} *ReadCtx;

/*
0123456789abcdef0123456789abcdef */
static const char	value_map[257] = "\
X........ab..a..................\
a.i..........f..ghhhhhhhhh......\
...........................k.m..\
......e.......c.....d......l.n..\
................................\
................................\
................................\
................................v";

static const char	null_map[257] = "\
................................\
............o...................\
................................\
............F........F..........\
................................\
................................\
................................\
................................N";

static const char	true_map[257] = "\
................................\
............o...................\
................................\
.....F............F..F..........\
................................\
................................\
................................\
................................T";

static const char	false_map[257] = "\
................................\
............o...................\
................................\
.F...F......F......F............\
................................\
................................\
................................\
................................F";

static const char	comma_map[257] = "\
.........ab..a..................\
a.i..........f..ghhhhhhhhh......\
...........................k....\
......e.......c.....d......l....\
................................\
................................\
................................\
................................,";

static const char	after_map[257] = "\
X........ab..a..................\
a...........o...................\
.............................m..\
.............................n..\
................................\
................................\
................................\
................................a";

static const char	key1_map[257] = "\
.........ab..a..................\
a.p.............................\
................................\
.............................n..\
................................\
................................\
................................\
................................K";

static const char	key_map[257] = "\
.........ab..a..................\
a.p.............................\
................................\
................................\
................................\
................................\
................................\
................................k";

static const char	colon_map[257] = "\
.........ab..a..................\
a.........................q.....\
................................\
................................\
................................\
................................\
................................\
................................:";

static const char	neg_map[257] = "\
................................\
................O---------......\
................................\
................................\
................................\
................................\
................................\
................................-";

static const char	zero_map[257] = "\
.........rs..r..................\
r...........u.t.................\
.............................H..\
.............................G..\
................................\
................................\
................................\
................................0";

static const char	digit_map[257] = "\
.........rs..r..................\
r...........u.t.NNNNNNNNNN......\
.............................H..\
.............................G..\
................................\
................................\
................................\
................................d";

static const char	dot_map[257] = "\
................................\
................vvvvvvvvvv......\
................................\
................................\
................................\
................................\
................................\
.................................";

static const char	frac_map[257] = "\
.........rs..r..................\
r...........u...vvvvvvvvvv......\
.....w.......................H..\
.....w.......................G..\
................................\
................................\
................................\
................................f";

static const char	exp_sign_map[257] = "\
................................\
...........x.x..yyyyyyyyyy......\
................................\
................................\
................................\
................................\
................................\
................................x";

static const char	exp_zero_map[257] = "\
................................\
................yyyyyyyyyy......\
................................\
................................\
................................\
................................\
................................\
................................z";

static const char	exp_map[257] = "\
.........rs..r..................\
r...........u...yyyyyyyyyy......\
.............................H..\
.............................G..\
................................\
................................\
................................\
................................X";

static const char	string_map[257] = "\
................................\
RRzRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\
RRRRRRRRRRRRRRRRRRRRRRRRRRRRARRR\
RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\
RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\
RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\
RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\
RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRs";

static const char	esc_map[257] = "\
................................\
..B............B................\
............................B...\
..B...B.......B...B.BU..........\
................................\
................................\
................................\
................................~";

static const char	esc_byte_map[257] = "\
................................\
..\"............/................\
............................\\...\
..\b...\f.......\n...\r.\t..........\
................................\
................................\
................................\
................................b";

static const char	u_map[257] = "\
................................\
................EEEEEEEEEE......\
.EEEEEE.........................\
.EEEEEE.........................\
................................\
................................\
................................\
................................u";

static const char	space_map[257] = "\
.........ab..a..................\
a...............................\
................................\
................................\
................................\
................................\
................................\
................................S";

static const byte	hex_map[256] = "\
................................\
................\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09......\
.\x0a\x0b\x0c\x0d\x0e\x0f.........................\
.\x0a\x0b\x0c\x0d\x0e\x0f.........................\
................................\
................................\
................................\
................................";

// Works with extended unicode as well. \Uffffffff if support is desired in
// the future.
static size_t
unicodeToUtf8(uint32_t code, byte *buf) {
    byte	*start = buf;

    if (0x0000007F >= code) {
	*buf++ = (byte)code;
    } else if (0x000007FF >= code) {
	*buf++ = 0xC0 | (code >> 6);
	*buf++ = 0x80 | (0x3F & code);
    } else if (0x0000FFFF >= code) {
	*buf++ = 0xE0 | (code >> 12);
	*buf++ = 0x80 | ((code >> 6) & 0x3F);
	*buf++ = 0x80 | (0x3F & code);
    } else if (0x001FFFFF >= code) {
	*buf++ = 0xF0 | (code >> 18);
	*buf++ = 0x80 | ((code >> 12) & 0x3F);
	*buf++ = 0x80 | ((code >> 6) & 0x3F);
	*buf++ = 0x80 | (0x3F & code);
    } else if (0x03FFFFFF >= code) {
	*buf++ = 0xF8 | (code >> 24);
	*buf++ = 0x80 | ((code >> 18) & 0x3F);
	*buf++ = 0x80 | ((code >> 12) & 0x3F);
	*buf++ = 0x80 | ((code >> 6) & 0x3F);
	*buf++ = 0x80 | (0x3F & code);
    } else if (0x7FFFFFFF >= code) {
	*buf++ = 0xFC | (code >> 30);
	*buf++ = 0x80 | ((code >> 24) & 0x3F);
	*buf++ = 0x80 | ((code >> 18) & 0x3F);
	*buf++ = 0x80 | ((code >> 12) & 0x3F);
	*buf++ = 0x80 | ((code >> 6) & 0x3F);
	*buf++ = 0x80 | (0x3F & code);
    }
    return buf - start;
}

static ojStatus
byteError(ojErr err, const char *map, int off, byte b) {
    err->col = off - err->col;
    switch (map[256]) {
    case 'N': // null_map
	oj_err_set(err, OJ_ERR_PARSE, "expected null");
	break;
    case 'T': // true_map
	oj_err_set(err, OJ_ERR_PARSE, "expected true");
	break;
    case 'F': // false_map
	oj_err_set(err, OJ_ERR_PARSE, "expected false");
	break;
    case 's': // string_map
	oj_err_set(err, OJ_ERR_PARSE, "invalid JSON character 0x%02x", b);
	break;

/*
	case trueMap:
		err.Message = "expected true"
	case falseMap:
		err.Message = "expected false"
	case afterMap:
		err.Message = fmt.Sprintf("expected a comma or close, not '%c'", b)
	case key1Map:
		err.Message = fmt.Sprintf("expected a string start or object close, not '%c'", b)
	case keyMap:
		err.Message = fmt.Sprintf("expected a string start, not '%c'", b)
	case colonMap:
		err.Message = fmt.Sprintf("expected a colon, not '%c'", b)
	case negMap, zeroMap, digitMap, dotMap, fracMap, expSignMap, expZeroMap, expMap:
		err.Message = "invalid number"
	case stringMap:
		err.Message = fmt.Sprintf("invalid JSON character 0x%02x", b)
	case escMap:
		err.Message = fmt.Sprintf("invalid JSON escape character '\\%c'", b)
	case uMap:
		err.Message = fmt.Sprintf("invalid JSON unicode character '%c'", b)
	case spaceMap:
		err.Message = fmt.Sprintf("extra characters after close, '%c'", b)
	default:
		err.Message = fmt.Sprintf("unexpected character '%c'", b)
	}
*/
    default:
	oj_err_set(err, OJ_ERR_PARSE, "unexpected character '%c' in '%c' mode", b, map[256]);
	break;
    }
    return err->code;
}

static ojStatus
parse(ojParser p, const byte *json) {
    const byte *start;
/*
    if (NULL == p->next_map) {
	printf("*** parse with map: %c\n", p->map[256]);
    } else {
	printf("*** parse with map: %c and next: %c\n", p->map[256], p->next_map[256]);
    }
*/
    //printf("*** %s\n", json);

    for (const byte *b = json; '\0' != *b; b++) {
	//printf("*** op: %c  b: %c from %c\n", p->map[*b], *b, p->map[256]);
	switch (p->map[*b]) {
	case SKIP_NEWLINE:
	    p->err.line++;
	    p->err.col = b - json;
	    b++;
#ifdef SPACE_JUMP
	    //for (uint32_t *sj = (uint32_t*)b; 0x20202020 == *sj; sj++) { b += 4; }
	    for (uint16_t *sj = (uint16_t*)b; 0x2020 == *sj; sj++) { b += 2; }
#endif
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
	    b--;
	    break;
	case COLON_COLON:
	    p->map = value_map;
	    break;
	case SKIP_CHAR:
	    break;
	case KEY_QUOTE:
	    b++;
	    start = b;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    if ('"' == *b) {
		_oj_val_set_key(p, (char*)start, b - start);
		p->map = colon_map;
		break;
	    }
	    _oj_val_set_key(p, (char*)start, b - start);
	    b--;
	    p->map = string_map;
	    p->next_map = colon_map;
	    break;
	case AFTER_COMMA:
	    if (0 < p->depth && '{' == p->stack[p->depth-1]) {
		p->map = key_map;
	    } else {
		p->map = comma_map;
	    }
	    break;
	case VAL_QUOTE:
	    b++;
	    start = b;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    p->val.type = OJ_STRING;
	    if ('"' == *b) {
		_oj_val_set_str(p, (char*)start, b - start);
		p->push(&p->val, p->ctx);
		p->map = (0 == p->depth) ? value_map : after_map;
		break;
	    }
	    _oj_val_set_str(p, (char*)start, b - start);
	    b--;
	    p->map = string_map;
	    p->next_map = (0 == p->depth) ? value_map : after_map;
	    break;
	case OPEN_OBJECT:
	    if (sizeof(p->stack) <= p->depth) {
		// TBD realloc stack
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "too deeply nested");
	    }
	    p->stack[p->depth] = '{';
	    p->depth++;
	    p->val.type = OJ_OBJECT;
	    p->val.mod = OJ_OBJ_RAW;
	    p->val.list.head = NULL;
	    p->push(&p->val, p->ctx);
	    p->map = key1_map;
	    break;
	case NUM_CLOSE_OBJECT:
	    if (OJ_INT == p->val.type || OJ_DECIMAL == p->val.type) {
		p->val.num.raw[p->val.num.len] = '\0'; // TBD check length
		p->push(&p->val, p->ctx);
		p->val.type = OJ_NULL;
		p->val.key.len = 0;
	    }
	    // flow through
	case CLOSE_OBJECT:
	    p->depth--;
	    p->map = (0 == p->depth) ? value_map : after_map;
	    p->pop(p->ctx);
	    if (p->depth < 0 || '{' != p->stack[p->depth]) {
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "unexpected object close");
	    }
	    break;
	case OPEN_ARRAY:
	    if (sizeof(p->stack) <= p->depth) {
		// TBD realloc stack
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "too deeply nested");
	    }
	    p->stack[p->depth] = '[';
	    p->depth++;
	    p->val.type = OJ_ARRAY;
	    p->val.list.head = NULL;
	    p->push(&p->val, p->ctx);
	    p->map = value_map;
	    break;
	case NUM_CLOSE_ARRAY:
	    if (OJ_INT == p->val.type || OJ_DECIMAL == p->val.type) {
		p->val.num.raw[p->val.num.len] = '\0'; // TBD check length
		p->push(&p->val, p->ctx);
		p->val.type = OJ_NULL;
		p->val.key.len = 0;
	    }
	    // flow through
	case CLOSE_ARRAY:
	    p->depth--;
	    p->map = (0 == p->depth) ? value_map : after_map;
	    p->pop(p->ctx);
	    if (p->depth < 0 || '[' != p->stack[p->depth]) {
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "unexpected array close");
	    }
	    break;
	case NUM_COMMA:
	    p->val.num.raw[p->val.num.len] = '\0'; // TBD check length
	    p->push(&p->val, p->ctx);
	    p->val.type = OJ_NULL;
	    p->val.key.len = 0;
	    if (0 < p->depth && '{' == p->stack[p->depth-1]) {
		p->map = key_map;
	    } else {
		p->map = comma_map;
	    }
	    break;
	case VAL0:
	    p->val.type = OJ_INT;
	    p->val.num.native = false;
	    p->val.num.len = 1;
	    *p->val.num.raw = *b;
	    p->map = zero_map;
	    break;
	case VAL_NEG:
	    p->val.type = OJ_INT;
	    p->val.num.native = false;
	    p->val.num.len = 1;
	    *p->val.num.raw = *b;
	    p->map = neg_map;
	    break;;
	case VAL_DIGIT:
	    p->val.type = OJ_INT;
	    p->val.num.native = false;
	    p->map = digit_map;
	    p->val.num.len = 0;
	    start = b;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    _oj_append_num(p, (char*)start, b - start);
	    b--;
	    break;
	case NUM_DIGIT:
	    start = b;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    _oj_append_num(p, (char*)start, b - start);
	    b--;
	    break;
	case NUM_DOT:
	    p->val.type = OJ_DECIMAL;
	    _oj_append_num(p, (const char*)b, 1);
	    p->map = dot_map;
	    break;
	case NUM_FRAC:
	    p->map = frac_map;
	    start = b;
	    for (; NUM_FRAC == frac_map[*b]; b++) {
	    }
	    _oj_append_num(p, (char*)start, b - start);
	    b--;
	    break;
	case FRAC_E:
	    p->val.type = OJ_DECIMAL;
	    _oj_append_num(p, (const char*)b, 1);
	    p->map = exp_sign_map;
	    break;
	case NUM_ZERO:
	    p->val.num.raw[p->val.num.len] = *b;
	    p->val.num.len++;
	    p->map = zero_map;
	    break;
	case NEG_DIGIT:
	    p->val.num.raw[p->val.num.len] = *b;
	    p->val.num.len++;
	    p->map = digit_map;
	    break;
	case EXP_SIGN:
	    _oj_append_num(p, (const char*)b, 1);
	    p->map = exp_zero_map;
	    break;
	case EXP_DIGIT:
	    start = b;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    _oj_append_num(p, (char*)start, b - start);
	    b--;
	    p->map = exp_map;
	    break;
	case NUM_SPC:
	    p->map = (0 == p->depth) ? value_map : after_map;
	    p->val.num.raw[p->val.num.len] = '\0'; // TBD check length
	    p->push(&p->val, p->ctx);
	    p->val.type = OJ_NULL;
	    p->val.key.len = 0;
	    break;
	case NUM_NEWLINE:
	    p->map = (0 == p->depth) ? value_map : after_map;
	    p->val.num.raw[p->val.num.len] = '\0'; // TBD check length
	    p->push(&p->val, p->ctx);
	    p->val.type = OJ_NULL;
	    p->val.key.len = 0;
	    p->err.line++;
	    p->err.col = b - json;
	    b++;
#ifdef SPACE_JUMP
	    //for (uint32_t *sj = (uint32_t*)b; 0x20202020 == *sj; sj++) { b += 4; }
	    for (uint16_t *sj = (uint16_t*)b; 0x2020 == *sj; sj++) { b += 2; }
#endif
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
	    b--;
	    break;
	case STR_OK:
	    start = b;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    if (':' == p->next_map[256]) {
		_oj_append_str(p, &p->val.key, start, b - start);
	    } else {
		_oj_append_str(p, &p->val.str, start, b - start);
	    }
	    if ('"' == *b) {
		p->map = p->next_map;
		if (':' != p->map[256]) {
		    p->push(&p->val, p->ctx);
		}
		break;
	    }
	    b--;
	    break;
	case STR_SLASH:
	    p->map = esc_map;
	    break;
	case STR_QUOTE:
	    p->map = p->next_map;
	    if (':' != p->map[256]) {
		p->push(&p->val, p->ctx);
	    }
	    break;
	case ESC_U:
	    p->map = u_map;
	    p->ri = 0;
	    p->ucode = 0;
	    break;
	case U_OK:
	    p->ri++;
	    p->ucode = p->ucode << 4 | (uint32_t)hex_map[*b];
	    if (4 <= p->ri) {
		byte	utf8[8];
		size_t	ulen = unicodeToUtf8(p->ucode, utf8);

		if (0 < ulen) {
		    if (':' == p->next_map[256]) {
			_oj_append_str(p, &p->val.key, utf8, ulen);
		    } else {
			_oj_append_str(p, &p->val.str, utf8, ulen);
		    }
		} else {
		    return oj_err_set(&p->err, OJ_ERR_PARSE, "invalid unicode");
		}
		p->map = string_map;
	    }
	    break;
	case ESC_OK:
	    if (':' == p->next_map[256]) {
		_oj_append_str(p, &p->val.key, (byte*)&esc_byte_map[*b], 1);
	    } else {
		_oj_append_str(p, &p->val.str, (byte*)&esc_byte_map[*b], 1);
	    }
	    p->map = string_map;
	    break;
	case VAL_NULL:
	    if ('u' == b[1] && 'l' == b[2] && 'l' == b[3]) {
		b += 3;
		p->map = (0 == p->depth) ? value_map : after_map;
		p->val.type = OJ_NULL;
		p->val.key.len = 0;
		p->push(&p->val, p->ctx);
		break;
	    }
	    if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		p->map = null_map;
		// TBD copy what we have and update p->ri
		p->ri = 0;
		break;
	    }
	    p->err.col = b - json - p->err.col;
	    return oj_err_set(&p->err, OJ_ERR_PARSE, "expected null");
	case VAL_TRUE:
	    if ('r' == b[1] && 'u' == b[2] && 'e' == b[3]) {
		b += 3;
		p->map = (0 == p->depth) ? value_map : after_map;
		p->val.type = OJ_TRUE;
		p->push(&p->val, p->ctx);
		break;
	    }
	    if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		p->map = true_map;
		// TBD copy what we have and update p->ri
		p->ri = 0;
		break;
	    }
	    p->err.col = b - json - p->err.col;
	    return oj_err_set(&p->err, OJ_ERR_PARSE, "expected true");
	case VAL_FALSE:
	    if ('a' == b[1] && 'l' == b[2] && 's' == b[3] && 'e' == b[4]) {
		b += 4;
		p->map = (0 == p->depth) ? value_map : after_map;
		p->val.type = OJ_FALSE;
		p->push(&p->val, p->ctx);
		break;
	    }
	    if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3] || '\0' == b[4]) {
		p->map = false_map;
		// TBD copy what we have and update p->ri
		p->ri = 0;
		break;
	    }
	    p->err.col = b - json - p->err.col;
	    return oj_err_set(&p->err, OJ_ERR_PARSE, "expected false");
	case CHAR_ERR:
	    if (OJ_OK == p->err.code) {
		byteError(&p->err, p->map, b - json, *b);
	    }
	    return p->err.code;
	default:
	    printf("*** internal error, unknown mode '%c'\n", p->map[*b]);
	    break;
	}
    }
    switch (p->map[256]) {
    case '0':
    case 'd':
    case 'f':
    case 'z':
    case 'X':
	p->push(&p->val, p->ctx);
	break;
    }
    return p->err.code;
}

static ojStatus
validate(ojValidator v, const byte *json) {
    for (const byte *b = json; '\0' != *b; b++) {
	//printf("*** op: %c  b: %c from %c\n", v->map[*b], *b, v->map[256]);
	switch (v->map[*b]) {
	case SKIP_NEWLINE:
	    v->err.line++;
	    v->err.col = b - json;
	    b++;
#ifdef SPACE_JUMP
	    //for (uint32_t *sj = (uint32_t*)b; 0x20202020 == *sj; sj++) { b += 4; }
	    for (uint16_t *sj = (uint16_t*)b; 0x2020 == *sj; sj++) { b += 2; }
#endif
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
	    b--;
	    break;
	case COLON_COLON:
	    v->map = value_map;
	    break;
	case SKIP_CHAR:
	    break;
	case KEY_QUOTE:
	    b++;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    b--;
	    v->map = string_map;
	    v->next_map = colon_map;
	    break;
	case AFTER_COMMA:
	    if (0 < v->depth && '{' == v->stack[v->depth-1]) {
		v->map = key_map;
	    } else {
		v->map = comma_map;
	    }
	    break;
	case VAL_QUOTE:
	    b++;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    switch (*b) {
	    case '"': // normal termination
		v->map = (0 == v->depth) ? value_map : after_map;
		break;
	    case '\0':
		v->map = string_map;
		v->next_map = (0 == v->depth) ? value_map : after_map;
	    case '\\':
		v->map = esc_map;
		v->next_map = (0 == v->depth) ? value_map : after_map;
		break;
	    default:
		v->err.col = b - json - v->err.col;
		return oj_err_set(&v->err, OJ_ERR_PARSE, "invalid JSON character 0x%02x", *b);
	    }
	    break;
	case OPEN_OBJECT:
	    // TBD check depth vs stack len
	    v->stack[v->depth] = '{';
	    v->depth++;
	    v->map = key1_map;
	    break;
	case NUM_CLOSE_OBJECT:
	case CLOSE_OBJECT:
	    v->depth--;
	    v->map = (0 == v->depth) ? value_map : after_map;
	    if (v->depth < 0 || '{' != v->stack[v->depth]) {
		v->err.col = b - json - v->err.col;
		return oj_err_set(&v->err, OJ_ERR_PARSE, "unexpected object close");
	    }
	    break;
	case OPEN_ARRAY:
	    // TBD check depth vs stack len
	    v->stack[v->depth] = '[';
	    v->depth++;
	    v->map = value_map;
	    break;
	case NUM_CLOSE_ARRAY:
	case CLOSE_ARRAY:
	    v->depth--;
	    v->map = (0 == v->depth) ? value_map : after_map;
	    if (v->depth < 0 || '[' != v->stack[v->depth]) {
		v->err.col = b - json - v->err.col;
		return oj_err_set(&v->err, OJ_ERR_PARSE, "unexpected array close");
	    }
	    break;
	case NUM_COMMA:
	    if (0 < v->depth && '{' == v->stack[v->depth-1]) {
		v->map = key_map;
	    } else {
		v->map = comma_map;
	    }
	    break;
	case VAL0:
	    v->map = zero_map;
	    break;
	case VAL_NEG:
	    v->map = neg_map;
	    break;;
	case VAL_DIGIT:
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    b--;
	    v->map = digit_map;
	    break;
	case NUM_DIGIT:
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    b--;
	    break;
	case NUM_DOT:
	    v->map = dot_map;
	    break;
	case NUM_FRAC:
	    v->map = frac_map;
	    for (; NUM_FRAC == frac_map[*b]; b++) {
	    }
	    b--;
	    break;
	case FRAC_E:
	    v->map = exp_sign_map;
	    break;
	case NUM_ZERO:
	    v->map = zero_map;
	    break;
	case NEG_DIGIT:
	    v->map = digit_map;
	    break;
	case EXP_SIGN:
	    v->map = exp_zero_map;
	    break;
	case EXP_DIGIT:
	    v->map = exp_map;
	    break;
	case NUM_SPC:
	    v->map = (0 == v->depth) ? value_map : after_map;
	    break;
	case NUM_NEWLINE:
	    v->map = (0 == v->depth) ? value_map : after_map;
	    v->err.line++;
	    v->err.col = b - json;
	    b++;
#ifdef SPACE_JUMP
	    //for (uint32_t *sj = (uint32_t*)b; 0x20202020 == *sj; sj++) { b += 4; }
	    for (uint16_t *sj = (uint16_t*)b; 0x2020 == *sj; sj++) { b += 2; }
#endif
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
	    b--;
	    break;
	case STR_OK:
	    break;
	case STR_SLASH:
	    v->map = esc_map;
	    break;
	case STR_QUOTE:
	    v->map = v->next_map;
	    break;
	case ESC_U:
	    v->map = u_map;
	    v->ri = 0;
	    break;
	case U_OK:
	    v->ri++;
	    if (4 <= v->ri) {
		v->map = string_map;
	    }
	    break;
	case ESC_OK:
	    v->map = string_map;
	    break;
	case VAL_NULL:
	    //if (*(uint32_t*)b == *(uint32_t*)"null") {
	    if ('u' == b[1] && 'l' == b[2] && 'l' == b[3]) {
		b += 3;
		v->map = (0 == v->depth) ? value_map : after_map;
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		v->map = null_map;
		v->ri = 0;
	    } else {
		v->err.col = b - json - v->err.col;
		return oj_err_set(&v->err, OJ_ERR_PARSE, "expected null");
	    }
	    break;
	case VAL_TRUE:
	    //if (*(uint32_t*)b == *(uint32_t*)"true") {
	    if ('r' == b[1] && 'u' == b[2] && 'e' == b[3]) {
		b += 3;
		v->map = (0 == v->depth) ? value_map : after_map;
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		v->map = true_map;
		v->ri = 0;
	    } else {
		v->err.col = b - json - v->err.col;
		return oj_err_set(&v->err, OJ_ERR_PARSE, "expected true");
	    }
	    break;
	case VAL_FALSE:
	    //if (*(uint32_t*)b == *(uint32_t*)"fals" && 'e' == b[4]) {
	    if ('a' == b[1] && 'l' == b[2] && 's' == b[3] && 'e' == b[4]) {
		b += 4;
		v->map = (0 == v->depth) ? value_map : after_map;
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3] || '\0' == b[4]) {
		v->map = false_map;
		v->ri = 0;
	    } else {
		v->err.col = b - json - v->err.col;
		return oj_err_set(&v->err, OJ_ERR_PARSE, "expected false");
	    }
	    break;
	case CHAR_ERR:
	    return byteError(&v->err, v->map, b - json, *b);
	default:
	    printf("*** internal error, unknown mode '%c'\n", v->map[*b]);
	    v->err.col = b - json - v->err.col;
	    return oj_err_set(&v->err, OJ_ERR_PARSE, "internal error, unknown mode");
	    break;
	}
    }
    return OJ_OK;
}

ojStatus
oj_validate_str(ojErr err, const char *json) {
    struct _ojValidator	v;

    memset(&v, 0, sizeof(v));
    v.err.line = 1;
    v.map = value_map;
    if (OJ_OK != validate(&v, (const byte*)json) && NULL != err) {
	*err = v.err;
    }
    return v.err.code;
}

ojStatus
oj_validate_strzzz(ojValidator v, const char *json) {
    memset(v, 0, sizeof(struct _ojValidator));
    v->map = value_map;
    v->err.line = 1;
    validate(v, (const byte*)json);

    return v->err.code;
}

void
oj_parser_reset(ojParser p) {
    p->map = value_map;
    p->err.line = 1;
    p->err.col = 0;
    *p->err.msg = '\0';
    p->depth = 0;
}

static void
no_push(ojVal val, void *ctx) {
}

static void
no_pop(void *ctx) {
}

ojStatus
oj_parse_str(ojParser p, const char *json) {
    if (NULL == p->push) {
	p->push = no_push;
    }
    if (NULL == p->pop) {
	p->pop = no_pop;
    }
    p->err.line = 1;
    p->map = value_map;
    parse(p, (const byte*)json);

    return p->err.code;
}

ojStatus
oj_parse_strp(ojParser p, const char **json) {
    // TBD
    return OJ_OK;
}

static void
read_block(int fd, ReadBlock b) {
    size_t	rcnt = read(fd, b->buf, sizeof(b->buf) - 1);

    if (0 < rcnt) {
	b->buf[rcnt] = '\0';
	//printf("*** %s\n", (char*)b->buf);
    }
    if (rcnt != sizeof(b->buf) - 1) {
	if (0 == rcnt) {
	    b->eof = true;
	} else if (rcnt < 0) {
	    b->status = errno;
	}
    }
}

static void*
reader(void *ctx) {
    ReadCtx	rc = (ReadCtx)ctx;
    ReadBlock	b = rc->blocks + 1;
    ReadBlock	next;

    pthread_mutex_lock(&b->mu);
    while (true) {
	if (OJ_OK != b->status) {
	    break;
	}
	read_block(rc->fd, b);
	if (OJ_OK != b->status || b->eof) {
	    break;
	}
	next = b + 1;
	if (rc->bend <= next) {
	    next = rc->blocks;
	}
	pthread_mutex_lock(&next->mu);
	pthread_mutex_unlock(&b->mu);
	b = next;
    }
    pthread_mutex_unlock(&b->mu);
    pthread_exit(0);

    return NULL;
}

static ojStatus
parse_large(ojParser p, int fd) {
    struct _ReadCtx	rc;
    ReadBlock		b; // current block being parsed

    //printf("*** large - %ld\n", sizeof(*rc.blocks));
    memset(&rc, 0, sizeof(rc));
    rc.bend = rc.blocks + sizeof(rc.blocks) / sizeof(*rc.blocks);
    rc.fd = fd;

    for (b = rc.blocks; b < rc.bend; b++) {
	if (0 != pthread_mutex_init(&b->mu, NULL)) {
	    return oj_err_no(&p->err, "initializing mutex on concurrent file reader");
	}
    }
    // Lock the first 2 blocks, the first to read into and the second to stop
    // the new thread of reading into the second block which should not happen
    // until the reading of the first completes.
    pthread_mutex_lock(&rc.blocks->mu);
    pthread_mutex_lock(&rc.blocks[1].mu);

    pthread_t	t;
    int		err;

    if (0 != (err = pthread_create(&t, NULL, reader, (void*)&rc))) {
	return oj_err_set(&p->err, err, "failed to create reader thread");
    }
    pthread_detach(t);

    ReadBlock	next;

    b = rc.blocks;
    read_block(fd, b);

    // Release the second block so the reader can start reading additional
    // blocks.
    pthread_mutex_unlock(&rc.blocks[1].mu);
    while (true) {
	if (OJ_OK != b->status) {
	    oj_err_set(&p->err, b->status, "read failed");
	    break;
	}
	if (OJ_OK != parse(p, b->buf) || b->eof) {
	    b->status = p->err.code;
	    break;
	}
	// Lock next before unlocking the current to assure the reader doesn't
	// overtake the parser.
	next = b + 1;
	if (rc.bend <= next) {
	    next = rc.blocks;
	}
	pthread_mutex_lock(&next->mu);
	pthread_mutex_unlock(&b->mu);
	b = next;
    }
    pthread_mutex_unlock(&b->mu);

    return p->err.code;
}

ojStatus
oj_parse_fd(ojParser p, int fd) {
    if (NULL == p->push) {
	p->push = no_push;
    }
    if (NULL == p->pop) {
	p->pop = no_pop;
    }
    p->err.line = 1;
    p->map = value_map;

    struct stat	info;

    if (0 == fstat(fd, &info) && USE_THREAD_LIMIT < info.st_size && false) {
	// Use threaded version.
	return parse_large(p, fd);
    }
    byte	buf[16385];
    size_t	size = sizeof(buf) - 1;
    size_t	rsize;

    while (true) {
	if (0 < (rsize = read(fd, buf, size))) {
	    buf[rsize] = '\0';
	    if (OJ_OK != parse(p, buf)) {
		break;
	    }
	}
	if (rsize <= 0) {
	    if (0 != rsize) {
		return oj_err_no(&p->err, "read error");
	    }
	    break;
	}
    }
    return p->err.code;
}

ojStatus
oj_parse_file(ojParser p, const char *filename) {
    int	fd = open(filename, O_RDONLY);

    if (fd < 0) {
	return oj_err_no(&p->err, "error opening %s", filename);
    }
    oj_parse_fd(p, fd);
    close(fd);

    return p->err.code;
}

ojStatus
oj_parse_reader(ojParser p, void *src, ojReadFunc rf) {
    // TBD
    return OJ_OK;
}

ojStatus
oj_parse_file_follow(ojParser p, FILE *file) {
    // TBD
    return OJ_OK;
}
