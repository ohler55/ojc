// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <string.h>

#include "oj.h"
#include "intern.h"

typedef uint8_t	byte;

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
.............................m..\
.............................n..\
................................\
................................\
................................\
................................0";

static const char	digit_map[257] = "\
.........rs..r..................\
r...........u.t.NNNNNNNNNN......\
.............................m..\
.............................n..\
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
.....w.......................m..\
.....w.......................n..\
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
.............................m..\
.............................n..\
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
	oj_err_set(err, OJ_ERR_PARSE, "unexpected character '%c'", b);
	break;
    }
    return err->code;
}

static ojStatus
parse(ojParser p, const byte *json) {
    int		len;
    const byte *start;

    for (const byte *b = json; '\0' != *b; b++) {
	//printf("*** op: %c  b: %c from %c\n", p->map[*b], *b, p->map[256]);
	switch (p->map[*b]) {
	case SKIP_NEWLINE:
	    p->err.line++;
	    p->err.col = b - json;
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
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
		len = b - start;
		if (len < sizeof(p->val.key.val)) {
		    *p->val.key.val = (char)len;
		    memcpy(p->val.key.val + 1, start, len);
		} else {
		    // TBD build key
		}
	    }
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
	    if ('"' == *b) {
		len = b - start;
		if (len < sizeof(p->val.str.val)) {
		    *p->val.str.val = (char)len;
		    memcpy(p->val.str.val + 1, start, len);
		    p->val.type = OJ_STRING;
		    p->val.mod = OJ_STR_INLINE;
		} else {
		    // TBD build string
		    printf("*** long string\n");
		}
	    }
	    b--;
	    p->map = string_map;
	    p->next_map = after_map;
	    break;
	case OPEN_OBJECT:
	    // TBD check depth vs stack len
	    p->stack[p->depth] = '{';
	    p->depth++;
	    if (OJ_INT == p->val.type || OJ_DECIMAL == p->val.type) {
		p->push(&p->val, p->pp_ctx);
		p->val.type = OJ_NULL;
	    }
	    p->val.type = OJ_OBJECT;
	    p->val.mod = OJ_OBJ_RAW;
	    p->val.list.head = NULL;
	    p->push(&p->val, p->pp_ctx);
	    p->map = key1_map;
	    break;
	case CLOSE_OBJECT:
	    p->map = after_map;
	    p->depth--;
	    if (OJ_INT == p->val.type || OJ_DECIMAL == p->val.type) {
		p->push(&p->val, p->pp_ctx);
		p->val.type = OJ_NULL;
	    }
	    p->pop(p->pp_ctx);
	    if (p->depth < 0 || '{' != p->stack[p->depth]) {
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "unexpected object close");
	    }
	    break;
	case OPEN_ARRAY:
	    // TBD check depth vs stack len
	    p->stack[p->depth] = '[';
	    p->depth++;
	    if (OJ_INT == p->val.type || OJ_DECIMAL == p->val.type) {
		p->push(&p->val, p->pp_ctx);
		p->val.type = OJ_NULL;
	    }
	    p->val.type = OJ_ARRAY;
	    p->val.list.head = NULL;
	    p->push(&p->val, p->pp_ctx);
	    p->map = value_map;
	    break;
	case CLOSE_ARRAY:
	    p->map = after_map;
	    p->depth--;
	    if (OJ_INT == p->val.type || OJ_DECIMAL == p->val.type) {
		p->push(&p->val, p->pp_ctx);
		p->val.type = OJ_NULL;
	    }
	    p->pop(p->pp_ctx);
	    if (p->depth < 0 || '[' != p->stack[p->depth]) {
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "unexpected array close");
	    }
	    break;
	case NUM_COMMA:
	    p->push(&p->val, p->pp_ctx);
	    p->val.type = OJ_NULL;
	    if (0 < p->depth && '{' == p->stack[p->depth-1]) {
		p->map = key_map;
	    } else {
		p->map = comma_map;
	    }
	    break;
	case VAL0:
	    p->val.type = OJ_INT;
	    p->val.mod = OJ_INT_RAW;
	    *p->val.str.val = 1;
	    p->val.str.val[*(byte*)p->val.str.val] = *b;
	    p->map = zero_map;
	    break;
	case VAL_NEG:
	    p->val.type = OJ_INT;
	    p->val.mod = OJ_INT_RAW;
	    *p->val.str.val = 1;
	    p->val.str.val[*(byte*)p->val.str.val] = *b;
	    p->map = neg_map;
	    break;;
	case VAL_DIGIT:
	    p->val.type = OJ_INT;
	    p->val.mod = OJ_INT_RAW;
	    p->map = digit_map;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
		*p->val.str.val = *p->val.str.val + 1;
		p->val.str.val[*(byte*)p->val.str.val] = *b;
	    }
	    b--;
	    break;
	case NUM_DIGIT:
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
		*p->val.str.val = *p->val.str.val + 1;
		p->val.str.val[*(byte*)p->val.str.val] = *b;
	    }
	    b--;
	    break;
	case NUM_DOT:
	    p->val.type = OJ_DECIMAL;
	    p->val.mod = OJ_DEC_RAW;
	    *p->val.str.val = *p->val.str.val + 1;
	    p->val.str.val[*(byte*)p->val.str.val] = *b;
	    p->map = dot_map;
	    break;
	case NUM_FRAC:
	    p->map = frac_map;
	    for (; NUM_FRAC == frac_map[*b]; b++) {
		*p->val.str.val = *p->val.str.val + 1;
		p->val.str.val[*(byte*)p->val.str.val] = *b;
	    }
	    b--;
	    break;
	case FRAC_E:
	    p->val.type = OJ_DECIMAL;
	    p->val.mod = OJ_DEC_RAW;
	    *p->val.str.val = *p->val.str.val + 1;
	    p->val.str.val[*(byte*)p->val.str.val] = *b;
	    p->map = exp_sign_map;
	    break;
	case NUM_ZERO:
	    *p->val.str.val = *p->val.str.val + 1;
	    p->val.str.val[*(byte*)p->val.str.val] = *b;
	    p->map = zero_map;
	    break;
	case NEG_DIGIT:
	    *p->val.str.val = *p->val.str.val + 1;
	    p->val.str.val[*(byte*)p->val.str.val] = *b;
	    p->map = digit_map;
	    break;
	case EXP_SIGN:
	    *p->val.str.val = *p->val.str.val + 1;
	    p->val.str.val[*(byte*)p->val.str.val] = *b;
	    p->map = exp_zero_map;
	    break;
	case EXP_DIGIT:
	    *p->val.str.val = *p->val.str.val + 1;
	    p->val.str.val[*(byte*)p->val.str.val] = *b;
	    p->map = exp_map;
	    break;
	case NUM_SPC:
	    p->map = after_map;
	    p->push(&p->val, p->pp_ctx);
	    p->val.type = OJ_NULL;
	    break;
	case NUM_NEWLINE:
	    p->map = after_map;
	    p->push(&p->val, p->pp_ctx);
	    p->val.type = OJ_NULL;
	    p->err.line++;
	    p->err.col = b - json;
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
	    break;
	case STR_OK:
	    break;
	case STR_SLASH:
	    p->map = esc_map;
	    break;
	case STR_QUOTE:
	    p->map = p->next_map;
	    if (':' != p->map[256]) {
		p->push(&p->val, p->pp_ctx);
	    }
	    break;
	case ESC_U:
	    p->map = u_map;
	    p->ri = 0;
	    break;
	case U_OK:
	    p->ri++;
	    if (4 <= p->ri) {
		p->map = string_map;
		// TBD convert to UTF-8 and add
	    }
	    break;
	case ESC_OK:
	    *p->val.str.val = *p->val.str.val + 1;
	    p->val.str.val[*(byte*)p->val.str.val] = esc_byte_map[*b];
	    p->map = string_map;
	    break;
	case VAL_NULL:
	    if ('u' == b[1] && 'l' == b[2] && 'l' == b[3]) {
		b += 3;
		p->map = after_map;
		p->val.type = OJ_NULL;
		p->push(&p->val, p->pp_ctx);
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		p->map = null_map;
		p->ri = 0;
	    } else {
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "expected null");
	    }
	    break;
	case VAL_TRUE:
	    if ('r' == b[1] && 'u' == b[2] && 'e' == b[3]) {
		b += 3;
		p->map = after_map;
		p->val.type = OJ_TRUE;
		p->push(&p->val, p->pp_ctx);
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		p->map = true_map;
		p->ri = 0;
	    } else {
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "expected true");
	    }
	    break;
	case VAL_FALSE:
	    if ('a' == b[1] && 'l' == b[2] && 's' == b[3] && 'e' == b[4]) {
		b += 4;
		p->map = after_map;
		p->val.type = OJ_FALSE;
		p->push(&p->val, p->pp_ctx);
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3] || '\0' == b[4]) {
		p->map = false_map;
		p->ri = 0;
	    } else {
		p->err.col = b - json - p->err.col;
		return oj_err_set(&p->err, OJ_ERR_PARSE, "expected false");
	    }
	    break;
	case CHAR_ERR:
	    return byteError(&p->err, p->map, b - json, *b);
	default:
	    printf("*** internal error, unknown mode '%c'\n", p->map[*b]);
	    break;
	}
    }
    return OJ_OK;
}

static ojStatus
validate(ojValidator v, const byte *json) {
    for (const byte *b = json; '\0' != *b; b++) {
	//printf("*** op: %c  b: %c from %c\n", v->map[*b], *b, v->map[256]);
	switch (v->map[*b]) {
	case SKIP_NEWLINE:
	    v->err.line++;
	    v->err.col = b - json;
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
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
		v->map = after_map;
		break;
	    case '\0':
		v->map = string_map;
		v->next_map = after_map;
	    case '\\':
		v->map = esc_map;
		v->next_map = after_map;
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
	case CLOSE_OBJECT:
	    v->map = after_map;
	    v->depth--;
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
	case CLOSE_ARRAY:
	    v->map = after_map;
	    v->depth--;
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
	    v->map = after_map;
	    break;
	case NUM_NEWLINE:
	    v->map = after_map;
	    v->err.line++;
	    v->err.col = b - json;
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
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
		v->map = after_map;
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
		v->map = after_map;
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
		v->map = after_map;
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
    //printf("*** push %s\n", oj_type_str(val->type));
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

ojStatus
oj_parse_file(ojParser p, FILE *file) {
    // TBD
    return OJ_OK;
}

ojStatus
oj_parse_fd(ojParser p, int socket) {
    // TBD
    return OJ_OK;
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
