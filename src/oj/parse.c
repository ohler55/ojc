// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "oj.h"
#include "intern.h"

#define DEBUG	0

#define USE_THREAD_LIMIT	100000
#define MAX_EXP			4932
// max in the pow_map
#define MAX_POW			400

#define MIN_SLEEP	(1000000000LL / (double)CLOCKS_PER_SEC)

static void
one_beat() {
    struct timespec	ts = { .tv_sec = 0, .tv_nsec = 100 };

    pselect(0, NULL, NULL, NULL, &ts, 0);
}

#if DEBUG
static void
print_stack(ojParser p, const char *label) {
    printf("*** %s\n", label);
    for (ojVal v = p->stack; NULL != v; v = v->next) {
	char	*s = oj_to_str(v, 0);
	printf("  %s\n", s);
	free(s);
    }
}
#endif

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
    BIG_DIGIT		= 'C',
    BIG_DOT		= 'D',
    U_OK		= 'E',
    TOKEN_OK		= 'F',
    NUM_CLOSE_OBJECT	= 'G',
    NUM_CLOSE_ARRAY	= 'H',
    BIG_FRAC		= 'I',
    BIG_E		= 'J',
    BIG_EXP_SIGN	= 'K',
    BIG_EXP		= 'L',
    UTF1		= 'M', // expect 1 more follow byte
    NUM_DIGIT		= 'N',
    NUM_ZERO		= 'O',
    UTF2		= 'P', // expect 2 more follow byte
    UTF3		= 'Q', // expect 3 more follow byte
    STR_OK		= 'R',
    UTFX		= 'S', // following bytes
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
    char		stack[1024];
} *ojValidator;

typedef struct _ojParser {
    const char		*map;
    const char		*next_map;
    ojVal		stack;
    ojVal		results;
    struct _ojErr	err;
    ojVal		all_head;
    ojVal		all_tail;
    ojVal		all_dig;
    const char		*end;

    void		(*push)(ojVal val, void *ctx);
    void		(*pop)(void *ctx);
    ojVal		ready;

    ojParseCallback	cb;
    void		*ctx;

    ojCaller		caller;

    char		token[8];
    int			ri;
    uint32_t		ucode;
    bool		pp;
    bool		has_cb;
    bool		has_caller;
} *ojParser;

typedef struct _ReadBlock {
    atomic_flag		busy;
    ojStatus		status;
    bool		eof;
    byte		buf[16385];
} *ReadBlock;

typedef struct _ReadCtx {
    struct _ReadBlock	blocks[16];
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
.....w.......................H..\
.....w.......................G..\
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

static const char	big_digit_map[257] = "\
.........rs..r..................\
r...........u.D.CCCCCCCCCC......\
.....J.......................H..\
.....J.......................G..\
................................\
................................\
................................\
................................D";

static const char	big_dot_map[257] = "\
................................\
................IIIIIIIIII......\
................................\
................................\
................................\
................................\
................................\
................................o";

static const char	big_frac_map[257] = "\
.........rs..r..................\
r...........u...IIIIIIIIII......\
.....J.......................H..\
.....J.......................G..\
................................\
................................\
................................\
................................g";

static const char	big_exp_sign_map[257] = "\
................................\
...........K.K..LLLLLLLLLL......\
................................\
................................\
................................\
................................\
................................\
................................B";

static const char	big_exp_zero_map[257] = "\
................................\
................LLLLLLLLLL......\
................................\
................................\
................................\
................................\
................................\
................................Z";

static const char	big_exp_map[257] = "\
.........rs..r..................\
r...........u...LLLLLLLLLL......\
.............................H..\
.............................G..\
................................\
................................\
................................\
................................Y";

static const char	string_map[257] = "\
................................\
RRzRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\
RRRRRRRRRRRRRRRRRRRRRRRRRRRRARRR\
RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR\
................................\
................................\
MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\
PPPPPPPPPPPPPPPPQQQQQQQQ........s";

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

static const char	utf_map[257] = "\
................................\
................................\
................................\
................................\
SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS\
SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS\
................................\
................................8";

static const char	trail_map[257] = "\
.........ab..a..................\
a...............................\
................................\
................................\
................................\
................................\
................................\
................................R";


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

static long double	pow_map[401] = {
    1.0L,     1.0e1L,   1.0e2L,   1.0e3L,   1.0e4L,   1.0e5L,   1.0e6L,   1.0e7L,   1.0e8L,   1.0e9L,   //  00
    1.0e10L,  1.0e11L,  1.0e12L,  1.0e13L,  1.0e14L,  1.0e15L,  1.0e16L,  1.0e17L,  1.0e18L,  1.0e19L,  //  10
    1.0e20L,  1.0e21L,  1.0e22L,  1.0e23L,  1.0e24L,  1.0e25L,  1.0e26L,  1.0e27L,  1.0e28L,  1.0e29L,  //  20
    1.0e30L,  1.0e31L,  1.0e32L,  1.0e33L,  1.0e34L,  1.0e35L,  1.0e36L,  1.0e37L,  1.0e38L,  1.0e39L,  //  30
    1.0e40L,  1.0e41L,  1.0e42L,  1.0e43L,  1.0e44L,  1.0e45L,  1.0e46L,  1.0e47L,  1.0e48L,  1.0e49L,  //  40
    1.0e50L,  1.0e51L,  1.0e52L,  1.0e53L,  1.0e54L,  1.0e55L,  1.0e56L,  1.0e57L,  1.0e58L,  1.0e59L,  //  50
    1.0e60L,  1.0e61L,  1.0e62L,  1.0e63L,  1.0e64L,  1.0e65L,  1.0e66L,  1.0e67L,  1.0e68L,  1.0e69L,  //  60
    1.0e70L,  1.0e71L,  1.0e72L,  1.0e73L,  1.0e74L,  1.0e75L,  1.0e76L,  1.0e77L,  1.0e78L,  1.0e79L,  //  70
    1.0e80L,  1.0e81L,  1.0e82L,  1.0e83L,  1.0e84L,  1.0e85L,  1.0e86L,  1.0e87L,  1.0e88L,  1.0e89L,  //  80
    1.0e90L,  1.0e91L,  1.0e92L,  1.0e93L,  1.0e94L,  1.0e95L,  1.0e96L,  1.0e97L,  1.0e98L,  1.0e99L,  //  90
    1.0e100L, 1.0e101L, 1.0e102L, 1.0e103L, 1.0e104L, 1.0e105L, 1.0e106L, 1.0e107L, 1.0e108L, 1.0e109L, // 100
    1.0e110L, 1.0e111L, 1.0e112L, 1.0e113L, 1.0e114L, 1.0e115L, 1.0e116L, 1.0e117L, 1.0e118L, 1.0e119L, // 110
    1.0e120L, 1.0e121L, 1.0e122L, 1.0e123L, 1.0e124L, 1.0e125L, 1.0e126L, 1.0e127L, 1.0e128L, 1.0e129L, // 120
    1.0e130L, 1.0e131L, 1.0e132L, 1.0e133L, 1.0e134L, 1.0e135L, 1.0e136L, 1.0e137L, 1.0e138L, 1.0e139L, // 130
    1.0e140L, 1.0e141L, 1.0e142L, 1.0e143L, 1.0e144L, 1.0e145L, 1.0e146L, 1.0e147L, 1.0e148L, 1.0e149L, // 140
    1.0e150L, 1.0e151L, 1.0e152L, 1.0e153L, 1.0e154L, 1.0e155L, 1.0e156L, 1.0e157L, 1.0e158L, 1.0e159L, // 150
    1.0e160L, 1.0e161L, 1.0e162L, 1.0e163L, 1.0e164L, 1.0e165L, 1.0e166L, 1.0e167L, 1.0e168L, 1.0e169L, // 160
    1.0e170L, 1.0e171L, 1.0e172L, 1.0e173L, 1.0e174L, 1.0e175L, 1.0e176L, 1.0e177L, 1.0e178L, 1.0e179L, // 170
    1.0e180L, 1.0e181L, 1.0e182L, 1.0e183L, 1.0e184L, 1.0e185L, 1.0e186L, 1.0e187L, 1.0e188L, 1.0e189L, // 180
    1.0e190L, 1.0e191L, 1.0e192L, 1.0e193L, 1.0e194L, 1.0e195L, 1.0e196L, 1.0e197L, 1.0e198L, 1.0e199L, // 190
    1.0e200L, 1.0e201L, 1.0e202L, 1.0e203L, 1.0e204L, 1.0e205L, 1.0e206L, 1.0e207L, 1.0e208L, 1.0e209L, // 200
    1.0e210L, 1.0e211L, 1.0e212L, 1.0e213L, 1.0e214L, 1.0e215L, 1.0e216L, 1.0e217L, 1.0e218L, 1.0e219L, // 210
    1.0e220L, 1.0e221L, 1.0e222L, 1.0e223L, 1.0e224L, 1.0e225L, 1.0e226L, 1.0e227L, 1.0e228L, 1.0e229L, // 220
    1.0e230L, 1.0e231L, 1.0e232L, 1.0e233L, 1.0e234L, 1.0e235L, 1.0e236L, 1.0e237L, 1.0e238L, 1.0e239L, // 230
    1.0e240L, 1.0e241L, 1.0e242L, 1.0e243L, 1.0e244L, 1.0e245L, 1.0e246L, 1.0e247L, 1.0e248L, 1.0e249L, // 240
    1.0e250L, 1.0e251L, 1.0e252L, 1.0e253L, 1.0e254L, 1.0e255L, 1.0e256L, 1.0e257L, 1.0e258L, 1.0e259L, // 250
    1.0e260L, 1.0e261L, 1.0e262L, 1.0e263L, 1.0e264L, 1.0e265L, 1.0e266L, 1.0e267L, 1.0e268L, 1.0e269L, // 260
    1.0e270L, 1.0e271L, 1.0e272L, 1.0e273L, 1.0e274L, 1.0e275L, 1.0e276L, 1.0e277L, 1.0e278L, 1.0e279L, // 270
    1.0e280L, 1.0e281L, 1.0e282L, 1.0e283L, 1.0e284L, 1.0e285L, 1.0e286L, 1.0e287L, 1.0e288L, 1.0e289L, // 280
    1.0e290L, 1.0e291L, 1.0e292L, 1.0e293L, 1.0e294L, 1.0e295L, 1.0e296L, 1.0e297L, 1.0e298L, 1.0e299L, // 290
    1.0e300L, 1.0e301L, 1.0e302L, 1.0e303L, 1.0e304L, 1.0e305L, 1.0e306L, 1.0e307L, 1.0e308L, 1.0e309L, // 300
    1.0e310L, 1.0e311L, 1.0e312L, 1.0e313L, 1.0e314L, 1.0e315L, 1.0e316L, 1.0e317L, 1.0e318L, 1.0e319L, // 310
    1.0e320L, 1.0e321L, 1.0e322L, 1.0e323L, 1.0e324L, 1.0e325L, 1.0e326L, 1.0e327L, 1.0e328L, 1.0e329L, // 320
    1.0e330L, 1.0e331L, 1.0e332L, 1.0e333L, 1.0e334L, 1.0e335L, 1.0e336L, 1.0e337L, 1.0e338L, 1.0e339L, // 330
    1.0e340L, 1.0e341L, 1.0e342L, 1.0e343L, 1.0e344L, 1.0e345L, 1.0e346L, 1.0e347L, 1.0e348L, 1.0e349L, // 340
    1.0e350L, 1.0e351L, 1.0e352L, 1.0e353L, 1.0e354L, 1.0e355L, 1.0e356L, 1.0e357L, 1.0e358L, 1.0e359L, // 350
    1.0e360L, 1.0e361L, 1.0e362L, 1.0e363L, 1.0e364L, 1.0e365L, 1.0e366L, 1.0e367L, 1.0e368L, 1.0e369L, // 360
    1.0e370L, 1.0e371L, 1.0e372L, 1.0e373L, 1.0e374L, 1.0e375L, 1.0e376L, 1.0e377L, 1.0e378L, 1.0e379L, // 370
    1.0e380L, 1.0e381L, 1.0e382L, 1.0e383L, 1.0e384L, 1.0e385L, 1.0e386L, 1.0e387L, 1.0e388L, 1.0e389L, // 380
    1.0e390L, 1.0e391L, 1.0e392L, 1.0e393L, 1.0e394L, 1.0e395L, 1.0e396L, 1.0e397L, 1.0e398L, 1.0e399L, // 390
    1.0e400L
};

static void	oj_caller_push(ojParser p, ojCaller caller, ojVal val);

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
byte_error(ojErr err, const char *map, int off, byte b) {
    err->col = off - err->col + 1;
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
    default:
	oj_err_set(err, OJ_ERR_PARSE, "unexpected character '%c' in '%c' mode", b, map[256]);
	break;
    }
    return err->code;
}

static void
parse_free_stack(ojParser p) {
    ojVal	v;

    while (NULL != (v = p->stack)) {
	bool	found = false;

	p->stack = v->next;
	for (ojVal a = p->all_head; NULL != a; a = a->free) {
	    if (a == v) {
		found = true;
		break;
	    }
	}
	for (ojVal a = p->all_dig; NULL != a; a = a->free) {
	    if (a == v) {
		found = true;
		break;
	    }
	}
	if (!found) {
	    if ((OJ_STRING == v->type && sizeof(v->str.raw) <= v->str.len) ||
		(OJ_BIG == v->type && sizeof(v->num.raw) <= v->num.len) ||
		sizeof(v->key.raw) <= v->key.len) {
		v->free = p->all_dig;
		p->all_dig = v;
	    } else {
		v->free = p->all_head;
		if (NULL == p->all_head) {
		    p->all_tail = v;
		}
		p->all_head = v;
	    }
	}
    }
}

static ojStatus
parse_error(ojParser p, const char *fmt, ...) {
    va_list	ap;

    va_start(ap, fmt);
    vsnprintf(p->err.msg, sizeof(p->err.msg), fmt, ap);
    va_end(ap);
    p->err.code = OJ_ERR_PARSE;
    parse_free_stack(p);

    return p->err.code;
}

static ojVal
push_val(ojParser p, ojType type, ojMod mod) {
    ojVal	val;

    if (p->pp) {
	if (NULL != p->stack && OJ_NONE == p->stack->type) { // indicates a object member
	    val = p->stack;
	    val->type = type;
	    val->mod = mod;
	} else if (NULL != (val = p->ready)) {
	    p->ready = val->next;
	    val->type = type;
	    val->mod = mod;
	    val->key.len = 0;
	    val->next = p->stack;
	    p->stack = val;
	} else {
	    val = oj_val_create();
	    val->type = type;
	    val->mod = mod;
	    val->key.len = 0;
	    val->next = p->stack;
	    p->stack = val;
	}
	if (OJ_ARRAY == type || OJ_OBJECT == type) {
	    val->list.head = NULL;
	    p->push(val, p->ctx);
	}
    } else {
	if (NULL != p->stack && OJ_NONE == p->stack->type) { // indicates a object member
	    val = p->stack;
	    val->type = type;
	    val->mod = mod;
	} else {
	    val = oj_val_create();
	    val->type = type;
	    val->mod = mod;
	    val->key.len = 0;
	    val->next = p->stack;
	    p->stack = val;
	}
    }
    return val;
}

// Return true to stop.
static bool
pop_val(ojParser p) {
    ojVal	parent;
    ojVal	top = p->stack;

    if (p->pp) {
	if (OJ_ARRAY == top->type || OJ_OBJECT == top->type) {
	    p->pop(p->ctx);
	    _oj_val_clear(top);
	} else {
	    p->push(top, p->ctx);
	}
	p->stack = top->next;
	top->next = p->ready;
	p->ready = top;
	if (NULL == p->stack) {
	    p->map = value_map;
	} else {
	    p->map = after_map;
	}
    } else {
	// add to all list
	if ((OJ_STRING == top->type && sizeof(top->str.raw) <= top->str.len) ||
	    (OJ_BIG == top->type && sizeof(top->num.raw) <= top->num.len) ||
	    sizeof(top->key.raw) <= top->key.len) {
	    top->free = p->all_dig;
	    p->all_dig = top;
	} else {
	    top->free = p->all_head;
	    if (NULL == p->all_head) {
		p->all_tail = top;
	    }
	    p->all_head = top;
	}
	if (NULL == (parent = top->next)) {
	    if (p->has_cb) {
		ojCallbackOp	op = p->cb(top, p->ctx);

		if (0 != (OJ_DESTROY & op)) {
		    struct _ojReuser	r = {
			.head = p->all_head,
			.tail = p->all_tail,
			.dig = p->all_dig,
		    };
		    oj_reuse(&r);
		}
		if (0 != (OJ_STOP & op)) {
		    return true;
		}
		p->all_head = NULL;
		p->all_tail = NULL;
		p->all_dig = NULL;
		p->map = value_map;
	    } else if (p->has_caller) {
		oj_caller_push(p, p->caller, top);
		p->stack = NULL;
		p->map = value_map;

		return p->caller->done;
	    } else {
		top->next = p->results;
		p->results = p->stack;
		p->map = trail_map;
	    }
	    p->stack = NULL;
	} else {
	    if (NULL == parent->list.head) {
		parent->list.head = top;
	    } else {
		parent->list.tail->next = top;
	    }
	    parent->list.tail = top;
	    top->next = NULL;
	    p->stack = parent;
	    p->map = after_map;
	}
    }
    return false;
}

static void
calc_num(ojVal v) {
    switch (v->type) {
    case OJ_INT:
	if (v->num.neg) {
	    v->num.fixnum = -v->num.fixnum;
	}
	break;
    case OJ_DECIMAL: {
	long double	d = (long double)v->num.fixnum;

	if (v->num.neg) {
	    d = -d;
	}
	if (0 < v->num.shift) {
	    d /= pow_map[v->num.shift];
	}
	if (0 < v->num.exp) {
	    long double	x;

	    if (MAX_POW < v->num.exp) {
		x = powl(10.0L, (long double)v->num.exp);
	    } else {
		x = pow_map[v->num.exp];
	    }
	    if (v->num.exp_neg) {
		d /= x;
	    } else {
		d *= x;
	    }
	}
	v->num.dub = d;
	break;
    }
    }
}

static void
big_change(ojVal v) {
    switch (v->type) {
    case OJ_INT: {
	// If an int then it will fit in the num.raw so no need to check length;
	int64_t	i = v->num.fixnum;
	int	len = 0;

	for (len = 24; 0 < i; len--, i /= 10) {
	    v->num.raw[len] = '0' + (i % 10);
	}
	if (v->num.neg) {
	    v->num.raw[len] = '-';
	    len--;
	}
	v->num.len = 24 - len;
	memmove(v->num.raw, v->num.raw + len +1, v->num.len);
	v->num.raw[v->num.len] = '\0';
	v->type = OJ_BIG;
	break;
    }
    case OJ_DECIMAL: {
	int	shift = v->num.shift;
	int64_t	i = v->num.fixnum;
	int	len = 0;

	for (len = 24; 0 < i; len--, i /= 10, shift--) {
	    if (0 == shift) {
		v->num.raw[len] = '.';
		len--;
	    }
	    v->num.raw[len] = '0' + (i % 10);
	}
	if (v->num.neg) {
	    v->num.raw[len] = '-';
	    len--;
	}
	v->num.len = 24 - len;
	memmove(v->num.raw, v->num.raw + len +1, v->num.len);
	if (0 < v->num.exp) {
	    int		x = v->num.exp;
	    int		d;
	    bool	started = false;

	    v->num.raw[v->num.len++] = 'e';
	    if (0 < v->num.exp_neg) {
		v->num.raw[v->num.len++] = '-';
	    }
	    // There will at most 4 digits to left to right should be fine.
	    for (int div = 1000; 0 < div; div /= 10) {
		d = x / div % 10;
		if (started || 0 < d) {
		    v->num.raw[v->num.len++] = '0' + d;
		}
	    }
	}
	v->num.raw[v->num.len] = '\0';
	v->type = OJ_BIG;
	break;
    }
    }
}

static ojStatus
parse(ojParser p, const byte *json) {
    const byte *start;
    ojVal	v;
    const byte	*b = json;

    for (; '\0' != *b; b++) {
	//print_stack(p, "loop");
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
	    v = push_val(p, OJ_NONE, 0);
	    b++;
	    start = b;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    if ('"' == *b) {
		_oj_val_set_key(v, (char*)start, b - start);
		p->map = colon_map;
		break;
	    }
	    _oj_val_set_key(v, (char*)start, b - start);
	    b--;
	    p->map = string_map;
	    p->next_map = colon_map;
	    break;
	case AFTER_COMMA:
	    if (OJ_OBJECT == p->stack->type) {
		p->map = key_map;
	    } else {
		p->map = comma_map;
	    }
	    break;
	case VAL_QUOTE:
	    v = push_val(p, OJ_STRING, 0);
	    b++;
	    start = b;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    if ('"' == *b) {
		_oj_val_set_str(v, (char*)start, b - start);
		if (pop_val(p)) {
		    return OJ_ABORT;
		}
		p->map = (NULL == p->stack) ? value_map : after_map;
		break;
	    }
	    _oj_val_set_str(v, (char*)start, b - start);
	    b--;
	    p->map = string_map;
	    p->next_map = (NULL == p->stack->next) ? value_map : after_map;
	    break;
	case OPEN_OBJECT:
	    v = push_val(p, OJ_OBJECT, OJ_OBJ_RAW);
	    v->list.head = NULL;
	    v->list.tail = NULL;
	    p->map = key1_map;
	    break;
	case NUM_CLOSE_OBJECT:
	    calc_num(p->stack);
	    if (pop_val(p)) {
		return OJ_ABORT;
	    }
	    // flow through
	case CLOSE_OBJECT:
	    if (OJ_OBJECT != p->stack->type) {
		p->err.col = b - json - p->err.col + 1;
		return parse_error(p, "unexpected object close");
	    }
	    if (pop_val(p)) {
		return OJ_ABORT;
	    }
	    break;
	case OPEN_ARRAY:
	    v = push_val(p, OJ_ARRAY, 0);
	    v->list.head = NULL;
	    v->list.tail = NULL;
	    p->map = value_map;
	    break;
	case NUM_CLOSE_ARRAY:
	    calc_num(p->stack);
	    if (pop_val(p)) {
		return OJ_ABORT;
	    }
	    // flow through
	case CLOSE_ARRAY:
	    if (OJ_ARRAY != p->stack->type) {
		p->err.col = b - json - p->err.col + 1;
		return parse_error(p, "unexpected array close");
	    }
	    if (pop_val(p)) {
		return OJ_ABORT;
	    }
	    break;
	case NUM_COMMA:
	    calc_num(p->stack);
	    if (pop_val(p)) {
		return OJ_ABORT;
	    }
	    if (OJ_OBJECT == p->stack->type) {
		p->map = key_map;
	    } else {
		p->map = comma_map;
	    }
	    break;
	case VAL0:
	    v = push_val(p, OJ_INT, 0);
	    v->num.fixnum = 0;
	    v->num.neg = false;
	    v->num.shift = 0;
	    v->num.calc = false;
	    v->num.len = 0;
	    v->num.exp = 0;
	    v->num.exp_neg = false;
	    p->map = zero_map;
	    break;
	case VAL_NEG:
	    v = push_val(p, OJ_INT, 0);
	    v->num.fixnum = 0;
	    v->num.neg = true;
	    v->num.shift = 0;
	    v->num.calc = false;
	    v->num.len = 0;
	    v->num.exp = 0;
	    v->num.exp_neg = false;
	    p->map = neg_map;
	    break;;
	case VAL_DIGIT:
	    v = push_val(p, OJ_INT, 0);
	    v->num.fixnum = 0;
	    v->num.neg = false;
	    v->num.shift = 0;
	    v->num.calc = false;
	    v->num.exp = 0;
	    v->num.exp_neg = false;
	    v->num.len = 0;
	    p->map = digit_map;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
		uint64_t	x = (uint64_t)v->num.fixnum * 10 + (uint64_t)(*b - '0');

		// Tried just checking for an int less than zero but that
		// fails when optimization is on for some reason with the
		// clang compiler so us a bit mask instead.
		if (0 == (0x8000000000000000ULL & x)) {
		    v->num.fixnum = (int64_t)x;
		} else {
		    big_change(v);
		    p->map = big_digit_map;
		    break;
		}
	    }
	    b--;
	    break;
	case NUM_DIGIT:
	    v = p->stack;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
		uint64_t	x = v->num.fixnum * 10 + (uint64_t)(*b - '0');

		if (0 == (0x8000000000000000ULL & x)) {
		    v->num.fixnum = (int64_t)x;
		} else {
		    big_change(v);
		    p->map = big_digit_map;
		    break;
		}
	    }
	    b--;
	    break;
	case NUM_DOT:
	    p->stack->type = OJ_DECIMAL;
	    p->map = dot_map;
	    break;
	case NUM_FRAC:
	    p->map = frac_map;
	    v = p->stack;
	    for (; NUM_FRAC == frac_map[*b]; b++) {
		uint64_t	x = v->num.fixnum * 10 + (uint64_t)(*b - '0');

		if (0 == (0x8000000000000000ULL & x)) {
		    v->num.fixnum = (int64_t)x;
		    v->num.shift++;
		} else {
		    big_change(v);
		    p->map = big_frac_map;
		    break;
		}
	    }
	    b--;
	    break;
	case FRAC_E:
	    p->stack->type = OJ_DECIMAL;
	    p->map = exp_sign_map;
	    break;
	case NUM_ZERO:
	    p->map = zero_map;
	    break;
	case NEG_DIGIT:
	    v = p->stack;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
		uint64_t	x = v->num.fixnum * 10 + (uint64_t)(*b - '0');

		if (0 == (0x8000000000000000ULL & x)) {
		    v->num.fixnum = (int64_t)x;
		} else {
		    big_change(v);
		    p->map = big_digit_map;
		    break;
		}
	    }
	    b--;
	    p->map = digit_map;
	    break;
	case EXP_SIGN:
	    p->stack->num.exp_neg = ('-' == *b);
	    p->map = exp_zero_map;
	    break;
	case EXP_DIGIT:
	    v = p->stack;
	    p->map = exp_map;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
		int16_t	x = v->num.exp * 10 + (int16_t)(*b - '0');

		if (x <= MAX_EXP) {
		    v->num.exp = x;
		} else {
		    big_change(v);
		    p->map = big_exp_map;
		    break;
		}
	    }
	    b--;
	    break;
	case BIG_DIGIT:
	    start = b;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    _oj_append_num(&p->err, &p->stack->num, (char*)start, b - start);
	    b--;
	    break;
	case BIG_DOT:
	    p->stack->type = OJ_DECIMAL;
	    _oj_append_num(&p->err, &p->stack->num, ".", 1);
	    p->map = big_dot_map;
	    break;
	case BIG_FRAC:
	    p->map = big_frac_map;
	    start = b;
	    for (; NUM_FRAC == frac_map[*b]; b++) {
	    }
	    _oj_append_num(&p->err, &p->stack->num, (char*)start, b - start);
	    b--;
	case BIG_E:
	    p->stack->type = OJ_DECIMAL;
	    _oj_append_num(&p->err, &p->stack->num, (const char*)b, 1);
	    p->map = big_exp_sign_map;
	    break;
	case BIG_EXP_SIGN:
	    _oj_append_num(&p->err, &p->stack->num, (const char*)b, 1);
	    p->map = big_exp_zero_map;
	    break;
	case BIG_EXP:
	    start = b;
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    _oj_append_num(&p->err, &p->stack->num, (char*)start, b - start);
	    b--;
	    p->map = big_exp_map;
	    break;
	case NUM_SPC:
	    calc_num(p->stack);
	    if (pop_val(p)) {
		return OJ_ABORT;
	    }
	    break;
	case NUM_NEWLINE:
	    calc_num(p->stack);
	    if (pop_val(p)) {
		return OJ_ABORT;
	    }
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
		_oj_append_str(&p->err, &p->stack->key, start, b - start);
	    } else {
		_oj_append_str(&p->err, &p->stack->str, start, b - start);
	    }
	    if ('"' == *b) {
		p->map = p->next_map;
		if (':' != p->map[256]) {
		    if (pop_val(p)) {
			return OJ_ABORT;
		    }
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
		if (pop_val(p)) {
		    return OJ_ABORT;
		}
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
			_oj_append_str(&p->err, &p->stack->key, utf8, ulen);
		    } else {
			_oj_append_str(&p->err, &p->stack->str, utf8, ulen);
		    }
		} else {
		    return parse_error(p, "invalid unicode");
		}
		p->map = string_map;
	    }
	    break;
	case ESC_OK:
	    if (':' == p->next_map[256]) {
		_oj_append_str(&p->err, &p->stack->key, (byte*)&esc_byte_map[*b], 1);
	    } else {
		_oj_append_str(&p->err, &p->stack->str, (byte*)&esc_byte_map[*b], 1);
	    }
	    p->map = string_map;
	    break;
	case UTF1:
	    p->ri = 1;
	    p->map = utf_map;
	    if (':' == p->next_map[256]) {
		_oj_append_str(&p->err, &p->stack->key, b, 1);
	    } else {
		_oj_append_str(&p->err, &p->stack->str, b, 1);
	    }
	    break;
	case UTF2:
	    p->ri = 2;
	    p->map = utf_map;
	    if (':' == p->next_map[256]) {
		_oj_append_str(&p->err, &p->stack->key, b, 1);
	    } else {
		_oj_append_str(&p->err, &p->stack->str, b, 1);
	    }
	    break;
	case UTF3:
	    p->ri = 3;
	    p->map = utf_map;
	    if (':' == p->next_map[256]) {
		_oj_append_str(&p->err, &p->stack->key, b, 1);
	    } else {
		_oj_append_str(&p->err, &p->stack->str, b, 1);
	    }
	    break;
	case UTFX:
	    p->ri--;
	    if (':' == p->next_map[256]) {
		_oj_append_str(&p->err, &p->stack->key, b, 1);
	    } else {
		_oj_append_str(&p->err, &p->stack->str, b, 1);
	    }
	    if (p->ri <= 0) {
		p->map = string_map;
	    }
	    break;
	case VAL_NULL:
	    if ('u' == b[1] && 'l' == b[2] && 'l' == b[3]) {
		b += 3;
		push_val(p, OJ_NULL, 0);
		if (pop_val(p)) {
		    return OJ_ABORT;
		}
		break;
	    }
	    p->ri = 0;
	    *p->token = *b++;
	    for (int i = 1; i < 4; i++) {
		if ('\0' == *b) {
		    p->ri = i;
		    break;
		} else {
		    p->token[i] = *b++;
		}
	    }
	    if (0 < p->ri) {
		p->map = null_map;
		break;
	    }
	    p->err.col = b - json - p->err.col;
	    return parse_error(p, "expected null");
	case VAL_TRUE:
	    if ('r' == b[1] && 'u' == b[2] && 'e' == b[3]) {
		b += 3;
		push_val(p, OJ_TRUE, 0);
		if (pop_val(p)) {
		    return OJ_ABORT;
		}
		break;
	    }
	    p->ri = 0;
	    *p->token = *b++;
	    for (int i = 1; i < 4; i++) {
		if ('\0' == *b) {
		    p->ri = i;
		    break;
		} else {
		    p->token[i] = *b++;
		}
	    }
	    if (0 < p->ri) {
		p->map = true_map;
		break;
	    }
	    p->err.col = b - json - p->err.col;
	    return parse_error(p, "expected true");
	case VAL_FALSE:
	    if ('a' == b[1] && 'l' == b[2] && 's' == b[3] && 'e' == b[4]) {
		b += 4;
		push_val(p, OJ_FALSE, 0);
		if (pop_val(p)) {
		    return OJ_ABORT;
		}
		break;
	    }
	    p->ri = 0;
	    *p->token = *b++;
	    for (int i = 1; i < 5; i++) {
		if ('\0' == *b) {
		    p->ri = i;
		    break;
		} else {
		    p->token[i] = *b++;
		}
	    }
	    if (0 < p->ri) {
		p->map = false_map;
		break;
	    }
	    p->err.col = b - json - p->err.col;
	    return parse_error(p, "expected false");
	case TOKEN_OK:
	    p->token[p->ri] = *b;
	    p->ri++;
	    switch (p->map[256]) {
	    case 'N':
		if (4 == p->ri) {
		    if (0 != strncmp("null", p->token, 4)) {
			p->err.col = b - json - p->err.col;
			return parse_error(p, "expected null");
		    }
		    push_val(p, OJ_NULL, 0);
		    if (pop_val(p)) {
			return OJ_ABORT;
		    }
		}
		break;
	    case 'F':
		if (5 == p->ri) {
		    if (0 != strncmp("false", p->token, 5)) {
			p->err.col = b - json - p->err.col;
			return parse_error(p, "expected false");
		    }
		    push_val(p, OJ_FALSE, 0);
		    if (pop_val(p)) {
			return OJ_ABORT;
		    }
		}
		break;
	    case 'T':
		if (4 == p->ri) {
		    if (0 != strncmp("true", p->token, 4)) {
			p->err.col = b - json - p->err.col;
			return parse_error(p, "expected true");
		    }
		    push_val(p, OJ_TRUE, 0);
		    if (pop_val(p)) {
			return OJ_ABORT;
		    }
		}
		break;
	    default:
		p->err.col = b - json - p->err.col;
		return parse_error(p, "parse error");
	    }
	    break;
	case CHAR_ERR:
	    if (OJ_OK == p->err.code) {
		byte_error(&p->err, p->map, b - json, *b);
		parse_free_stack(p);
	    }
	    return p->err.code;
	default:
	    break;
	}
    }
    if (NULL != p->stack && NULL == p->stack->next) {
	switch (p->map[256]) {
	case '0':
	case 'd':
	case 'f':
	case 'z':
	case 'X':
	case 'D':
	case 'g':
	case 'Y':
	    calc_num(p->stack);
	    if (pop_val(p)) {
		return OJ_ABORT;
	    }
	    break;
	}
    }
    if ('R' == p->map[256]) {
	p->end = (const char*)b + 1;
    }
    return p->err.code;
}

ojStatus
oj_validate_str(ojErr err, const char *json_str) {
    struct _ojValidator	v;
    const byte		*json = (const byte*)json_str;

    memset(&v, 0, sizeof(v));
    v.err.line = 1;
    v.map = value_map;
    for (const byte *b = json; '\0' != *b; b++) {
	switch (v.map[*b]) {
	case SKIP_NEWLINE:
	    v.err.line++;
	    v.err.col = b - json;
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
	    v.map = value_map;
	    break;
	case SKIP_CHAR:
	    break;
	case KEY_QUOTE:
	    b++;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    b--;
	    v.map = string_map;
	    v.next_map = colon_map;
	    break;
	case AFTER_COMMA:
	    if (0 < v.depth && '{' == v.stack[v.depth-1]) {
		v.map = key_map;
	    } else {
		v.map = comma_map;
	    }
	    break;
	case VAL_QUOTE:
	    b++;
	    for (; STR_OK == string_map[*b]; b++) {
	    }
	    switch (*b) {
	    case '"': // normal termination
		v.map = (0 == v.depth) ? value_map : after_map;
		break;
	    case '\0':
		v.map = string_map;
		v.next_map = (0 == v.depth) ? value_map : after_map;
	    case '\\':
		v.map = esc_map;
		v.next_map = (0 == v.depth) ? value_map : after_map;
		break;
	    default:
		b--;
		v.map = string_map;
		v.next_map = (0 == v.depth) ? value_map : after_map;
		break;
	    }
	    break;
	case OPEN_OBJECT:
	    // TBD check depth vs stack len
	    v.stack[v.depth] = '{';
	    v.depth++;
	    v.map = key1_map;
	    break;
	case NUM_CLOSE_OBJECT:
	case CLOSE_OBJECT:
	    v.depth--;
	    v.map = (0 == v.depth) ? value_map : after_map;
	    if (v.depth < 0 || '{' != v.stack[v.depth]) {
		v.err.col = b - json - v.err.col;
		return oj_err_set(err, OJ_ERR_PARSE, "unexpected object close");
	    }
	    break;
	case OPEN_ARRAY:
	    // TBD check depth vs stack len
	    v.stack[v.depth] = '[';
	    v.depth++;
	    v.map = value_map;
	    break;
	case NUM_CLOSE_ARRAY:
	case CLOSE_ARRAY:
	    v.depth--;
	    v.map = (0 == v.depth) ? value_map : after_map;
	    if (v.depth < 0 || '[' != v.stack[v.depth]) {
		v.err.col = b - json - v.err.col;
		return oj_err_set(err, OJ_ERR_PARSE, "unexpected array close");
	    }
	    break;
	case NUM_COMMA:
	    if (0 < v.depth && '{' == v.stack[v.depth-1]) {
		v.map = key_map;
	    } else {
		v.map = comma_map;
	    }
	    break;
	case VAL0:
	    v.map = zero_map;
	    break;
	case VAL_NEG:
	    v.map = neg_map;
	    break;;
	case VAL_DIGIT:
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    b--;
	    v.map = digit_map;
	    break;
	case NUM_DIGIT:
	    for (; NUM_DIGIT == digit_map[*b]; b++) {
	    }
	    b--;
	    break;
	case NUM_DOT:
	    v.map = dot_map;
	    break;
	case NUM_FRAC:
	    v.map = frac_map;
	    for (; NUM_FRAC == frac_map[*b]; b++) {
	    }
	    b--;
	    break;
	case FRAC_E:
	    v.map = exp_sign_map;
	    break;
	case NUM_ZERO:
	    v.map = zero_map;
	    break;
	case NEG_DIGIT:
	    v.map = digit_map;
	    break;
	case EXP_SIGN:
	    v.map = exp_zero_map;
	    break;
	case EXP_DIGIT:
	    v.map = exp_map;
	    break;
	case NUM_SPC:
	    v.map = (0 == v.depth) ? value_map : after_map;
	    break;
	case NUM_NEWLINE:
	    v.map = (0 == v.depth) ? value_map : after_map;
	    v.err.line++;
	    v.err.col = b - json;
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
	    v.map = esc_map;
	    break;
	case STR_QUOTE:
	    v.map = v.next_map;
	    break;
	case ESC_U:
	    v.map = u_map;
	    v.ri = 0;
	    break;
	case U_OK:
	    v.ri++;
	    if (4 <= v.ri) {
		v.map = string_map;
	    }
	    break;
	case ESC_OK:
	    v.map = string_map;
	    break;
	case UTF1:
	    v.ri = 1;
	    v.map = utf_map;
	    break;
	case UTF2:
	    v.ri = 2;
	    v.map = utf_map;
	    break;
	case UTF3:
	    v.ri = 3;
	    v.map = utf_map;
	    break;
	case UTFX:
	    v.ri--;
	    if (v.ri <= 0) {
		v.map = string_map;
	    }
	    break;
	case VAL_NULL:
	    if ('u' == b[1] && 'l' == b[2] && 'l' == b[3]) {
		b += 3;
		v.map = (0 == v.depth) ? value_map : after_map;
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		v.map = null_map;
		v.ri = 0;
	    } else {
		v.err.col = b - json - v.err.col;
		return oj_err_set(err, OJ_ERR_PARSE, "expected null");
	    }
	    break;
	case VAL_TRUE:
	    if ('r' == b[1] && 'u' == b[2] && 'e' == b[3]) {
		b += 3;
		v.map = (0 == v.depth) ? value_map : after_map;
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3]) {
		v.map = true_map;
		v.ri = 0;
	    } else {
		v.err.col = b - json - v.err.col;
		return oj_err_set(err, OJ_ERR_PARSE, "expected true");
	    }
	    break;
	case VAL_FALSE:
	    if ('a' == b[1] && 'l' == b[2] && 's' == b[3] && 'e' == b[4]) {
		b += 4;
		v.map = (0 == v.depth) ? value_map : after_map;
	    } else if ('\0' == b[1] || '\0' == b[2] || '\0' == b[3] || '\0' == b[4]) {
		v.map = false_map;
		v.ri = 0;
	    } else {
		v.err.col = b - json - v.err.col;
		return oj_err_set(err, OJ_ERR_PARSE, "expected false");
	    }
	    break;
	case CHAR_ERR:
	    return byte_error(err, v.map, b - json, *b);
	default:
	    v.err.col = b - json - v.err.col;
	    return oj_err_set(err, OJ_ERR_PARSE, "internal error, unknown mode");
	    break;
	}
    }
    return OJ_OK;
}

static void
read_block(int fd, ReadBlock b) {
    size_t	rcnt = read(fd, b->buf, sizeof(b->buf) - 1);

    if (0 < rcnt) {
	b->buf[rcnt] = '\0';
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
    ReadBlock	b = rc->blocks;
    ReadBlock	next;

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
	while (atomic_flag_test_and_set(&next->busy)) {
	    one_beat();
	}
	atomic_flag_clear(&b->busy);
	b = next;
    }
    atomic_flag_clear(&b->busy);
    pthread_exit(0);

    return NULL;
}

static ojStatus
parse_large(ojParser p, int fd) {
    struct _ReadCtx	rc;
    ReadBlock		b; // current block being parsed
    ReadBlock		prev;

    memset(&rc, 0, sizeof(rc));
    rc.bend = rc.blocks + sizeof(rc.blocks) / sizeof(*rc.blocks);
    rc.fd = fd;

    for (b = rc.blocks; b < rc.bend; b++) {
	atomic_flag_clear(&b->busy);
    }
    prev = rc.bend - 1;
    b = rc.blocks;

    // Lock the prev block as if the parser has already finished but not yet
    // grabbed the next block.
    atomic_flag_test_and_set(&prev->busy);
    // Lock the first block for the reader.
    atomic_flag_test_and_set(&b->busy);

    pthread_t	t;
    int		err;

    if (0 != (err = pthread_create(&t, NULL, reader, (void*)&rc))) {
	return oj_err_set(&p->err, err, "failed to create reader thread");
    }
    pthread_detach(t);

    while (true) {
	// Get the lock on the current before unlocking the previous to assure
	// the reader doesn't overtake the parser.
	while (atomic_flag_test_and_set(&b->busy)) {
	    one_beat();
	}
	atomic_flag_clear(&prev->busy);
	if (OJ_OK != b->status) { // could be set by reader
	    oj_err_set(&p->err, b->status, "read failed");
	    break;
	}
	if (b->eof) {
	    break;
	}
	if (OJ_OK != parse(p, b->buf)) {
	    b->status = p->err.code;
	    break;
	}
	prev = b;
	b++;
	if (rc.bend <= b) {
	    b = rc.blocks;
	}
    }
    atomic_flag_clear(&b->busy);

    return p->err.code;
}

//// parse string functions

ojVal
oj_parse_str(ojErr err, const char *json, ojReuser reuser) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.has_cb = false;
    p.err.line = 1;
    p.map = value_map;
    parse(&p, (const byte*)json);
    if (NULL != reuser) {
	reuser->head = p.all_head;
	reuser->tail = p.all_tail;
	reuser->dig = p.all_dig;
    }
    if (OJ_OK != p.err.code && OJ_ABORT != p.err.code) {
	if (NULL != err) {
	    *err = p.err;
	}
	return NULL;
    }
    return p.results;
}

ojVal
oj_parse_strp(ojErr err, const char **json, ojReuser reuser) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.has_cb = false;
    p.err.line = 1;
    p.map = value_map;
    parse(&p, *(const byte**)json);
    *json = p.end;
    if (NULL != reuser) {
	reuser->head = p.all_head;
	reuser->tail = p.all_tail;
	reuser->dig = p.all_dig;
    }
    if (OJ_OK != p.err.code && OJ_ABORT != p.err.code) {
	if (NULL != err) {
	    *err = p.err;
	}
	return NULL;
    }
    return p.results;
}

ojStatus
oj_parse_str_cb(ojErr err, const char *json, ojParseCallback cb, void *ctx) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.cb = cb;
    p.has_cb = (NULL != cb);
    p.ctx = ctx;
    p.err.line = 1;
    p.map = value_map;
    parse(&p, (const byte*)json);
    if (OJ_OK != p.err.code && OJ_ABORT != p.err.code) {
	if (NULL != err) {
	    *err = p.err;
	}
	return p.err.code;
    }
    return OJ_OK;
}

ojStatus
oj_parse_str_call(ojErr err, const char *json, ojCaller caller) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.has_cb = false;
    p.has_caller = true;
    p.caller = caller;
    p.err.line = 1;
    p.map = value_map;
    parse(&p, (const byte*)json);
    oj_caller_push(&p, caller, NULL);
    if (OJ_OK != p.err.code && OJ_ABORT != p.err.code) {
	if (NULL != err) {
	    *err = p.err;
	}
    }
    return p.err.code;;
}

ojStatus
oj_pp_parse_str(ojErr err, const char *json, void (*push)(ojVal val, void *ctx), void (*pop)(void *ctx), void *ctx) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.pp = true;
    p.ctx = ctx;
    p.push = push;
    p.pop = pop;
    p.err.line = 1;
    p.map = value_map;
    parse(&p, (const byte*)json);
    if (OJ_OK != p.err.code && NULL != err) {
	*err = p.err;
    }
    if (NULL != p.ready) {
	p.ready->type = OJ_ARRAY;
	p.ready->list.head = p.ready->next;
	p.ready->list.tail = p.ready->next;
	if (NULL != p.ready->list.tail) {
	    for (; NULL != p.ready->list.tail->next; p.ready->list.tail = p.ready->list.tail->next) {
	    }
	}
	oj_destroy(p.ready);
    }
    return p.err.code;
}

//// parse file descriptor functions

ojVal
oj_parse_fd(ojErr err, int fd, ojReuser reuser) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.has_cb = false;
    p.err.line = 1;
    p.map = value_map;

    struct stat	info;

    // st_size will be 0 if not a file
    if (0 == fstat(fd, &info) && USE_THREAD_LIMIT < info.st_size && false) {
	// Use threaded version.
	parse_large(&p, fd);
	if (OJ_OK != p.err.code) {
	    if (NULL != err) {
		*err = p.err;
	    }
	    return NULL;
	}
	return p.results;
    }
    byte	buf[16385];
    size_t	size = sizeof(buf) - 1;
    size_t	rsize;

    while (true) {
	if (0 < (rsize = read(fd, buf, size))) {
	    buf[rsize] = '\0';
	    if (OJ_OK != parse(&p, buf)) {
		break;
	    }
	}
	if (rsize <= 0) {
	    if (0 != rsize) {
		if (NULL != err) {
		    oj_err_no(err, "read error");
		}
		return NULL;
	    }
	    break;
	}
    }
    if (NULL != reuser) {
	reuser->head = p.all_head;
	reuser->tail = p.all_tail;
	reuser->dig = p.all_dig;
    }
    if (OJ_OK != p.err.code) {
	if (NULL != err) {
	    *err = p.err;
	}
	return NULL;
    }
    return p.results;
}

static ojStatus
parse_fd(ojParser p, ojErr err, int fd) {
    struct stat	info;

    // st_size will be 0 if not a file
    if (0 == fstat(fd, &info) && USE_THREAD_LIMIT < info.st_size) {
	// Use threaded version.
	parse_large(p, fd);
	if (OJ_OK != p->err.code) {
	    if (NULL != err) {
		*err = p->err;
	    }
	    return p->err.code;
	}
	return OJ_OK;
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
		if (NULL != err) {
		    oj_err_no(err, "read error");
		}
		return errno;
	    }
	    break;
	}
    }
    if (OJ_OK != p->err.code) {
	if (NULL != err) {
	    *err = p->err;
	}
	return p->err.code;
    }
    return OJ_OK;
}

ojStatus
oj_parse_fd_cb(ojErr err, int fd, ojParseCallback cb, void *ctx) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.cb = cb;
    p.has_cb = (NULL != cb);
    p.ctx = ctx;
    p.err.line = 1;
    p.map = value_map;

    return parse_fd(&p, err, fd);
}

ojStatus
oj_parse_fd_call(ojErr err, int fd, ojCaller caller) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.has_cb = false;
    p.has_caller = true;
    p.caller = caller;
    p.err.line = 1;
    p.map = value_map;

    parse_fd(&p, err, fd);
    oj_caller_push(&p, caller, NULL);

    return p.err.code;
}

ojStatus
oj_pp_parse_fd(ojErr err, int fd, void (*push)(ojVal val, void *ctx), void (*pop)(void * ctx), void *ctx) {
    struct _ojParser	p;

    memset(&p, 0, sizeof(p));
    p.pp = true;
    p.ctx = ctx;
    p.push = push;
    p.pop = pop;
    p.err.line = 1;
    p.map = value_map;
    parse_fd(&p, err, fd);

    if (OJ_OK != p.err.code && NULL != err) {
	*err = p.err;
    }
    if (NULL != p.ready) {
	p.ready->type = OJ_ARRAY;
	p.ready->list.head = p.ready->next;
	p.ready->list.tail = p.ready->next;
	if (NULL != p.ready->list.tail) {
	    for (; NULL != p.ready->list.tail->next; p.ready->list.tail = p.ready->list.tail->next) {
	    }
	}
	oj_destroy(p.ready);
    }
    return OJ_OK;
}

ojVal
oj_parse_file(ojErr err, const char *filename, ojReuser reuser) {
    int	fd = open(filename, O_RDONLY);

    if (fd < 0) {
	if (NULL != err) {
	    oj_err_no(err, "error opening %s", filename);
	}
	return NULL;
    }
    ojVal	val = oj_parse_fd(err, fd, reuser);

    close(fd);

    return val;
}

ojStatus
oj_parse_file_cb(ojErr err, const char *filename, ojParseCallback cb, void *ctx) {
    int	fd = open(filename, O_RDONLY);

    if (fd < 0) {
	if (NULL != err) {
	    oj_err_no(err, "error opening %s", filename);
	}
	return errno;
    }
    ojStatus	status = oj_parse_fd_cb(err, fd, cb, ctx);

    close(fd);

    return status;
}

ojStatus
oj_parse_file_call(ojErr err, const char *filepath, ojCaller caller) {
    int	fd = open(filepath, O_RDONLY);

    if (fd < 0) {
	if (NULL != err) {
	    oj_err_no(err, "error opening %s", filepath);
	}
	return errno;
    }
    ojStatus	status = oj_parse_fd_call(err, fd, caller);

    close(fd);

    return status;
}

ojStatus
oj_pp_parse_file(ojErr err, const char *filepath, void (*push)(ojVal val, void *ctx), void (*pop)(void * ctx), void *ctx) {
    int	fd = open(filepath, O_RDONLY);

    if (fd < 0) {
	if (NULL != err) {
	    oj_err_no(err, "error opening %s", filepath);
	}
	return errno;
    }
    ojStatus	status = oj_pp_parse_fd(err, fd, push, pop, ctx);

    close(fd);

    return status;
}

static void*
caller_loop(void *ctx) {
    ojCaller		caller = (ojCaller)ctx;
    struct _ojCall	c;
    ojCallbackOp	op;
    ojCall		head = caller->queue;
    ojCall		next;

    atomic_flag_clear(&caller->starting);
    while (atomic_flag_test_and_set(&head->busy)) {
	one_beat();
    }
    while (true) {
	next = head + 1;
	if (caller->end <= next) {
	    next = caller->queue;
	}
	c = *head;
	head->val = NULL;
	if (NULL == c.val) {
	    atomic_flag_clear(&head->busy);
	    break;
	}
	op = caller->cb(c.val, caller->ctx);
	if (0 != (OJ_DESTROY & op)) {
	    oj_reuse(&c.reuser);
	}
	if (0 != (OJ_STOP & op)) {
	    caller->done = true;
	    break;
	}
	while (atomic_flag_test_and_set(&next->busy)) {
	    one_beat();
	}
	atomic_flag_clear(&head->busy);
	head = next;
    }
    return NULL;
}

static void
oj_caller_push(ojParser p, ojCaller caller, ojVal val) {
    // Should be sitting on a slot waiting for a push.
    ojCall	tail = caller->tail;

    tail->val = val;
    tail->reuser.head = p->all_head;
    tail->reuser.tail = p->all_tail;
    tail->reuser.dig = p->all_dig;

    tail++;
    if (caller->end <= tail) {
	tail = caller->queue;
    }
    while (atomic_flag_test_and_set(&tail->busy)) {
	one_beat();
    }
    atomic_flag_clear(&caller->tail->busy);
    caller->tail = tail;
    p->all_head = NULL;
    p->all_tail = NULL;
    p->all_dig = NULL;
}

ojStatus
oj_caller_start(ojErr err, ojCaller caller, ojParseCallback cb, void *ctx) {
    memset(caller, 0, sizeof(struct _ojCaller));
    caller->cb = cb;
    caller->ctx = ctx;
    caller->end = caller->queue + sizeof(caller->queue) / sizeof(*caller->queue);
    caller->tail = caller->queue;
    for (ojCall c = caller->queue; c < caller->end; c++) {
	atomic_flag_clear(&c->busy);
    }
    atomic_flag_test_and_set(&caller->queue->busy);
    atomic_flag_test_and_set(&caller->starting);

    int	status;

    if (0 != (status = pthread_create(&caller->thread, NULL, caller_loop, (void*)caller))) {
	return oj_err_set(err, status, "failed to create caller thread");
    }
    while (atomic_flag_test_and_set(&caller->starting)) {
	one_beat();
    }
    atomic_flag_clear(&caller->starting);

    return OJ_OK;
}

void
oj_caller_shutdown(ojCaller caller) {
    pthread_cancel(caller->thread);
    pthread_join(caller->thread, NULL);
}

void
oj_caller_wait(ojCaller caller) {
    ojCall	tail = caller->tail;

    tail->val = NULL;
    tail->reuser.head = NULL;
    tail->reuser.tail = NULL;
    tail->reuser.dig = NULL;
    atomic_flag_clear(&tail->busy);

    pthread_join(caller->thread, NULL);
}

void
_oj_val_append_str(ojParser p, const byte *s, size_t len) {
    _oj_append_str(&p->err, &p->stack->str, s, len);
}
