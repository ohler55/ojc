/* wire.c
 * Copyright (c) 2017, Peter Ohler
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

#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buf.h"
#include "wire.h"
#include "val.h"

#define NO_TIME		0xffffffffffffffffULL
#define WIRE_INC	4096
#define MAX_STACK_BUF	4096
#define MIN_WIRE_BUF	1024
#define TIME_STR_LEN	30
#define UUID_STR_LEN	36

typedef enum {
    WIRE_NULL	= (uint8_t)'Z',
    WIRE_TRUE	= (uint8_t)'t',
    WIRE_FALSE	= (uint8_t)'f',
    WIRE_INT1	= (uint8_t)'i',
    WIRE_INT2	= (uint8_t)'2',
    WIRE_INT4	= (uint8_t)'4',
    WIRE_INT8	= (uint8_t)'8',
    WIRE_STR1	= (uint8_t)'s',
    WIRE_STR2	= (uint8_t)'S',
    WIRE_STR4	= (uint8_t)'B',
    WIRE_KEY1	= (uint8_t)'k',
    WIRE_KEY2	= (uint8_t)'K',
    WIRE_KEY4	= (uint8_t)'X',
    WIRE_DEC	= (uint8_t)'d',
    WIRE_NUM1	= (uint8_t)'b',
    WIRE_NUM2	= (uint8_t)'n',
    WIRE_NUM4	= (uint8_t)'N',
    WIRE_UUID	= (uint8_t)'u',
    WIRE_TIME	= (uint8_t)'T',
    WIRE_OBEG	= (uint8_t)'{',
    WIRE_OEND	= (uint8_t)'}',
    WIRE_ABEG	= (uint8_t)'[',
    WIRE_AEND	= (uint8_t)']',
} ojcWireTag;

typedef struct _ParseCtx {
    ojcVal	top;
    ojcVal	stack[WIRE_STACK_SIZE];
    ojcVal	*cur;
    ojcVal	*end;
    const char	*key;
    int		klen;
} *ParseCtx;

static const char	uuid_char_map[257] = "\
................................\
.............-..hhhhhhhhhh......\
................................\
.hhhhhh.........................\
................................\
................................\
................................\
................................";

static const char	uuid_map[37] = "hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh";

static int
size_bytes(int64_t n) {
    int	cnt = 0;
    
    if (-128 <= n && n <= 127) {
	cnt = 1;
    } else if (-32768 <= n && n <= 32767) {
	cnt = 2;
    } else if (-2147483648 <= n && n <= 2147483647) {
	cnt = 4;
    } else {
	cnt = 8;
    }
    return cnt;
}

static const uint8_t*
read_uint16(const uint8_t *b, uint16_t *nump) {
    uint16_t	num = (uint16_t)*b++;

    num = (num << 8) | (uint16_t)*b++;
    *nump = num;
    
    return b;
}

static const uint8_t*
read_uint32(const uint8_t *b, uint32_t *nump) {
    const uint8_t	*end = b + 4;
    uint32_t		num = 0;

    for (; b < end; b++) {
	num = (num << 8) | (uint32_t)*b;
    }
    *nump = num;
    
    return b;
}

static const uint8_t*
read_uint64(const uint8_t *b, uint64_t *nump) {
    const uint8_t	*end = b + 8;
    uint64_t		num = 0;

    for (; b < end; b++) {
	num = (num << 8) | (uint64_t)*b;
    }
    *nump = num;
    
    return b;
}


static uint8_t*
fill_uint16(uint8_t *b, uint16_t n) {
    *b++ = (uint8_t)(n >> 8);
    *b++ = (uint8_t)(0x00ff & n);
    return b;
}

static uint8_t*
fill_uint32(uint8_t *b, uint32_t n) {
    *b++ = (uint8_t)(n >> 24);
    *b++ = (uint8_t)(0x000000ff & (n >> 16));
    *b++ = (uint8_t)(0x000000ff & (n >> 8));
    *b++ = (uint8_t)(0x000000ff & n);
    return b;
}

static uint8_t*
fill_uint64(uint8_t *b, uint64_t n) {
    *b++ = (uint8_t)(n >> 56);
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 48));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 40));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 32));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 24));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 16));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 8));
    *b++ = (uint8_t)(0x00000000000000ff & n);
    return b;
}

static uint8_t*
fill_str(uint8_t *w, const char *str, size_t len) {
    if (len <= 127) {
	*w++ = WIRE_STR1;
	*w++ = (uint8_t)len;
    } else if (len <= 32767) {
	*w++ = WIRE_STR2;
	w = fill_uint16(w, (uint16_t)len);
    } else {
	*w++ = WIRE_STR4;
	w = fill_uint32(w, (uint32_t)len);
    }
    memcpy(w, str, len);
    w += len;

    return w;
}

static uint8_t*
fill_key(uint8_t *w, const char *str, size_t len) {
    if (len <= 127) {
	*w++ = WIRE_KEY1;
	*w++ = (uint8_t)len;
    } else if (len <= 32767) {
	*w++ = WIRE_KEY2;
	w = fill_uint16(w, (uint16_t)len);
    } else {
	*w++ = WIRE_KEY4;
	w = fill_uint32(w, (uint32_t)len);
    }
    memcpy(w, str, len);
    w += len;

    return w;
}

static uint8_t*
fill_num(uint8_t *w, const char *str, size_t len) {
    if (len <= 127) {
	*w++ = WIRE_NUM1;
	*w++ = (uint8_t)len;
    } else if (len <= 32767) {
	*w++ = WIRE_NUM2;
	w = fill_uint16(w, (uint16_t)len);
    } else {
	*w++ = WIRE_NUM4;
	w = fill_uint32(w, (uint32_t)len);
    }
    memcpy(w, str, len);
    w += len;

    return w;
}

static uint8_t*
fill_uuid(uint8_t *w, const char *str) {
    const char	*s = str;
    const char	*end = str + UUID_STR_LEN;
    int		digits = 0;
    uint8_t	b = 0;
	    
    *w++ = WIRE_UUID;
    for (; s < end; s++) {
	if ('0' <= *s && *s <= '9') {
	    b = (b << 4) | (*s - '0');
	} else if ('a' <= *s && *s <= 'f') {
	    b = (b << 4) | (*s - 'a' + 10);
	} else {
	    continue;
	}
	if (0 == digits) {
	    digits++;
	} else {
	    digits = 0;
	    *w++ = b;
	    b = 0;
	}
    }
    return w;
}

static bool
detect_uuid(const char *str, int len) {
    if (UUID_STR_LEN == len) {
	const char	*u = uuid_map;

	for (; 0 < len; len--, str++, u++) {
	    if (*u != uuid_char_map[*(uint8_t*)str]) {
		return false;
	    }
	}
	return true;
    }
    return false;
}

static const char*
read_num(const char *s, int len, int *vp) {
    uint32_t	v = 0;

    for (; 0 < len; len--, s++) {
	if ('0' <= *s && *s <= '9') {
	    v = v * 10 + *s - '0';
	} else {
	    return NULL;
	}
    }
    *vp = (int)v;

    return s;
}

static uint64_t
time_parse(const char *s, int len) {
    struct tm	tm;
    long	nsecs = 0;
    int		i;
    time_t	secs;
    
    memset(&tm, 0, sizeof(tm));
    if (NULL == (s = read_num(s, 4, &tm.tm_year))) {
	return NO_TIME;
    }
    tm.tm_year -= 1900;
    if ('-' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_mon))) {
	return NO_TIME;
    }
    tm.tm_mon--;
    if ('-' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_mday))) {
	return NO_TIME;
    }
    if ('T' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_hour))) {
	return NO_TIME;
    }
    if (':' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_min))) {
	return NO_TIME;
    }
    if (':' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_sec))) {
	return NO_TIME;
    }
    if ('.' != *s) {
	return NO_TIME;
    }
    s++;
    for (i = 9; 0 < i; i--, s++) {
	if ('0' <= *s && *s <= '9') {
	    nsecs = nsecs * 10 + *s - '0';
	} else {
	    return NO_TIME;
	}
    }
    if ('Z' != *s) {
	return NO_TIME;
    }
    secs = (time_t)timegm(&tm);

    return (uint64_t)secs * 1000000000ULL + (uint64_t)nsecs;
}

static int
wire_size(ojcVal val) {
    int64_t	size = 1;
    int64_t	len;

    switch (val->type) {
    case OJC_NULL:
    case OJC_TRUE:
    case OJC_FALSE:
	break;
    case OJC_STRING:
	if (UUID_STR_LEN == val->str_len && detect_uuid(val->str.bstr->ca, UUID_STR_LEN)) {
	    size += 16;
	} else if (TIME_STR_LEN == val->str_len && NO_TIME != time_parse(val->str.bstr->ca, TIME_STR_LEN)) {
	    size += 8;
	} else {
	    size += (int64_t)val->str_len + size_bytes((int64_t)val->str_len);
	}
	break;
    case OJC_NUMBER:
	size += (int64_t)val->str_len + size_bytes((int64_t)val->str_len);
	break;
    case OJC_FIXNUM:
	size += size_bytes(val->fixnum);
	break;
    case OJC_DECIMAL: {
	char	str[64];

	size++;
	if (val->dub == (double)(int64_t)val->dub) {
	    size += snprintf(str, sizeof(str) - 1, "%.1f", val->dub);
	} else {
	    size += snprintf(str, sizeof(str) - 1, "%0.15g", val->dub);
	}
	break;
    }
    case OJC_ARRAY:
	size++;
	for (ojcVal m = val->members.head; NULL != m; m = m->next) {
	    size += wire_size(m);
	}
	break;
    case OJC_OBJECT:
	size++;
	for (ojcVal m = val->members.head; NULL != m; m = m->next) {
	    if (KEY_BIG == (len = m->key_len)) {
		len = strlen(m->key.str);
	    } else if (KEY_NONE == len) {
		len = 0;
	    }
	    size += len + size_bytes(len) + 1;
	    size += wire_size(m);
	}
	break;
    case OJC_WORD:
	len = strlen(val->str.ca);
	size += len + size_bytes(len);
	break;
    default:
	break;
    }
    return (int)size;
}

static uint8_t*
wire_fill(ojcVal val, uint8_t *w) {
    switch (val->type) {
    case OJC_NULL:
	*w++ = WIRE_NULL;
	break;
    case OJC_TRUE:
	*w++ = WIRE_TRUE;
	break;
    case OJC_FALSE:
	*w++ = WIRE_FALSE;
	break;
    case OJC_STRING: {
	const char	*str;
	uint64_t	t;

	if ((int)sizeof(union _Bstr) <= val->str_len) {
	    str = val->str.str;
	} else if ((int)sizeof(val->str.ca) <= val->str_len) {
	    str = val->str.bstr->ca;
	} else {
	    str = val->str.ca;
	}
	if (UUID_STR_LEN == val->str_len && detect_uuid(val->str.bstr->ca, UUID_STR_LEN)) {
	    w = fill_uuid(w, str);
	} else if (TIME_STR_LEN == val->str_len && NO_TIME != (t = time_parse(val->str.bstr->ca, TIME_STR_LEN))) {
	    *w++ = WIRE_TIME;
	    w = fill_uint64(w, t);
	} else {
	    w = fill_str(w, str, val->str_len);
	}
	break;
    }
    case OJC_NUMBER:
	if ((int)sizeof(union _Bstr) <= val->str_len) {
	    w = fill_num(w, val->str.str, val->str_len);
	} else if ((int)sizeof(val->str.ca) <= val->str_len) {
	    w = fill_num(w, val->str.bstr->ca, val->str_len);
	} else {
	    w = fill_num(w, val->str.ca, val->str_len);
	}
	break;
    case OJC_FIXNUM:
	if (-128 <= val->fixnum && val->fixnum <= 127) {
	    *w++ = WIRE_INT1;
	    *w++ = (uint8_t)(int8_t)val->fixnum;
	} else if (-32768 <= val->fixnum && val->fixnum <= 32767) {
	    *w++ = WIRE_INT2;
	    w = fill_uint16(w, (uint16_t)(int16_t)val->fixnum);
	} else if (-2147483648 <= val->fixnum && val->fixnum <= 2147483647) {
	    *w++ = WIRE_INT4;
	    w = fill_uint32(w, (uint32_t)(int32_t)val->fixnum);
	} else {
	    *w++ = WIRE_INT8;
	    w = fill_uint64(w, (uint64_t)val->fixnum);
	}
	break;
    case OJC_DECIMAL: {
	char	str[64];
	int	cnt;

	*w++ = WIRE_DEC;
	if (val->dub == (double)(int64_t)val->dub) {
	    cnt = snprintf(str, sizeof(str) - 1, "%.1f", val->dub);
	} else {
	    cnt = snprintf(str, sizeof(str) - 1, "%0.15g", val->dub);
	}
	*w++ = (uint8_t)cnt;
	memcpy(w, str, cnt);
	w += cnt;
	break;
    }
    case OJC_ARRAY:
	*w++ = WIRE_ABEG;
	for (ojcVal m = val->members.head; NULL != m; m = m->next) {
	    w = wire_fill(m, w);
	}
	*w++ = WIRE_AEND;
	break;
    case OJC_OBJECT:
	*w++ = WIRE_OBEG;
	for (ojcVal m = val->members.head; NULL != m; m = m->next) {
	    if ((int)sizeof(union _Bstr) <= m->key_len) {
		w = fill_key(w, m->key.str, m->key_len);
	    } else if ((int)sizeof(m->key.ca) <= m->key_len) {
		w = fill_key(w, m->key.bstr->ca, m->key_len);
	    } else {
		w = fill_key(w, m->key.ca, m->key_len);
	    }
	    w = wire_fill(m, w);
	}
	*w++ = WIRE_OEND;
	break;
    case OJC_WORD:
	break;
    default:
	break;
    }
    return w;
}

int
ojc_wire_size(ojcVal val) {
    if (NULL == val) {
	return 0;
    }
    return wire_size(val) + 4;
}

size_t
ojc_wire_buf_size(const uint8_t *wire) {
    size_t	size = 0;

    for (int i = 4; 0 < i; i--, wire++) {
	size = (size << 8) + (size_t)*wire;
    }
    return size + 4;
}

uint8_t*
ojc_wire_write_mem(ojcErr err, ojcVal val) {
    if (NULL == val) {
	err->code = OJC_ARG_ERR;
	snprintf(err->msg, sizeof(err->msg), "NULL value");
	return NULL;
    }
    size_t	size = (size_t)wire_size(val);
    uint8_t	*wire = (uint8_t*)malloc(size + 4);

    if (NULL == wire) {
	err->code = OJC_MEMORY_ERR;
	snprintf(err->msg, sizeof(err->msg), "memory allocation failed for size %lu", (unsigned long)size );
	return NULL;
    }
    if ((int64_t)0xffffffffU < size) {
	err->code = OJC_ARG_ERR;
	snprintf(err->msg, sizeof(err->msg), "too large (%lu bytes), limit is %d bytes", (unsigned long)size, (int)0xffffffffU);
	return NULL;
    }
    ojc_wire_fill(val, wire, size + 4);

    return wire;
}

size_t
ojc_wire_fill(ojcVal val, uint8_t *wire, size_t size) {
    if (0 == size) {
	size = wire_size(val);
	if (0xffffffffU < size) {
	    return 0;
	}
    } else if (NULL == val) {
	return 0;
    } else {
	size -= 4;
    }
    wire_fill(val, fill_uint32(wire, (uint32_t)size));

    return size + 4;
}

int
ojc_wire_write_file(ojcErr err, ojcVal val, FILE *file) {
    return ojc_wire_write_fd(err, val, fileno(file));
}

int
ojc_wire_write_fd(ojcErr err, ojcVal val, int fd) {
    int		size = wire_size(val) + 4;

    if (MAX_STACK_BUF <= size) {
	uint8_t	*buf = (uint8_t*)malloc(size);

	ojc_wire_fill(val, buf, size);
	if (size != write(fd, buf, size)) {
	    err->code = OJC_WRITE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "write failed: %s", strerror(errno));
	}
	free(buf);
    } else {
	uint8_t	buf[MAX_STACK_BUF];

	ojc_wire_fill(val, buf, size);
	if (size != write(fd, buf, size)) {
	    err->code = OJC_WRITE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "write failed: %s", strerror(errno));
	}
    }
    return err->code;
}

///// parse

static int
push_parse_value(ojcErr err, ParseCtx pc, ojcVal val) {
    if (pc->stack <= pc->cur) {
	ojcVal	parent = *pc->cur;
	
	if (OJC_OBJECT == parent->type) {
	    if (NULL == pc->key) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "missing key");
		return err->code;
	    }
	    ojc_object_nappend(err, parent, pc->key, pc->klen, val);
	    pc->key = NULL;
	    pc->klen = 0;
	} else { // array
	    ojc_array_append(err, parent, val);
	}
    } else {
	pc->top = val;
    }
    return OJC_OK;
}

// Did not use the callbacks as that adds 10% to 20% over this more direct
// approach.
ojcVal
ojc_wire_parse(ojcErr err, const uint8_t *wire) {
    const uint8_t	*end = wire + ojc_wire_buf_size(wire);
    struct _ParseCtx	ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.top = NULL;
    ctx.cur = ctx.stack - 1;
    ctx.end = ctx.stack + WIRE_STACK_SIZE;
    
    wire += 4;
    while (wire < end) {
	switch (*wire++) {
	case WIRE_NULL:
	    push_parse_value(err, &ctx, ojc_create_null());
	    break;
	case WIRE_TRUE:
	    push_parse_value(err, &ctx, ojc_create_bool(true));
	    break;
	case WIRE_FALSE:
	    push_parse_value(err, &ctx, ojc_create_bool(false));
	    break;
	case WIRE_INT1:
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int8_t)*wire));
	    wire++;
	    break;
	case WIRE_INT2: {
	    uint16_t	num;;

	    wire = read_uint16(wire, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int16_t)num));
	    break;
	}
	case WIRE_INT4: {
	    uint32_t	num;;

	    wire = read_uint32(wire, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int32_t)num));
	    break;
	}
	case WIRE_INT8: {
	    uint64_t	num;

	    wire = read_uint64(wire, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)num));
	    break;
	}
	case WIRE_STR1: {
	    uint8_t	len = *wire++;

	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_STR2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_STR4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_KEY1: {
	    uint8_t	len = *wire++;

	    ctx.key = (const char*)wire;
	    ctx.klen = len;
	    wire += len;
	    break;
	}
	case WIRE_KEY2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    ctx.key = (const char*)wire;
	    ctx.klen = len;
	    wire += len;
	    break;
	}
	case WIRE_KEY4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    ctx.key = (const char*)wire;
	    ctx.klen = len;
	    wire += len;
	    break;
	}
	case WIRE_DEC: {
	    uint8_t	len = *wire++;
	    char	buf[256];
	    double	d;
		
	    memcpy(buf, wire, (size_t)len);
	    buf[len] = '\0';
	    d = strtod(buf, NULL);
	    push_parse_value(err, &ctx, ojc_create_double(d));
	    wire += len;
	    break;
	}
	case WIRE_NUM1: {
	    uint8_t	len = *wire++;

	    push_parse_value(err, &ctx, ojc_create_number((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_NUM2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_NUM4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_UUID: {
	    uint64_t	hi;
	    uint64_t	lo;
	    char	buf[40];
	    
	    wire = read_uint64(wire, &hi);
	    wire = read_uint64(wire, &lo);
	    sprintf(buf, "%08lx-%04lx-%04lx-%04lx-%012lx",
		    (unsigned long)(hi >> 32),
		    (unsigned long)((hi >> 16) & 0x000000000000FFFFUL),
		    (unsigned long)(hi & 0x000000000000FFFFUL),
		    (unsigned long)(lo >> 48),
		    (unsigned long)(lo & 0x0000FFFFFFFFFFFFUL));

	    push_parse_value(err, &ctx, ojc_create_str(buf, UUID_STR_LEN));
	    break;
	}
	case WIRE_TIME: {
	    uint64_t	nsec;
	    
	    wire = read_uint64(wire, &nsec);
	    char	buf[64];
	    struct tm	tm;
	    time_t	t = (time_t)(nsec / 1000000000LL);
	    long	frac = nsec - (int64_t)t * 1000000000LL;

	    if (NULL == gmtime_r(&t, &tm)) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "invalid time");
		break;
	    }
	    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ",
		    1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
		    tm.tm_hour, tm.tm_min, tm.tm_sec, frac);
	    push_parse_value(err, &ctx, ojc_create_str(buf, TIME_STR_LEN));
	    break;
	}
	case WIRE_OBEG:
	    if (ctx.end - 1 <= ctx.cur) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "too deeply nested");
	    } else {
		ojcVal	obj = _ojc_val_create(OJC_OBJECT);

		if (OJC_OK == push_parse_value(err, &ctx, obj)) {
		    ctx.cur++;
		    *ctx.cur = obj;
		}
	    }
	    break;
	case WIRE_ABEG:
	    if (ctx.end - 1 <= ctx.cur) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "too deeply nested");
	    } else {
		ojcVal	array = _ojc_val_create(OJC_ARRAY);

		if (OJC_OK == push_parse_value(err, &ctx, array)) {
		    ctx.cur++;
		    *ctx.cur = array;
		}
	    }
	    break;
	case WIRE_OEND:
	case WIRE_AEND:
	    if (ctx.cur < ctx.stack) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "too many closes");
	    } else {
		ctx.cur--;
	    }
	    break;
	default:
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "corrupt wire format");
	    break;
	}
	if (OJC_OK != err->code) {
	    break;
	}
    }
    return ctx.top;
}

ojcVal
ojc_wire_parse_file(ojcErr err, FILE *file) {
    return ojc_wire_parse_fd(err, fileno(file));
}

ojcVal
ojc_wire_parse_fd(ojcErr err, int fd) {
    off_t	here = lseek(fd, 0, SEEK_CUR);
    off_t	size = lseek(fd, 0, SEEK_END) - here;
    ojcVal	val = NULL;
    
    lseek(fd, here, SEEK_SET);
    if (MAX_STACK_BUF <= size) {
	uint8_t	*buf = (uint8_t*)malloc(size);

	if (size != read(fd, buf, size)) {
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "read failed: %s", strerror(errno));
	}
	val = ojc_wire_parse(err, buf);
	free(buf);
    } else {
	uint8_t	buf[MAX_STACK_BUF];

	if (size != read(fd, buf, size)) {
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "read failed: %s", strerror(errno));
	}
	val = ojc_wire_parse(err, buf);
    }
    return val;
}

int
ojc_wire_cbparse(ojcErr err, const uint8_t *wire, ojcWireCallbacks callbacks, void *ctx) {
    const uint8_t	*end = wire + ojc_wire_buf_size(wire);
    
    wire += 4;
    while (wire < end) {
	switch (*wire++) {
	case WIRE_NULL:
	    if (NULL != callbacks->null) {
		callbacks->null(err, ctx);
	    }
	    break;
	case WIRE_TRUE:
	    if (NULL != callbacks->boolean) {
		callbacks->boolean(err, true, ctx);
	    }
	    break;
	case WIRE_FALSE:
	    if (NULL != callbacks->boolean) {
		callbacks->boolean(err, false, ctx);
	    }
	    break;
	case WIRE_INT1:
	    if (NULL != callbacks->fixnum) {
		callbacks->fixnum(err, (int64_t)(int8_t)*wire, ctx);
	    }
	    wire++;
	    break;
	case WIRE_INT2:
	    if (NULL != callbacks->fixnum) {
		uint16_t	num;;

		wire = read_uint16(wire, &num);
		callbacks->fixnum(err, (int64_t)(int16_t)num, ctx);
	    } else {
		wire += 2;
	    }
	    break;
	case WIRE_INT4:
	    if (NULL != callbacks->fixnum) {
		uint32_t	num;;

		wire = read_uint32(wire, &num);
		callbacks->fixnum(err, (int64_t)(int32_t)num, ctx);
	    } else {
		wire += 4;
	    }
	    break;
	case WIRE_INT8:
	    if (NULL != callbacks->fixnum) {
		uint64_t	num;

		wire = read_uint64(wire, &num);
		callbacks->fixnum(err, (int64_t)num, ctx);
	    } else {
		wire += 8;
	    }
	    break;
	case WIRE_STR1: {
	    uint8_t	len = *wire++;

	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_STR2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_STR4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_KEY1: {
	    uint8_t	len = *wire++;

	    if (NULL != callbacks->key) {
		callbacks->key(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_KEY2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    if (NULL != callbacks->key) {
		callbacks->key(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_KEY4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    if (NULL != callbacks->key) {
		callbacks->key(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_DEC: {
	    uint8_t	len = *wire++;

	    if (NULL != callbacks->decimal) {
		char	buf[256];
		double	d;
		
		memcpy(buf, wire, (size_t)len);
		buf[len] = '\0';
		d = strtod(buf, NULL);
		callbacks->decimal(err, d, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_NUM1: {
	    uint8_t	len = *wire++;

	    if (NULL != callbacks->number) {
		callbacks->number(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_NUM2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    if (NULL != callbacks->number) {
		callbacks->number(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_NUM4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    if (NULL != callbacks->number) {
		callbacks->number(err, (const char*)wire, (int)len, ctx);
	    }
	    wire += len;
	    break;
	}
	case WIRE_UUID:
	    if (NULL != callbacks->uuid || NULL != callbacks->uuid_str) {
		uint64_t	hi;
		uint64_t	lo;
	    
		wire = read_uint64(wire, &hi);
		wire = read_uint64(wire, &lo);
		if (NULL != callbacks->uuid) {
		    callbacks->uuid(err, hi, lo, ctx);
		} else {
		    char	buf[40];

		    sprintf(buf, "%08lx-%04lx-%04lx-%04lx-%012lx",
			    (unsigned long)(hi >> 32),
			    (unsigned long)((hi >> 16) & 0x000000000000FFFFUL),
			    (unsigned long)(hi & 0x000000000000FFFFUL),
			    (unsigned long)(lo >> 48),
			    (unsigned long)(lo & 0x0000FFFFFFFFFFFFUL));
		    callbacks->uuid_str(err, buf, ctx);
		}
	    } else {
		wire += 16;
	    }
	    break;
	case WIRE_TIME:
	    if (NULL != callbacks->time || NULL != callbacks->time_str) {
		uint64_t	nsec;
	    
		wire = read_uint64(wire, &nsec);
		if (NULL != callbacks->time) {
		    callbacks->time(err, nsec, ctx);
		} else {
		    char	buf[64];
		    struct tm	tm;
		    time_t	t = (time_t)(nsec / 1000000000LL);
		    long	frac = nsec - (int64_t)t * 1000000000LL;

		    if (NULL == gmtime_r(&t, &tm)) {
			err->code = OJC_PARSE_ERR;
			snprintf(err->msg, sizeof(err->msg), "invalid time");
			break;
		    }
		    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ",
			    1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
			    tm.tm_hour, tm.tm_min, tm.tm_sec, frac);
		    callbacks->time_str(err, buf, ctx);
		}
	    } else {
		wire += 8;
	    }
	    break;
	case WIRE_OBEG:
	    if (NULL != callbacks->begin_object) {
		callbacks->begin_object(err, ctx);
	    }
	    break;
	case WIRE_OEND:
	    if (NULL != callbacks->end_object) {
		callbacks->end_object(err, ctx);
	    }
	    break;
	case WIRE_ABEG:
	    if (NULL != callbacks->begin_array) {
		callbacks->begin_array(err, ctx);
	    }
	    break;
	case WIRE_AEND:
	    if (NULL != callbacks->end_array) {
		callbacks->end_array(err, ctx);
	    }
	    break;
	default:
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "corrupt wire format");
	    break;
	}
	if (OJC_OK != err->code) {
	    break;
	}
    }
    return err->code;
}

int
ojc_wire_cbparse_file(ojcErr err, FILE *file, ojcWireCallbacks callbacks, void *ctx) {
    return ojc_wire_cbparse_fd(err, fileno(file), callbacks, ctx);
}

int
ojc_wire_cbparse_fd(ojcErr err, int fd, ojcWireCallbacks callbacks, void *ctx) {
    off_t	here = lseek(fd, 0, SEEK_CUR);
    off_t	size = lseek(fd, 0, SEEK_END) - here;
    
    lseek(fd, here, SEEK_SET);
    if (MAX_STACK_BUF <= size) {
	uint8_t	*buf = (uint8_t*)malloc(size);

	if (size != read(fd, buf, size)) {
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "read failed: %s", strerror(errno));
	}
	ojc_wire_cbparse(err, buf, callbacks, ctx);
	free(buf);
    } else {
	uint8_t	buf[MAX_STACK_BUF];

	if (size != read(fd, buf, size)) {
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "read failed: %s", strerror(errno));
	}
	ojc_wire_cbparse(err, buf, callbacks, ctx);
    }
    return err->code;
}

///// builder

static int
wire_assure(ojcErr err, ojcWire wire, size_t size) {
    if (wire->end <= wire->cur + size) {
	size_t	new_size = wire->end - wire->buf + WIRE_INC + size;
	int	off = wire->cur - wire->buf;
	
	if (wire->own) {
	    wire->buf = (uint8_t*)realloc(wire->buf, new_size);
	} else {
	    wire->buf = (uint8_t*)malloc(new_size);
	}
	if (NULL == wire->buf) {
	    err->code = OJC_MEMORY_ERR;
	    snprintf(err->msg, sizeof(err->msg), "memory allocation failed for size %lu", (unsigned long)new_size);
	}
	wire->end = wire->buf + new_size;
	wire->cur = wire->buf + off;
    }
    return OJC_OK;
}
    
static int
wire_append_byte(ojcErr err, ojcWire wire, uint8_t b) {
    if (OJC_OK != wire_assure(err, wire, 1)) {
	return err->code;
    }
    *wire->cur++ = b;

    return OJC_OK;
}
    
static int
wire_push_str(ojcErr err, ojcWire wire, const char *str, size_t len) {
    if (OJC_OK != wire_assure(err, wire, len + 5)) {
	return err->code;
    }
    wire->cur = fill_str(wire->cur, str, len);

    return OJC_OK;
}
    
static int
wire_push_key(ojcErr err, ojcWire wire, const char *str, size_t len) {
    if (OJC_OK != wire_assure(err, wire, len + 5)) {
	return err->code;
    }
    wire->cur = fill_key(wire->cur, str, len);

    return OJC_OK;
}
    
int
ojc_wire_init(ojcErr err, ojcWire wire, uint8_t *buf, size_t size) {
    if (NULL == buf) {
	if (size < MIN_WIRE_BUF) {
	    size = MIN_WIRE_BUF;
	}
	if (NULL == (wire->buf = (uint8_t*)malloc(size))) {
	    err->code = OJC_MEMORY_ERR;
	    snprintf(err->msg, sizeof(err->msg), "memory allocation failed for size %lu", (unsigned long)size );
	    return err->code;
	}
	wire->own = true;
    } else {
	wire->buf = buf;
	wire->own = false;
    }
    wire->end = wire->buf + size;
    wire->cur = wire->buf;
    memset(wire->stack, 0, sizeof(wire->stack));
    wire->top = wire->stack - 1;
    wire->cur = fill_uint32(wire->cur, 0);
    
    return OJC_OK;
}

void
ojc_wire_cleanup(ojcWire wire) {
    if (wire->own) {
	free(wire->buf);
    }
}

int
ojc_wire_finish(ojcErr err, ojcWire wire) {
    for (; wire->stack <= wire->top; wire->top--) {
	if (*wire->top) {
	    if (OJC_OK != wire_append_byte(err, wire, WIRE_OEND)) {
		return err->code;
	    }
	} else {
	    if (OJC_OK != wire_append_byte(err, wire, WIRE_AEND)) {
		return err->code;
	    }
	}
    }
    fill_uint32(wire->buf, (uint32_t)(wire->cur - wire->buf - 4));

    return 0;
}

size_t
ojc_wire_length(ojcWire wire) {
    return wire->cur - wire->buf;
}

uint8_t*
ojc_wire_take(ojcWire wire) {
    uint8_t	*buf = wire->buf;

    wire->buf = NULL;
    wire->cur = NULL;
    wire->end = NULL;
    
    return buf;
}

int
ojc_wire_push_object(ojcErr err, ojcWire wire) {
    if (wire->stack + WIRE_STACK_SIZE <= wire->top) {
	err->code = OJC_OVERFLOW_ERR;
	snprintf(err->msg, sizeof(err->msg), "too deeply nested. Limit is %d", WIRE_STACK_SIZE);
	return err->code;
    }
    wire->top++;
    *wire->top = true;
	
    return wire_append_byte(err, wire, WIRE_OBEG);
}

int
ojc_wire_push_array(ojcErr err, ojcWire wire) {
    if (wire->stack + WIRE_STACK_SIZE <= wire->top) {
	err->code = OJC_OVERFLOW_ERR;
	snprintf(err->msg, sizeof(err->msg), "too deeply nested. Limit is %d", WIRE_STACK_SIZE);
	return err->code;
    }
    wire->top++;
    *wire->top = false;
	
    return wire_append_byte(err, wire, WIRE_ABEG);
}

int
ojc_wire_pop(ojcErr err, ojcWire wire) {
    if (wire->stack > wire->top) {
	err->code = OJC_OVERFLOW_ERR;
	snprintf(err->msg, sizeof(err->msg), "nothing left to pop");
	return err->code;
    }
    if (*wire->top) {
	wire_append_byte(err, wire, WIRE_OEND);
    } else {
	wire_append_byte(err, wire, WIRE_AEND);
    }
    wire->top--;

    return OJC_OK;
}

int
ojc_wire_push_key(ojcErr err, ojcWire wire, const char *key, int len) {
    if (!*wire->top) {
	err->code = OJC_ARG_ERR;
	snprintf(err->msg, sizeof(err->msg), "can only push a key to an object");
	return err->code;
    }
    if (0 >= len) {
	len = strlen(key);
    }
    if (OJC_OK != wire_push_key(err, wire, key, len)) {
	return err->code;
    }
    return OJC_OK;
}

int
ojc_wire_push_null(ojcErr err, ojcWire wire) {
    return wire_append_byte(err, wire, WIRE_NULL);
}

int
ojc_wire_push_bool(ojcErr err, ojcWire wire, bool value) {
    return wire_append_byte(err, wire, value ? WIRE_TRUE : WIRE_FALSE);
}

int
ojc_wire_push_int(ojcErr err, ojcWire wire, int64_t value) {
    if (-128 <= value && value <= 127) {
	if (OJC_OK != wire_assure(err, wire, 2)) {
	    return err->code;
	}
	*wire->cur++ = WIRE_INT1;
	*wire->cur++ = (uint8_t)(int8_t)value;
    } else if (-32768 <= value && value <= 32767) {
	if (OJC_OK != wire_assure(err, wire, 3)) {
	    return err->code;
	}
	*wire->cur++ = WIRE_INT2;
	wire->cur = fill_uint16(wire->cur, (uint16_t)(int16_t)value);
    } else if (-2147483648 <= value && value <= 2147483647) {
	if (OJC_OK != wire_assure(err, wire, 5)) {
	    return err->code;
	}
	*wire->cur++ = WIRE_INT4;
	wire->cur = fill_uint32(wire->cur, (uint32_t)(int32_t)value);
    } else {
	if (OJC_OK != wire_assure(err, wire, 9)) {
	    return err->code;
	}
	*wire->cur++ = WIRE_INT8;
	wire->cur = fill_uint64(wire->cur, (uint64_t)value);
    }
    return OJC_OK;
}

int
ojc_wire_push_double(ojcErr err, ojcWire wire, double value) {
    char	str[64];
    int		cnt;
    
    if (value == (double)(int64_t)value) {
	cnt = snprintf(str, sizeof(str) - 1, "%.1f", value);
    } else {
	cnt = snprintf(str, sizeof(str) - 1, "%0.15g", value);
    }
    if (OJC_OK != wire_assure(err, wire, cnt + 2)) {
	return err->code;
    }
    *wire->cur++ = WIRE_DEC;
    *wire->cur++ = (uint8_t)cnt;
    memcpy(wire->cur, str, cnt);
    wire->cur += cnt;

    return OJC_OK;
}

int
ojc_wire_push_string(ojcErr err, ojcWire wire, const char *value, int len) {
    if (0 >= len) {
	len = strlen(value);
    }
    if (OJC_OK != wire_push_str(err, wire, value, len)) {
	return err->code;
    }
    return OJC_OK;
}

int
ojc_wire_push_uuid(ojcErr err, ojcWire wire, uint64_t hi, uint64_t lo) {
    if (OJC_OK != wire_assure(err, wire, 17)) {
	return err->code;
    }
    *wire->cur++ = WIRE_UUID;
    wire->cur = fill_uint64(wire->cur, hi);
    wire->cur = fill_uint64(wire->cur, lo);

    return OJC_OK;
}

int
ojc_wire_push_uuid_string(ojcErr err, ojcWire wire, const char *value) {
    if (OJC_OK != wire_assure(err, wire, 17)) {
	return err->code;
    }
    wire->cur = fill_uuid(wire->cur, value);

    return OJC_OK;
}

int
ojc_wire_push_time(ojcErr err, ojcWire wire, uint64_t value) {
    if (OJC_OK != wire_assure(err, wire, 37)) {
	return err->code;
    }
    *wire->cur++ = WIRE_TIME;
    wire->cur = fill_uint64(wire->cur, value);

    return OJC_OK;
}
