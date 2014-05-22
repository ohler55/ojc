/* parse.c
 * Copyright (c) 2013, Peter Ohler
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  - Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 *  - Neither the name of Peter Ohler nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "ojc.h"
#include "parse.h"
#include "buf.h"
#include "val.h"

#define EXP_MAX		1023
#define DEC_MAX		14
#define DIV_MAX		100000000000000000LL
#define BIGGISH		922337203685477580LL

void	ojc_set_error_at(ParseInfo pi, ojcErrCode code, const char* file, int line, const char *format, ...);

static void
skip_comment(ParseInfo pi) {
    char	c = reader_get(&pi->err, &pi->rd);

    if ('*' == c) {
	while ('\0' != (c = reader_get(&pi->err, &pi->rd))) {
	    if ('*' == c) {
		c = reader_get(&pi->err, &pi->rd);
		if ('/' == c) {
		    return;
		}
	    }
	}
    } else if ('/' == c) {
	while ('\0' != (c = reader_get(&pi->err, &pi->rd))) {
	    switch (c) {
	    case '\n':
	    case '\r':
	    case '\f':
	    case '\0':
		return;
	    default:
		break;
	    }
	}
    } else {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "invalid comment format");
    }
    if ('\0' == c) {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "comment not terminated");
	return;
    }
}

static ojcVal
get_val(ParseInfo pi, ojcValType type) {
    ojcVal	v;

    if (0 == pi->free_vals.head) {
	_ojc_val_create_batch(32, &pi->free_vals);
    }
    v = pi->free_vals.head;
    pi->free_vals.head = v->next;
    if (0 == pi->free_vals.head) {
	pi->free_vals.tail = 0;
    }
    v->next = 0;
    v->key_type = STR_NONE;
    v->str_type = STR_NONE;
    v->members.head = 0;
    v->members.tail = 0;
    v->type = type;
    v->expect = NEXT_NONE;

    return v;
}

static void
add_value(ParseInfo pi, ojcVal val) {
    ojcVal	parent = stack_peek(&pi->stack);

    if (0 == parent) { // simple add
	*pi->stack.head = val;
    } else {
	switch (parent->expect) {
	case NEXT_ARRAY_NEW:
	case NEXT_ARRAY_ELEMENT:
	    ojc_array_append(&pi->err, parent, val);
	    parent->expect = NEXT_ARRAY_COMMA;
	    break;
	case NEXT_OBJECT_VALUE:
	    ojc_object_nappend(&pi->err, parent, pi->key, pi->klen, val);
	    if (pi->kalloc) {
		free(pi->key);
	    }
	    pi->key = 0;
	    pi->klen = 0;
	    pi->kalloc = false;
	    parent->expect = NEXT_OBJECT_COMMA;
	    break;
	case NEXT_OBJECT_NEW:
	case NEXT_OBJECT_KEY:
	case NEXT_OBJECT_COMMA:
	case NEXT_NONE:
	case NEXT_ARRAY_COMMA:
	case NEXT_OBJECT_COLON:
	default:
	    ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "expected %s", _ojc_stack_next_str(parent->expect));
	    break;
	}
    }
}

static void
add_str(ParseInfo pi, const char *str, int len) {
    ojcVal	parent = stack_peek(&pi->stack);

    if (0 == parent) { // simple add
	*pi->stack.head = ojc_create_str(str, len);
    } else {
	switch (parent->expect) {
	case NEXT_ARRAY_NEW:
	case NEXT_ARRAY_ELEMENT:
	    ojc_array_append(&pi->err, parent, ojc_create_str(str, len));
	    parent->expect = NEXT_ARRAY_COMMA;
	    break;
	case NEXT_OBJECT_NEW:
	case NEXT_OBJECT_KEY:
	    if (sizeof(pi->karray) <= len) {
		pi->key = strndup(str, len);
		pi->kalloc = true;
	    } else {
		memcpy(pi->karray, str, len);
		pi->karray[len] = '\0';
		pi->key = pi->karray;
		pi->kalloc = false;
	    }
	    pi->klen = len;
	    parent->expect = NEXT_OBJECT_COLON;
	    break;
	case NEXT_OBJECT_VALUE:
	    ojc_object_nappend(&pi->err, parent, pi->key, pi->klen, ojc_create_str(str, len));
	    if (pi->kalloc) {
		free(pi->key);
	    }
	    pi->key = 0;
	    pi->klen = 0;
	    pi->kalloc = false;
	    parent->expect = NEXT_OBJECT_COMMA;
	    break;
	case NEXT_OBJECT_COMMA:
	case NEXT_NONE:
	case NEXT_ARRAY_COMMA:
	case NEXT_OBJECT_COLON:
	default:
	    ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "expected %s, not a string", _ojc_stack_next_str(parent->expect));
	    break;
	}
    }
    reader_release(&pi->rd);
}

static void
object_start(ParseInfo pi) {
    ojcVal	obj = get_val(pi, OJC_OBJECT);

    add_value(pi, obj);
    stack_push(&pi->stack, obj);
}

static void
object_end(ParseInfo pi) {
    ojcVal	obj = stack_pop(&pi->stack);

    if (0 == obj) {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "unexpected object close");
    } else if (NEXT_OBJECT_COMMA != obj->expect && NEXT_OBJECT_NEW != obj->expect) {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "expected %s, not a hash close", _ojc_stack_next_str(obj->expect));
    }
}

static void
array_start(ParseInfo pi) {
    ojcVal	array = get_val(pi, OJC_ARRAY);

    add_value(pi, array);
    stack_push(&pi->stack, array);
}

static void
array_end(ParseInfo pi) {
    ojcVal	array = stack_pop(&pi->stack);

    if (0 == array) {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "unexpected array close");
    } else if (NEXT_ARRAY_COMMA != array->expect && NEXT_ARRAY_NEW != array->expect) {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "expected %s, not an array close", _ojc_stack_next_str(array->expect));
    }
}

static void
read_null(ParseInfo pi) {
    if (0 == reader_expect(&pi->err, &pi->rd, "ull")) {
	add_value(pi, get_val(pi, OJC_NULL));
    } else {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "expected null");
    }
}

static void
read_true(ParseInfo pi) {
    if (0 == reader_expect(&pi->err, &pi->rd, "rue")) {
	add_value(pi, get_val(pi, OJC_TRUE));
    } else {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "expected true");
    }
}

static void
read_false(ParseInfo pi) {
    if (0 == reader_expect(&pi->err, &pi->rd, "alse")) {
	add_value(pi, get_val(pi, OJC_FALSE));
    } else {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "expected false");
    }
}

static uint32_t
read_hex(ParseInfo pi) {
    uint32_t	b = 0;
    int		i;
    char	c;

    for (i = 0; i < 4; i++) {
	c = reader_get(&pi->err, &pi->rd);
	b = b << 4;
	if ('0' <= c && c <= '9') {
	    b += c - '0';
	} else if ('A' <= c && c <= 'F') {
	    b += c - 'A' + 10;
	} else if ('a' <= c && c <= 'f') {
	    b += c - 'a' + 10;
	} else {
	    ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "invalid hex character");
	    return 0;
	}
    }
    return b;
}

static void
unicode_to_chars(ParseInfo pi, Buf buf, uint32_t code) {
    if (0x0000007F >= code) {
	buf_append(buf, (char)code);
    } else if (0x000007FF >= code) {
	buf_append(buf, 0xC0 | (code >> 6));
	buf_append(buf, 0x80 | (0x3F & code));
    } else if (0x0000FFFF >= code) {
	buf_append(buf, 0xE0 | (code >> 12));
	buf_append(buf, 0x80 | ((code >> 6) & 0x3F));
	buf_append(buf, 0x80 | (0x3F & code));
    } else if (0x001FFFFF >= code) {
	buf_append(buf, 0xF0 | (code >> 18));
	buf_append(buf, 0x80 | ((code >> 12) & 0x3F));
	buf_append(buf, 0x80 | ((code >> 6) & 0x3F));
	buf_append(buf, 0x80 | (0x3F & code));
    } else if (0x03FFFFFF >= code) {
	buf_append(buf, 0xF8 | (code >> 24));
	buf_append(buf, 0x80 | ((code >> 18) & 0x3F));
	buf_append(buf, 0x80 | ((code >> 12) & 0x3F));
	buf_append(buf, 0x80 | ((code >> 6) & 0x3F));
	buf_append(buf, 0x80 | (0x3F & code));
    } else if (0x7FFFFFFF >= code) {
	buf_append(buf, 0xFC | (code >> 30));
	buf_append(buf, 0x80 | ((code >> 24) & 0x3F));
	buf_append(buf, 0x80 | ((code >> 18) & 0x3F));
	buf_append(buf, 0x80 | ((code >> 12) & 0x3F));
	buf_append(buf, 0x80 | ((code >> 6) & 0x3F));
	buf_append(buf, 0x80 | (0x3F & code));
    } else {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "invalid Unicode character");
    }
}

// entered at backslash
static void
read_escaped_str(ParseInfo pi) {
    struct _Buf	buf;
    char	c;
    uint32_t	code;

    buf_init(&buf);
    if (pi->rd.str < pi->rd.tail) {
	buf_append_string(&buf, pi->rd.str, pi->rd.tail - pi->rd.str);
    }
    while ('\"' != (c = reader_get(&pi->err, &pi->rd))) {
	if ('\0' == c) {
	    ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "quoted string not terminated");
	    buf_cleanup(&buf);
	    return;
	} else if ('\\' == c) {
	    c = reader_get(&pi->err, &pi->rd);
	    switch (c) {
	    case 'n':	buf_append(&buf, '\n');	break;
	    case 'r':	buf_append(&buf, '\r');	break;
	    case 't':	buf_append(&buf, '\t');	break;
	    case 'f':	buf_append(&buf, '\f');	break;
	    case 'b':	buf_append(&buf, '\b');	break;
	    case '"':	buf_append(&buf, '"');	break;
	    case '/':	buf_append(&buf, '/');	break;
	    case '\\':	buf_append(&buf, '\\');	break;
	    case 'u':
		if (0 == (code = read_hex(pi)) && OJC_OK != pi->err.code) {
		    buf_cleanup(&buf);
		    return;
		}
		if (0x0000D800 <= code && code <= 0x0000DFFF) {
		    uint32_t	c1 = (code - 0x0000D800) & 0x000003FF;
		    uint32_t	c2;
		    char	ch2;

		    c = reader_get(&pi->err, &pi->rd);
		    ch2 = reader_get(&pi->err, &pi->rd);
		    if ('\\' != c || 'u' != ch2) {
			ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "invalid escaped character");
			buf_cleanup(&buf);
			return;
		    }
		    if (0 == (c2 = read_hex(pi)) && OJC_OK != pi->err.code) {
			buf_cleanup(&buf);
			return;
		    }
		    c2 = (c2 - 0x0000DC00) & 0x000003FF;
		    code = ((c1 << 10) | c2) + 0x00010000;
		}
		unicode_to_chars(pi, &buf, code);
		if (OJC_OK != pi->err.code) {
		    buf_cleanup(&buf);
		    return;
		}
		break;
	    default:
		ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "invalid escaped character");
		buf_cleanup(&buf);
		return;
	    }
	} else {
	    buf_append(&buf, c);
	}
    }
    add_str(pi, buf.head, buf_len(&buf));
    buf_cleanup(&buf);
}

static void
read_str(ParseInfo pi) {
    char	c;

    reader_protect(&pi->rd);
    while ('\"' != (c = reader_get(&pi->err, &pi->rd))) {
	if ('\0' == c) {
	    ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "quoted string not terminated");
	    return;
	} else if ('\\' == c) {
	    reader_backup(&pi->rd);
	    read_escaped_str(pi);
	    reader_release(&pi->rd);
	    return;
	}
    }
    add_str(pi, pi->rd.start, pi->rd.tail - pi->rd.start - 1);
    reader_release(&pi->rd);
}

static void
read_num(ParseInfo pi) {
    ojcVal	val;
    int		zero_cnt = 0;
    int64_t	i = 0;
    int64_t	num = 0;
    int64_t	div = 1;
    long	exp = 0;
    int		dec_cnt = 0;
    int		big = 0;
    int		neg = 0;
    char	c;

    reader_protect(&pi->rd);
    c = reader_get(&pi->err, &pi->rd);
    if ('-' == c) {
	c = reader_get(&pi->err, &pi->rd);
	neg = 1;
    } else if ('+' == c) {
	c = reader_get(&pi->err, &pi->rd);
    }
    for (; '0' <= c && c <= '9'; c = reader_get(&pi->err, &pi->rd)) {
	dec_cnt++;
	if (big) {
	    big++;
	} else {
	    int	d = (c - '0');

	    if (0 == d) {
		zero_cnt++;
	    } else {
		zero_cnt = 0;
	    }
	    if (BIGGISH < i && (INT64_MAX - (int64_t)d) / 10LL <= i) {
		big = 1;
	    } else {
		i = i * 10 + d;
	    }
	}
    }
    if ('.' == c) {
	c = reader_get(&pi->err, &pi->rd);
	for (; '0' <= c && c <= '9'; c = reader_get(&pi->err, &pi->rd)) {
	    int	d = (c - '0');

	    if (0 == d) {
		zero_cnt++;
	    } else {
		zero_cnt = 0;
	    }
	    dec_cnt++;
	    if ((BIGGISH < num && (INT64_MAX - (int64_t)d) / 10LL <= num) ||
		DIV_MAX <= div || DEC_MAX < dec_cnt - zero_cnt) {
		big = 1;
	    } else {
		num = num * 10 + d;
		div *= 10;
	    }
	}
    }
    if ('e' == c || 'E' == c) {
	int	eneg = 0;

	c = reader_get(&pi->err, &pi->rd);
	if ('-' == c) {
	    c = reader_get(&pi->err, &pi->rd);
	    eneg = 1;
	} else if ('+' == c) {
	    c = reader_get(&pi->err, &pi->rd);
	}
	for (; '0' <= c && c <= '9'; c = reader_get(&pi->err, &pi->rd)) {
	    exp = exp * 10 + (c - '0');
	    if (EXP_MAX <= exp) {
		big = 1;
	    }
	}
	if (eneg) {
	    exp = -exp;
	}
    }
    dec_cnt -= zero_cnt;
    if ('\0' != c) {
	reader_backup(&pi->rd);
    }
    if (big) {
	val = ojc_create_number(pi->rd.start, pi->rd.tail - pi->rd.start);
    } else if (1 == div && 0 == exp) { // fixnum
	if (neg) {
	    i = -i;
	}
	val = get_val(pi, OJC_FIXNUM);
	val->fixnum = i;
    } else { // decimal
	double	d = (double)i + (double)num / (double)div;

	if (neg) {
	    d = -d;
	}
	if (0 != exp) {
	    d *= pow(10.0, exp);
	}
	val = get_val(pi, OJC_DECIMAL);
	val->dub = d;
    }
    add_value(pi, val);
    reader_release(&pi->rd);
}

static void
comma(ParseInfo pi) {
    ojcVal	parent = stack_peek(&pi->stack);

    if (0 == parent) {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "unexpected comma");
    } else if (NEXT_ARRAY_COMMA == parent->expect) {
	parent->expect = NEXT_ARRAY_ELEMENT;
    } else if (NEXT_OBJECT_COMMA == parent->expect) {
	parent->expect = NEXT_OBJECT_KEY;
    } else {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "unexpected comma");
    }
}

static void
colon(ParseInfo pi) {
    ojcVal	parent = stack_peek(&pi->stack);

    if (0 != parent && NEXT_OBJECT_COLON == parent->expect) {
	parent->expect = NEXT_OBJECT_VALUE;
    } else {
	ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "unexpected colon");
    }
}

void
ojc_parse(ParseInfo pi) {
    char	c;

    pi->err.code = 0;
    *pi->err.msg = '\0';
    while (1) {
	c = reader_next_non_white(&pi->err, &pi->rd);
	switch (c) {
	case '{':
	    object_start(pi);
	    break;
	case '}':
	    object_end(pi);
	    break;
	case ':':
	    colon(pi);
	    break;
	case '[':
	    array_start(pi);
	    break;
	case ']':
	    array_end(pi);
	    break;
	case ',':
	    comma(pi);
	    break;
	case '"':
	    read_str(pi);
	    break;
	case '+':
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    reader_backup(&pi->rd);
	    read_num(pi);
	    break;
	case 't':
	    read_true(pi);
	    break;
	case 'f':
	    read_false(pi);
	    break;
	case 'n':
	    read_null(pi);
	    break;
	case '/':
	    skip_comment(pi);
	    break;
	case '\0':
	    return;
	default:
	    ojc_set_error_at(pi, OJC_PARSE_ERR, __FILE__, __LINE__, "unexpected character");
	    return;
	}
	if (OJC_OK != pi->err.code) {
	    return;
	}
	if (0 != pi->each_cb && stack_empty(&pi->stack)) {
	    ojcVal	val = stack_head(&pi->stack);

	    if (0 != val) {
		if (pi->each_cb(val, pi->each_ctx)) {
		    struct _List	freed = { 0, 0 };

		    _ojc_val_destroy(val, &freed);
		    if (0 == pi->free_vals.head) {
			pi->free_vals.head = freed.head;
			pi->free_vals.tail = freed.tail;
		    } else {
			freed.tail->next = pi->free_vals.head;
			pi->free_vals.head = freed.head;
		    }
		}
		*pi->stack.head = 0;
	    }
	}
    }
}

void
ojc_set_error_at(ParseInfo pi, ojcErrCode code, const char* file, int line, const char *format, ...) {
    va_list	ap;
    char	msg[128];

    va_start(ap, format);
    vsnprintf(msg, sizeof(msg) - 1, format, ap);
    va_end(ap);
    pi->err.code = code;
    snprintf(pi->err.msg, sizeof(pi->err.msg) - 1, "%s at line %d, column %d [%s:%d]", msg, pi->rd.line, pi->rd.col, file, line);
}
