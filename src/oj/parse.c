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
};

/*
0123456789abcdef0123456789abcdef */
static const char	value_map[257] = "\
.........ab..a..................\
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
.........ab..a..................\
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
a.p.......................q.....\
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
................................n";

static const char	digit_map[257] = "\
.........rs..r..................\
r...........u.t.NNNNNNNNNN......\
.............................m..\
.............................n..\
................................\
................................\
................................\
................................n";

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
................................n";

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
................................0";

static const char	exp_map[257] = "\
.........rs..r..................\
r...........u...yyyyyyyyyy......\
.............................m..\
.............................n..\
................................\
................................\
................................\
................................n";

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
................................s";

static ojStatus
byteError(ojParser p, int off, byte b) {
    switch (p->map[256]) {
    case 'N': // null_map
	oj_err_set(&p->err, OJ_ERR_PARSE, "expected null");
	break;
    case 'T': // true_map
	oj_err_set(&p->err, OJ_ERR_PARSE, "expected true");
	break;
    case 'F': // false_map
	oj_err_set(&p->err, OJ_ERR_PARSE, "expected false");
	break;
    case 's': // string_map
	oj_err_set(&p->err, OJ_ERR_PARSE, "invalid JSON character 0x%02x", b);
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
	oj_err_set(&p->err, OJ_ERR_PARSE, "unexpected character '%c'", b);
	break;
    }
    return p->err.code;
}

static ojStatus
parse(ojParser p, const byte *json) {

    // TBD put starts stack on parser

    for (const byte *b = json; '\0' != *b; b++) {
	//printf("*** op: %c  b: %c\n", p->map[*b], *b);
	switch (p->map[*b]) {
	case SKIP_NEWLINE:
	    p->err.line++;
	    p->err.col = b - json;
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
	    continue;
	case SKIP_CHAR:
	    continue;
	case VAL_QUOTE: {
	    b++;
	    if ('\0' == *b) {
		// TBD end of buf, build string the hard way
	    }
	    const byte	*start = b;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    switch (*b) {
	    case '"': // normal termination
		if (NULL != p->push) {
		    printf("*** start: %ld\n", start - json);
		    // TBD build string val as needed
		}
		break;
	    case '\0':
		// TBD string mode
		// save from start to current if push
	    case '\\':
		// TBD esc mode
		// save from start to current if push
	    default:
		return oj_err_set(&p->err, OJ_ERR_PARSE, "invalid JSON charactere 0x%02x", *b);
	    }
	    break;
	}
	case OPEN_ARRAY:
	    // TBD
	    //p.starts = append(p.starts, -1)
	    p->depth++;
	    p->map = after_map;
	    break;
	case CLOSE_ARRAY:
	    p->map = after_map;
	    p->depth--;
	    /*
	    if (depth < 0 || 0 <= p.starts[depth]) {
		return p.newError(off, "unexpected object close");
	    }
	    */
	    break;
	case OPEN_OBJECT:
	    // TBD
	    //p.starts = append(p.starts, -1)
	    p->depth++;
	    p->map = key1_map;
	    break;
	case CLOSE_OBJECT:
	    p->map = after_map;
	    p->depth--;
	    /*
	    if (depth < 0 || 0 <= p.starts[depth]) {
		return p.newError(off, "unexpected object close");
	    }
	    */
	    break;
	case VAL0:
	    p->map = zero_map;
	    if (NULL != p->push) {
		p->val.type = OJ_INT;
		p->val.mod = OJ_INT_RAW;
		*p->val.str.val = '\1'; // one character, 0
		p->val.str.val[1] = '0';
	    }
	    continue;
	case VAL_NEG:
	    p->map = neg_map;
	    if (NULL != p->push) {
		p->val.type = OJ_INT;
		p->val.mod = OJ_INT_RAW;
		*p->val.str.val = '\1'; // one character, -
		p->val.str.val[1] = '-';
	    }
	    continue;
	case VAL_DIGIT:
	    p->map = digit_map;
	    if (NULL != p->push) {
		p->val.type = OJ_INT;
		p->val.mod = OJ_INT_RAW;
		*p->val.str.val = '\1'; // one character, -
		p->val.str.val[1] = *b;
		// TBD add to val
	    }
	    break;
	case NUM_DIGIT:
	    if (NULL != p->push) {
		// TBD add to val
	    }
	    break;
	case NUM_DOT:
	    p->map = dot_map;
	    if (NULL != p->push) {
		p->val.type = OJ_DECIMAL;
		p->val.mod = OJ_DEC_RAW;
		// TBD add to val
	    }
	    break;
	case NUM_FRAC:
	    p->map = frac_map;
	    if (NULL != p->push) {
		// TBD add to val
	    }
	    break;
	case FRAC_E:
	    p->map = exp_sign_map;
	    if (NULL != p->push) {
		// TBD add to val
	    }
	    break;
	case NUM_ZERO:
	    p->map = zero_map;
	    if (NULL != p->push) {
		// TBD add to val
	    }
	    break;
	case NEG_DIGIT:
	    p->map = digit_map;
	    if (NULL != p->push) {
		// TBD add to val
	    }
	    break;
	case EXP_SIGN:
	    p->map = exp_zero_map;
	    if (NULL != p->push) {
		// TBD add to val
	    }
	    break;
	case EXP_DIGIT:
	    p->map = exp_map;
	    if (NULL != p->push) {
		// TBD add to val
	    }
	    break;
	case NUM_SPC:
	    if (NULL != p->push) {
		// TBD push number
	    }
	    p->map = after_map;
	    break;
	case NUM_NEWLINE:
	    if (NULL != p->push) {
		// TBD push number
	    }
	    p->map = after_map;
	    p->err.line++;
	    p->err.col = b - json;
	    for (; SKIP_CHAR == space_map[*b]; b++) {
	    }
	    continue;
	case VAL_NULL:
	    if ('u' == b[1] && 'l' == b[2] && 'l' == b[3]) {
		b += 3;
		p->map = after_map;
		if (NULL != p->push) {
		    p->val.type = OJ_NULL;
		    p->push(&p->val);
		}
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		p->map = null_map;
		//p.ri = 0;
	    } else {
		return oj_err_set(&p->err, OJ_ERR_PARSE, "expected null");
	    }
	    break;
	case VAL_TRUE:
	    if ('r' == b[1] && 'u' == b[2] && 'e' == b[3]) {
		b += 3;
		p->map = after_map;
		if (NULL != p->push) {
		    p->val.type = OJ_TRUE;
		    p->push(&p->val);
		}
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		p->map = true_map;
		//p.ri = 0;
	    } else {
		return oj_err_set(&p->err, OJ_ERR_PARSE, "expected true");
	    }
	    break;
	case VAL_FALSE:
	    if ('a' == b[1] && 'l' == b[2] && 's' == b[3] && 'e' == b[4]) {
		b += 4;
		p->map = after_map;
		if (NULL != p->push) {
		    p->val.type = OJ_FALSE;
		    p->push(&p->val);
		}
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3] || '\0' == b[4]) {
		p->map = false_map;
		//p.ri = 0;
	    } else {
		return oj_err_set(&p->err, OJ_ERR_PARSE, "expected false");
	    }
	    break;
	case CHAR_ERR:
	    return byteError(p, b - json, *b);
	default:
	    printf("*** internal error, unknown mode '%c'\n", p->map[*b]);
	    break;
	}
    }
    return OJ_OK;
}

void
oj_validator(ojParser p) {
    memset(p, 0, sizeof(struct _ojParser));
    p->map = value_map;
    p->err.line = 1;
}

ojStatus
oj_parse_str(ojParser p, const char *json) {
    p->err.line = 1;
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