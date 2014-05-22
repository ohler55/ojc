/* ojc.c
 * Copyright (c) 2011, Peter Ohler
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

#include <unistd.h>
#include <string.h>

#include "ojc.h"
#include "parse.h"
#include "buf.h"
#include "val.h"

#define MAX_INDEX	200000000

void
ojc_destroy(ojcVal val) {
    _ojc_destroy(val);
}

ojcVal
ojc_parse_str(ojcErr err, const char *json, ojcParseCallback cb, void *ctx) {
    struct _ParseInfo	pi;
    ojcVal		val;

    parse_init(&pi.err, &pi, cb, ctx);
    reader_init_str(&pi.err, &pi.rd, json);
    if (OJC_OK != pi.err.code) {
	return 0;
    }
    ojc_parse(&pi);
    val = *pi.stack.head;
    if (OJC_OK != pi.err.code && 0 != err) {
	err->code = pi.err.code;
	memcpy(err->msg, pi.err.msg, sizeof(pi.err.msg));
    }
    parse_cleanup(&pi);

    return val;
}

ojcVal
ojc_parse_stream(ojcErr err, FILE *file, ojcParseCallback cb, void *ctx) {
    struct _ParseInfo	pi;
    ojcVal		val;

    parse_init(&pi.err, &pi, cb, ctx);
    reader_init_stream(err, &pi.rd, file);
    if (OJC_OK != err->code) {
	return 0;
    }
    ojc_parse(&pi);
    val = *pi.stack.head;
    if (OJC_OK != pi.err.code && 0 != err) {
	err->code = pi.err.code;
	memcpy(err->msg, pi.err.msg, sizeof(pi.err.msg));
    }
    parse_cleanup(&pi);

    return val;
}

ojcVal
ojc_parse_socket(ojcErr err, int socket, ojcParseCallback cb, void *ctx) {
    struct _ParseInfo	pi;
    ojcVal		val;

    parse_init(&pi.err, &pi, cb, ctx);
    reader_init_socket(err, &pi.rd, socket);
    if (OJC_OK != err->code) {
	return 0;
    }
    ojc_parse(&pi);
    val = *pi.stack.head;
    if (OJC_OK != pi.err.code && 0 != err) {
	err->code = pi.err.code;
	memcpy(err->msg, pi.err.msg, sizeof(pi.err.msg));
    }
    parse_cleanup(&pi);

    return val;
}

ojcVal
ojc_get(ojcVal val, const char *path) {
    const char	*start;
    ojcVal	m;

    if (0 == path || 0 == val) {
	return val;
    }
    if ('/' == *path) {
	path++;
    }
    if ('\0' == *path) {
	return val;
    }
    start = path;
    switch (val->type) {
    case OJC_ARRAY:
	{
	    unsigned int	index = 0;

	    for (; '\0' != *path && '/' != *path; path++) {
		if (*path < '0' || '9' < *path) {
		    return 0;
		}
		index = index * 10 + (int)(*path - '0');
		if (MAX_INDEX < index) {
		    return 0;
		}
	    }
	    for (m = val->members.head; 0 < index; m = m->next, index--) {
		if (0 == m) {
		    return 0;
		}
	    }
	    return ojc_get(m, path);
	}
	break;
    case OJC_OBJECT:
	{
	    const char	*key;
	    int		plen;

	    for (; '\0' != *path && '/' != *path; path++) {
	    }
	    plen = path - start;
	    for (m = val->members.head; ; m = m->next) {
		if (0 == m) {
		    return 0;
		}
		switch (m->key_type) {
		case STR_PTR:	key = m->key.str;	break;
		case STR_ARRAY:	key = m->key.ca;	break;
		case STR_NONE:
		default:	key = 0;		break;
		}
		if (0 != key && 0 == strncmp(start, key, plen)) {
		    return ojc_get(m, path);
		}
	    }
	}
	break;
    default:
	break;
    }
    return 0;
}

ojcVal
ojc_aget(ojcVal val, const char **path) {
    ojcVal	m;

    if (0 == path || 0 == val || 0 == *path) {
	return val;
    }
    switch (val->type) {
    case OJC_ARRAY:
	{
	    unsigned int	index = 0;
	    const char		*p;

	    for (p = *path; '\0' != *p; p++) {
		if (*p < '0' || '9' < *p) {
		    return 0;
		}
		index = index * 10 + (int)(*p - '0');
		if (MAX_INDEX < index) {
		    return 0;
		}
	    }
	    for (m = val->members.head; 0 < index; m = m->next, index--) {
		if (0 == m) {
		    return 0;
		}
	    }
	    return ojc_aget(m, path + 1);
	}
	break;
    case OJC_OBJECT:
	{
	    const char	*key;

	    for (m = val->members.head; ; m = m->next) {
		if (0 == m) {
		    return 0;
		}
		switch (m->key_type) {
		case STR_PTR:	key = m->key.str;	break;
		case STR_ARRAY:	key = m->key.ca;	break;
		case STR_NONE:
		default:	key = 0;		break;
		}
		if (0 != key && 0 == strcmp(*path, key)) {
		    return ojc_aget(m, path + 1);
		}
	    }
	}
	break;
    default:
	break;
    }
    return 0;
}

ojcValType
ojc_type(ojcVal val) {
    return val->type;
}

int64_t
ojc_int(ojcErr err, ojcVal val) {
    if (OJC_FIXNUM != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not get and int64_t from a %s", ojc_type_str(val->type));
	}
	return 0;
    }
    return val->fixnum;
}

double
ojc_double(ojcErr err, ojcVal val) {
    if (OJC_DECIMAL != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not get and double from a %s", ojc_type_str(val->type));
	}
	return 0;
    }
    return val->dub;
}

ojcVal
ojc_members(ojcErr err, ojcVal val) {
    if (OJC_ARRAY != val->type && OJC_OBJECT != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not get members from a %s", ojc_type_str(val->type));
	}
	return 0;
    }
    return val->members.head;
}

ojcVal
ojc_next(ojcVal val) {
    return val->next;
}

int
ojc_member_count(ojcErr err, ojcVal val) {
    ojcVal	m;
    int		cnt = 0;

    if (OJC_ARRAY != val->type && OJC_OBJECT != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "No members in a %s", ojc_type_str(val->type));
	}
	return 0;
    }
    for (m = val->members.head; 0 != m; m = m->next) {
	cnt++;
    }
    return cnt;
}

bool
ojc_has_key(ojcVal val) {
    return STR_NONE != val->key_type;
}

const char*
ojc_key(ojcVal val) {
    switch (val->key_type) {
    case STR_PTR:	return val->key.str;
    case STR_ARRAY:	return val->key.ca;
    case STR_NONE:
    default:		return 0;
    }
}

ojcVal
ojc_create_object() {
    return _ojc_val_create(OJC_OBJECT);
}

ojcVal
ojc_create_array() {
    return _ojc_val_create(OJC_ARRAY);
}

ojcVal
ojc_create_str(const char *str, size_t len) {
    ojcVal	val = _ojc_val_create(OJC_STRING);
    
    if (0 >= len) {
	len = strlen(str);
    }
    if (sizeof(val->str.ca) - 1 <= len) {
	val->str_type = STR_PTR;
	val->str.str = strndup(str, len);
	val->str.len = len;
    } else {
	val->str_type = STR_ARRAY;
	*val->str.ca = (char)len;
	strncpy(val->str.ca + 1, str, len);
    }
    return val;
}

ojcVal
ojc_create_int(int64_t num) {
    ojcVal	val = _ojc_val_create(OJC_FIXNUM);

    val->fixnum = num;

    return val;
}

ojcVal
ojc_create_double(double num) {
    ojcVal	val = _ojc_val_create(OJC_DECIMAL);

    val->dub = num;

    return val;
}

ojcVal
ojc_create_number(const char *num, size_t len) {
    ojcVal	val = _ojc_val_create(OJC_NUMBER);

    if (0 >= len) {
	len = strlen(num);
    }
    if (sizeof(val->str.ca) - 1 <= len) {
	val->str_type = STR_PTR;
	val->str.str = strndup(num, len);
	val->str.len = len;
    } else {
	val->str_type = STR_ARRAY;
	*val->str.ca = (char)len;
	strncpy(val->str.ca + 1, num, len);
    }
    return val;
}

ojcVal
ojc_create_null(void) {
    return _ojc_val_create(OJC_NULL);
}

ojcVal
ojc_create_bool(bool boo) {
    return _ojc_val_create(boo ? OJC_TRUE : OJC_FALSE);
}

void
ojc_object_nappend(ojcErr err, ojcVal object, const char *key, int klen, ojcVal val) {
    if (OJC_OBJECT != object->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not object append to a %s", ojc_type_str(object->type));
	}
	return;
    }
    val->next = 0;
    if (sizeof(val->key.ca) <= klen) {
	val->key_type = STR_PTR;
	val->key.str = strdup(key);
	val->key.len = klen;
    } else {
	val->key_type = STR_ARRAY;
	strcpy(val->key.ca, key);
    }
    if (0 == object->members.head) {
	object->members.head = val;
    } else {
	object->members.tail->next = val;
    }
    object->members.tail = val;
}

void
ojc_object_append(ojcErr err, ojcVal object, const char *key, ojcVal val) {
    return ojc_object_nappend(err, object, key, strlen(key), val);
}

void
ojc_array_append(ojcErr err, ojcVal array, ojcVal val) {
    if (OJC_ARRAY != array->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not array append to a %s", ojc_type_str(array->type));
	}
	return;
    }
    val->next = 0;
    if (0 == array->members.head) {
	array->members.head = val;
    } else {
	array->members.tail->next = val;
    }
    array->members.tail = val;
}

static void
fixnum_fill(Buf bb, int64_t num) {
    char	buf[32];
    char	*end = buf + sizeof(buf) - 1;
    char	*b = end;
    int		neg = 0;

    if (0 > num) {
	neg = 1;
	num = -num;
    }
    *b-- = '\0';
    if (0 < num) {
	for (; 0 < num; num /= 10, b--) {
	    *b = (num % 10) + '0';
	}
	if (neg) {
	    *b = '-';
	} else {
	    b++;
	}
    } else {
	*b = '0';
    }
    buf_append_string(bb, b, end - b);
}

static void
fill_buf(Buf buf, ojcVal val, int indent, int depth) {
    if (0 == val) {
	return;
    }
    switch (val->type) {
    case OJC_ARRAY:
	{
	    ojcVal	m;
	    int		d2 = depth + 1;
	    size_t	icnt = indent * d2;
	    char	in[256];

	    if (0 < indent) {
		if (sizeof(in) <= icnt - 2) {
		    icnt = sizeof(in) - 2;
		}
		*in = '\n';
		memset(in + 1, ' ', icnt);
		icnt++;
		in[icnt] = '\0';
	    }
	    buf_append(buf, '[');
	    for (m = val->members.head; 0 != m; m = m->next) {
		if (m != val->members.head) {
		    buf_append(buf, ',');
		}
		if (0 < indent) {
		    buf_append_string(buf, in, icnt);
		}
		fill_buf(buf, m, indent, d2);
		if (buf->overflow) {
		    break;
		}
	    }
	    if (0 < indent) {
		buf_append_string(buf, in, icnt - indent);
	    }
	    buf_append(buf, ']');
	}
	break;
    case OJC_OBJECT:
	{
	    ojcVal	m;
	    int		d2 = depth + 1;
	    size_t	icnt = indent * d2;
	    char	in[256];

	    if (0 < indent) {
		if (sizeof(in) <= icnt - 2) {
		    icnt = sizeof(in) - 2;
		}
		*in = '\n';
		memset(in + 1, ' ', icnt);
		icnt++;
		in[icnt] = '\0';
	    }
	    buf_append(buf, '{');
	    for (m = val->members.head; 0 != m; m = m->next) {
		if (m != val->members.head) {
		    buf_append(buf, ',');
		}
		if (0 < indent) {
		    buf_append_string(buf, in, icnt);
		}
		buf_append(buf, '"');
		if (STR_PTR == m->key_type) {
		    buf_append_string(buf, m->key.str, m->key.len);
		} else {
		    buf_append_string(buf, m->key.ca, strlen(m->key.ca));
		}
		buf_append(buf, '"');
		buf_append(buf, ':');
		fill_buf(buf, m, indent, d2);
		if (buf->overflow) {
		    break;
		}
	    }
	    if (0 < indent) {
		buf_append_string(buf, in, icnt - indent);
	    }
	    buf_append(buf, '}');
	}
	break;
    case OJC_NULL:
	buf_append_string(buf, "null", 4);
	break;
    case OJC_TRUE:
	buf_append_string(buf, "true", 4);
	break;
    case OJC_FALSE:
	buf_append_string(buf, "false", 5);
	break;
    case OJC_STRING:
	buf_append(buf, '"');
	if (STR_PTR == val->str_type) {
	    buf_append_string(buf, val->str.str, val->str.len);
	} else {
	    buf_append_string(buf, val->str.ca + 1, *val->str.ca);
	}
	buf_append(buf, '"');
	break;
    case OJC_FIXNUM:
	fixnum_fill(buf, val->fixnum);
	break;
    case OJC_DECIMAL:
	{
	    char	str[64];
	    int		cnt;

	    if (val->dub == (double)(int64_t)val->dub) {
		cnt = snprintf(str, sizeof(str) - 1, "%.1f", val->dub);
	    } else {
		cnt = snprintf(str, sizeof(str) - 1, "%0.15g", val->dub);
	    }
	    buf_append_string(buf, str, cnt);
	}
	break;
    case OJC_NUMBER:
	if (STR_PTR == val->str_type) {
	    buf_append_string(buf, val->str.str, val->str.len);
	} else {
	    buf_append_string(buf, val->str.ca + 1, *val->str.ca);
	}
	break;
    default:
	break;
    }
}

char*
ojc_to_str(ojcVal val, int indent) {
    struct _Buf	b;
    
    buf_init(&b);
    fill_buf(&b, val, indent, 0);
    *b.tail = '\0';
    if (b.base == b.head) {
	return strdup(b.head);
    }
    return b.head;
}

int
ojc_fill(ojcErr err, ojcVal val, int indent, char *buf, size_t len) {
    struct _Buf	b;
    
    buf_finit(&b, buf, len);
    fill_buf(&b, val, indent, 0);
    if (b.overflow) {
	if (0 != err) {
	    err->code = OJC_OVERFLOW_ERR;
	    strcpy(err->msg, "buffer not large enough for output");
	}
	return -1;
    }
    *b.tail = '\0';
    return buf_len(&b);
}

static void
write_fixnum(int fd, int64_t num) {
    char	buf[32];
    char	*end = buf + sizeof(buf) - 1;
    char	*b = end;
    int		neg = 0;

    if (0 > num) {
	neg = 1;
	num = -num;
    }
    *b-- = '\0';
    if (0 < num) {
	for (; 0 < num; num /= 10, b--) {
	    *b = (num % 10) + '0';
	}
	if (neg) {
	    *b = '-';
	} else {
	    b++;
	}
    } else {
	*b = '0';
    }
    write(fd, b, end - b);
}


static void
write_val(int fd, ojcVal val, int indent, int depth) {
    if (0 == val) {
	return;
    }
    switch (val->type) {
    case OJC_ARRAY:
	{
	    ojcVal	m;
	    int		d2 = depth + 1;
	    size_t	icnt = indent * d2;
	    char	in[256];

	    if (0 < indent) {
		if (sizeof(in) <= icnt - 2) {
		    icnt = sizeof(in) - 2;
		}
		*in = '\n';
		memset(in + 1, ' ', icnt);
		icnt++;
		in[icnt] = '\0';
	    }
	    write(fd, "[", 1);
	    for (m = val->members.head; 0 != m; m = m->next) {
		if (m != val->members.head) {
		    write(fd, ",", 1);
		}
		if (0 < indent) {
		    write(fd, in, icnt);
		}
		write_val(fd, m, indent, d2);
	    }
	    if (0 < indent) {
		write(fd, in, icnt - indent);
	    }
	    write(fd, "]", 1);
	}
	break;
    case OJC_OBJECT:
	{
	    ojcVal	m;
	    int		d2 = depth + 1;
	    size_t	icnt = indent * d2;
	    char	in[256];

	    if (0 < indent) {
		if (sizeof(in) <= icnt - 2) {
		    icnt = sizeof(in) - 2;
		}
		*in = '\n';
		memset(in + 1, ' ', icnt);
		icnt++;
		in[icnt] = '\0';
	    }
	    write(fd, "{", 1);
	    for (m = val->members.head; 0 != m; m = m->next) {
		if (m != val->members.head) {
		    write(fd, ",", 1);
		}
		if (0 < indent) {
		    write(fd, in, icnt);
		}
		write(fd, "\"", 1);
		if (STR_PTR == m->key_type) {
		    write(fd, m->key.str, m->key.len);
		} else {
		    write(fd, m->key.ca, strlen(m->key.ca));
		}
		write(fd, "\":", 2);
		write_val(fd, m, indent, d2);
	    }
	    if (0 < indent) {
		write(fd, in, icnt - indent);
	    }
	    write(fd, "}", 1);
	}
	break;
    case OJC_NULL:
	write(fd, "null", 4);
	break;
    case OJC_TRUE:
	write(fd, "true", 4);
	break;
    case OJC_FALSE:
	write(fd, "false", 5);
	break;
    case OJC_STRING:
	write(fd, "\"", 1);
	if (STR_PTR == val->str_type) {
	    write(fd, val->str.str, val->str.len);
	} else {
	    write(fd, val->str.ca + 1, *val->str.ca);
	}
	write(fd, "\"", 1);
	break;
    case OJC_FIXNUM:
	write_fixnum(fd, val->fixnum);
	break;
    case OJC_DECIMAL:
	{
	    char	str[64];
	    int		cnt;

	    if (val->dub == (double)(int64_t)val->dub) {
		cnt = snprintf(str, sizeof(str) - 1, "%.1f", val->dub);
	    } else {
		cnt = snprintf(str, sizeof(str) - 1, "%0.15g", val->dub);
	    }
	    write(fd, str, cnt);
	}
	break;
    case OJC_NUMBER:
	if (STR_PTR == val->str_type) {
	    write(fd, val->str.str, val->str.len);
	} else {
	    write(fd, val->str.ca + 1, *val->str.ca);
	}
	break;
    default:
	break;
    }
}

void
ojc_write(ojcErr err, ojcVal val, int indent, int socket) {
    write_val(socket, val, indent, 0);
}

void
ojc_fwrite(ojcErr err, ojcVal val, int indent, FILE *file) {
    ojc_write(err, val, indent, fileno(file));
}

const char*
ojc_type_str(ojcValType type) {
    switch (type) {
    case OJC_NULL:	return "null";
    case OJC_STRING:	return "string";
    case OJC_NUMBER:	return "number";
    case OJC_FIXNUM:	return "fixnum";
    case OJC_DECIMAL:	return "decimal";
    case OJC_TRUE:	return "true";
    case OJC_FALSE:	return "false";
    case OJC_ARRAY:	return "array";
    case OJC_OBJECT:	return "object";
    default:		return "unknown";
    }
}
