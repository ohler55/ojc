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

#include "buf.h"
#include "wire.h"
#include "val.h"

typedef enum {
    WIRE_NULL	= (uint8_t)'Z',
    WIRE_TRUE	= (uint8_t)'t',
    WIRE_FALSE	= (uint8_t)'f',
    WIRE_INT1	= (uint8_t)'i',
    WIRE_INT2	= (uint8_t)'j',
    WIRE_INT4	= (uint8_t)'k',
    WIRE_INT8	= (uint8_t)'I',
    WIRE_STR1	= (uint8_t)'s',
    WIRE_STR2	= (uint8_t)'S',
    WIRE_STR4	= (uint8_t)'B',
    WIRE_DEC	= (uint8_t)'d',
    WIRE_NUM	= (uint8_t)'n',
    WIRE_UUID	= (uint8_t)'u',
    WIRE_TIME	= (uint8_t)'T',
    WIRE_OBEG	= (uint8_t)'{',
    WIRE_OEND	= (uint8_t)'}',
    WIRE_ABEG	= (uint8_t)'[',
    WIRE_AEND	= (uint8_t)']',
} ojcWireTag;


void
ojc_to_wire_buf(Buf buf, ojcVal val) {
    // TBD

}

int
ojc_to_wire_file(ojcErr err, ojcVal val, FILE *file) {
    // TBD
    return 0;
}

int
ojc_to_wire_fd(ojcErr err, ojcVal val, int fd) {
    // TBD
    return 0;
}

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
	// TBD detect time and uuid
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

int
ojc_wire_size(ojcVal val) {
    if (NULL == val) {
	return 0;
    }
    return wire_size(val) + 4;
}

int
ojc_wire_to_str(ojcErr err, Buf buf, const Wire wire) {
    // TBD
    return 0;
}

ojcVal
ojc_wire_parse_str(ojcErr err, const Wire wire) {
    // TBD
    return 0;
}

ojcVal
ojc_wire_parse_file(ojcErr err, FILE *file) {
    // TBD
    return 0;
}

ojcVal
ojc_wire_parse_fd(ojcErr err, int fd) {
    // TBD
    return 0;
}

