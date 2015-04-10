/* ojc.c
 * Copyright (c) 2014, Peter Ohler
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

static const char	hex_chars[17] = "0123456789abcdef";

static char		newline_friendly_chars[257] = "\
66666666221622666666666666666666\
11211111111111111111111111111111\
11111111111111111111111111112111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111";

static char		hibit_friendly_chars[257] = "\
66666666222622666666666666666666\
11211111111111111111111111111111\
11111111111111111111111111112111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111";

bool		ojc_newline_ok = false;
bool		ojc_word_ok = false;
bool		ojc_decimal_as_number = false;

const char*
ojc_version() {
    return OJC_VERSION;
}

void
ojc_cleanup() {
    _ojc_val_cleanup();
}

void
ojc_destroy(ojcVal val) {
    _ojc_destroy(val);
}

ojcVal
ojc_parse_str(ojcErr err, const char *json, ojcParseCallback cb, void *ctx) {
    struct _ParseInfo	pi;
    ojcVal		val;

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
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

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
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

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
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

void
ojc_parse_stream_follow(ojcErr err, FILE *file, ojcParseCallback cb, void *ctx) {
    struct _ParseInfo	pi;

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return;
    }
    parse_init(&pi.err, &pi, cb, ctx);
    reader_init_stream(err, &pi.rd, file);
    pi.rd.follow = true;
    if (OJC_OK != err->code) {
	return;
    }
    ojc_parse(&pi);
    if (OJC_OK != pi.err.code && 0 != err) {
	err->code = pi.err.code;
	memcpy(err->msg, pi.err.msg, sizeof(pi.err.msg));
    }
    parse_cleanup(&pi);
}

ojcVal
ojc_parse_reader(ojcErr err, void *src, ojcReadFunc rf, ojcParseCallback cb, void *ctx) {
    struct _ParseInfo	pi;
    ojcVal		val;

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
    parse_init(&pi.err, &pi, cb, ctx);
    reader_init_func(err, &pi.rd, src, rf);
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
		index = index * 10 + (unsigned int)(*path - '0');
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
	    break;
	}
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
		case STR_BLOCK:	key = m->key.bstr->ca;	break;
		case STR_NONE:
		default:	key = 0;		break;
		}
		if (0 != key && 0 == strcmp(*path, key)) {
		    return ojc_aget(m, path + 1);
		}
	    }
	    break;
	}
    default:
	break;
    }
    return 0;
}

ojcValType
ojc_type(ojcVal val) {
    return (ojcValType)val->type;
}

static bool
is_type_ok(ojcErr err, ojcVal val, ojcValType type) {
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return false;
    }
    if (0 == val) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not get an %s from NULL", ojc_type_str(type));
	}
	return false;
    }
    if (type != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg),
		     "Can not get an %s from a %s", ojc_type_str(type), ojc_type_str((ojcValType)val->type));
	}
	return false;
    }
    return true;
}

int64_t
ojc_int(ojcErr err, ojcVal val) {
    if (!is_type_ok(err, val, OJC_FIXNUM)) {
	return 0;
    }
    return val->fixnum;
}

double
ojc_double(ojcErr err, ojcVal val) {
    if (!is_type_ok(err, val, OJC_DECIMAL)) {
	return 0;
    }
    return val->dub;
}

const char*
ojc_number(ojcErr err, ojcVal val) {
    if (!is_type_ok(err, val, OJC_NUMBER)) {
	return 0;
    }
    switch (val->str_type) {
    case STR_PTR:	return val->str.str;
    case STR_ARRAY:	return val->str.ca;
    case STR_BLOCK:	return val->str.bstr->ca;
    case STR_NONE:
    default:		return 0;
    }
}

const char*
ojc_str(ojcErr err, ojcVal val) {
    if (!is_type_ok(err, val, OJC_STRING)) {
	return 0;
    }
    switch (val->str_type) {
    case STR_PTR:	return val->str.str;
    case STR_ARRAY:	return val->str.ca;
    case STR_BLOCK:	return val->str.bstr->ca;
    case STR_NONE:
    default:		return 0;
    }
}

const char*
ojc_word(ojcErr err, ojcVal val) {
    if (!is_type_ok(err, val, OJC_WORD)) {
	return 0;
    }
    return val->str.ca;
}

ojcVal
ojc_members(ojcErr err, ojcVal val) {
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
    if (0 == val) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    strcpy(err->msg, "Can not get members from NULL");
	}
	return 0;
    }
    if (OJC_ARRAY != val->type && OJC_OBJECT != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not get members from a %s", ojc_type_str((ojcValType)val->type));
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

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
    if (0 == val) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    strcpy(err->msg, "No members in NULL");
	}
	return 0;
    }
    if (OJC_ARRAY != val->type && OJC_OBJECT != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "No members in a %s", ojc_type_str((ojcValType)val->type));
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
    case STR_BLOCK:	return val->key.bstr->ca;
    case STR_NONE:
    default:		return 0;
    }
}

void
ojc_set_key(ojcVal val, const char *key) {
    _ojc_set_key(val, key, 0);
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
    if (sizeof(union _Bstr) <= len) {
	val->str_type = STR_PTR;
	val->str.str = strndup(str, len);
	val->str.str[len] = '\0';
    } else if (sizeof(val->str.ca) <= len) {
	val->str_type = STR_BLOCK;
	val->str.bstr = _ojc_bstr_create();
	memcpy(val->str.bstr->ca, str, len);
	val->str.bstr->ca[len] = '\0';
    } else {
	val->str_type = STR_ARRAY;
	memcpy(val->str.ca, str, len);
	val->str.ca[len] = '\0';
    }
    return val;
}

ojcVal
ojc_create_word(const char *str, size_t len) {
    ojcVal	val = _ojc_val_create(OJC_WORD);
    
    if (0 >= len) {
	len = strlen(str);
    }
    memcpy(val->str.ca, str, len);
    val->str.ca[len] = '\0';

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
    if (sizeof(union _Bstr) <= len) {
	val->str_type = STR_PTR;
	val->str.str = strndup(num, len);
	val->str.str[len] = '\0';
    } else if (sizeof(val->str.ca) <= len) {
	val->str_type = STR_BLOCK;
	val->str.bstr = _ojc_bstr_create();
	memcpy(val->str.bstr->ca, num, len);
	val->str.bstr->ca[len] = '\0';
    } else {
	val->str_type = STR_ARRAY;
	memcpy(val->str.ca, num, len);
	val->str.ca[len] = '\0';
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

// returns false if okay, true if there is an error
static bool
bad_object(ojcErr err, ojcVal object, const char *op) {
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return true;
    }
    if (0 == object) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not object %s to NULL", op);
	}
	return true;
    }
    if (OJC_OBJECT != object->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not object %s to a %s",
		     op, ojc_type_str((ojcValType)object->type));
	}
	return true;
    }
    return false;
}

void
ojc_object_nappend(ojcErr err, ojcVal object, const char *key, int klen, ojcVal val) {
    if (bad_object(err, object, "append")) {
	return;
    }
    val->next = 0;
    _ojc_set_key(val, key, klen);
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
ojc_object_replace(ojcErr err, ojcVal object, const char *key, ojcVal val) {
    ojcVal	m;
    ojcVal	prev = 0;

    if (bad_object(err, object, "replace")) {
	return;
    }
    _ojc_set_key(val, key, 0);
    for (m = object->members.head; 0 != m; m = m->next) {
	if (0 == strcmp(key, ojc_key(m))) {
	    val->next = m->next;
	    if (0 == prev) {
		object->members.head = val;
	    } else {
		prev->next = val;
	    }
	    if (0 == val->next) {
		object->members.tail = val;
	    }
	    m->next = 0;
	    ojc_destroy(m);
	    break;
	}
	prev = m;
    }
}

void
ojc_object_insert(ojcErr err, ojcVal object, int before, const char *key, ojcVal val) {
    if (bad_object(err, object, "insert")) {
	return;
    }
    _ojc_set_key(val, key, 0);
    val->next = 0;
    if (0 >= before || 0 == object->members.head) {
	val->next = object->members.head;
	object->members.head = val;
    } else {
	ojcVal	m;

	before--;
	for (m = object->members.head; 0 != m; m = m->next, before--) {
	    if (0 >= before) {
		val->next = m->next;
		m->next = val;
		break;
	    }
	}
	if (0 == m) {
	    object->members.tail->next = val;
	}
    }
    if (0 == val->next) {
	object->members.tail = val;
    }
}

void
ojc_object_remove_by_pos(ojcErr err, ojcVal object, int pos) {
    ojcVal	m;
    ojcVal	prev = 0;
    int		p = pos;

    if (bad_object(err, object, "remove by position")) {
	return;
    }
    for (m = object->members.head; 0 != m && 0 < pos; m = m->next, p--) {
	prev = m;
    }
    if (0 != p || 0 == object->members.head || 0 == m) {
	if (0 != err) {
	    err->code = OJC_ARG_ERR;
	    snprintf(err->msg, sizeof(err->msg), "No element at position %d.", pos);
	}
	return;
    }
    if (0 == prev) {
	object->members.head = m->next;
    } else {
	prev->next = m->next;
    }
    if (0 == prev->next) {
	object->members.tail = prev;
    }
    m->next = 0;
    ojc_destroy(m);
}

void
ojc_object_remove_by_key(ojcErr err, ojcVal object, const char *key) {
    ojcVal	m;
    ojcVal	prev = 0;
    ojcVal	next;

    if (bad_object(err, object, "remove by key")) {
	return;
    }
    for (m = object->members.head; 0 != m; m = next) {
	next = m->next;
	if (0 == strcmp(key, ojc_key(m))) {
	    if (0 == prev) {
		object->members.head = m->next;
	    } else {
		prev->next = m->next;
	    }
	    if (m == object->members.tail) {
		object->members.tail = prev;
	    }
	    m->next = 0;
	    ojc_destroy(m);
	    break;
	}
	prev = m;
    }
}

ojcVal
ojc_object_get_by_key(ojcErr err, ojcVal object, const char *key) {
    ojcVal	m = 0;

    if (bad_object(err, object, "get by key")) {
	return 0;
    }
    for (m = object->members.head; 0 != m; m = m->next) {
	if (0 == strcmp(key, ojc_key(m))) {
	    break;
	}
    }
    return m;
}

ojcVal
ojc_get_member(ojcErr err, ojcVal val, int pos) {
    ojcVal	m = 0;

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
    if (0 == val) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    strcpy(err->msg, "Can only get a member from an array or object, not from NULL.");
	}
	return 0;
    }
    if (OJC_ARRAY != val->type && OJC_OBJECT != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can only get a member from an array or object, not a %s",
		     ojc_type_str((ojcValType)val->type));
	}
	return 0;
    }
    if (0 > pos) {
	return 0;
    }
    for (m = val->members.head; 0 != m && 0 < pos; m = m->next, pos--) {
    }
    return m;
}

// returns false if okay, true if there is an error
static bool
bad_array(ojcErr err, ojcVal array, const char *op) {
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return true;
    }
    if (0 == array) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not array %s to NULL", op);
	}
	return true;
    }
    if (OJC_ARRAY != array->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not array %s to a %s",
		     op, ojc_type_str((ojcValType)array->type));
	}
	return true;
    }
    return false;
}

void
ojc_array_append(ojcErr err, ojcVal array, ojcVal val) {
    if (bad_array(err, array, "append")) {
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

void
ojc_array_push(ojcErr err, ojcVal array, ojcVal val) {
    if (bad_array(err, array, "push")) {
	return;
    }
    val->next = array->members.head;
    array->members.head = val;
    if (0 == array->members.tail) {
	array->members.tail = val;
    }
}

ojcVal
ojc_array_pop(ojcErr err, ojcVal array) {
    if (bad_array(err, array, "pop")) {
	return 0;
    }
    if (0 != array->members.head) {
	ojcVal	val = array->members.head;

	array->members.head = val->next;
	if (0 == array->members.head) {
	    array->members.tail = 0;
	}
	val->next = 0;

	return val;
    }
    return 0;
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

const char*
buf_unicode(Buf buf, const char *str) {
    uint32_t	code = 0;
    uint8_t	b = *(uint8_t*)str;
    int		i, cnt;
    
    if (0xC0 == (0xE0 & b)) {
	cnt = 1;
	code = b & 0x0000001F;
    } else if (0xE0 == (0xF0 & b)) {
	cnt = 2;
	code = b & 0x0000000F;
    } else if (0xF0 == (0xF8 & b)) {
	cnt = 3;
	code = b & 0x00000007;
    } else if (0xF8 == (0xFC & b)) {
	cnt = 4;
	code = b & 0x00000003;
    } else if (0xFC == (0xFE & b)) {
	cnt = 5;
	code = b & 0x00000001;
    } else {
	cnt = 0;
	buf->err = OJC_UNICODE_ERR;
	return 0;
    }
    str++;
    for (; 0 < cnt; cnt--, str++) {
	b = *(uint8_t*)str;
	if (0 == b || 0x80 != (0xC0 & b)) {
	    buf->err = OJC_UNICODE_ERR;
	    return 0;
	}
	code = (code << 6) | (b & 0x0000003F);
    }
    if (0x0000FFFF < code) {
	uint32_t	c1;

	code -= 0x00010000;
	c1 = ((code >> 10) & 0x000003FF) + 0x0000D800;
	code = (code & 0x000003FF) + 0x0000DC00;
	buf_append(buf, '\\');
	buf_append(buf, 'u');
	for (i = 3; 0 <= i; i--) {
	    buf_append(buf, hex_chars[(uint8_t)(c1 >> (i * 4)) & 0x0F]);
	}
    }
    buf_append(buf, '\\');
    buf_append(buf, 'u');
    for (i = 3; 0 <= i; i--) {
	buf_append(buf, hex_chars[(uint8_t)(code >> (i * 4)) & 0x0F]);
    }	
    return str - 1;
}

static void
fill_hex(uint8_t c, char *buf) {
    uint8_t	d = (c >> 4) & 0x0F;

    *buf++ = hex_chars[d];
    d = c & 0x0F;
    *buf++ = hex_chars[d];
}

static const char*
buf_append_chars(Buf buf, const char *str) {
    int	cnt = ojc_newline_ok ? newline_friendly_chars[(uint8_t)*str] : hibit_friendly_chars[(uint8_t)*str];
    switch (cnt) {
    case '1':
	buf_append(buf, *str);
	str++;
	break;
    case '2':
	buf_append(buf, '\\');
	switch (*str) {
	case '\b':	buf_append(buf, 'b');	break;
	case '\t':	buf_append(buf, 't');	break;
	case '\n':	buf_append(buf, 'n');	break;
	case '\f':	buf_append(buf, 'f');	break;
	case '\r':	buf_append(buf, 'r');	break;
	default:	buf_append(buf, *str);	break;
	}
	str++;
	break;
    case '3': // Unicode
	str = buf_unicode(buf, str);
	break;
    case '6': // control characters
	{
	    char	hex[4];

	    buf_append_string(buf, "\\u00", 4);
	    fill_hex((uint8_t)*str, hex);
	    buf_append_string(buf, hex, 2);
	    str++;
	}
	break;
    default:
	str++;
	break; // ignore, should never happen if the table is correct
    }
    return str;
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
		if (OJC_OK != buf->err) {
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
	    const char	*key;

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

		switch (m->key_type) {
		case STR_PTR:	key = m->key.str;	break;
		case STR_ARRAY:	key = m->key.ca;	break;
		case STR_BLOCK:	key = m->key.bstr->ca;	break;
		case STR_NONE:
		default:	key = "";		break;
		}
		while ('\0' != *key) {
		    key = buf_append_chars(buf, key);
		}
		buf_append(buf, '"');
		buf_append(buf, ':');
		fill_buf(buf, m, indent, d2);
		if (OJC_OK != buf->err) {
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
	{
	    const char	*str;

	    buf_append(buf, '"');
	    switch (val->str_type) {
	    case STR_PTR:	str = val->str.str;		break;
	    case STR_ARRAY:	str = val->str.ca;		break;
	    case STR_BLOCK:	str = val->str.bstr->ca;	break;
	    case STR_NONE:
	    default:		str = "";			break;
	    }
	    while ('\0' != *str) {
		str = buf_append_chars(buf, str);
	    }
	    buf_append(buf, '"');
	}
	break;
    case OJC_WORD:
	buf_append_string(buf, val->str.ca, strlen(val->str.ca));
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
	{
	    const char	*str;

	    switch (val->str_type) {
	    case STR_PTR:	str = val->str.str;		break;
	    case STR_ARRAY:	str = val->str.ca;		break;
	    case STR_BLOCK:	str = val->str.bstr->ca;	break;
	    case STR_NONE:
	    default:		str = "";			break;
	    }
	    buf_append_string(buf, str, strlen(str));
	}
	break;
    default:
	break;
    }
}

char*
ojc_to_str(ojcVal val, int indent) {
    struct _Buf	b;
    
    buf_init(&b, 0);
    fill_buf(&b, val, indent, 0);
    if (OJC_OK != b.err) {
	return 0;
    }
    *b.tail = '\0';
    if (b.base == b.head) {
	return strdup(b.head);
    }
    return b.head;
}

int
ojc_fill(ojcErr err, ojcVal val, int indent, char *buf, size_t len) {
    struct _Buf	b;
    
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
    buf_finit(&b, buf, len);
    fill_buf(&b, val, indent, 0);
    if (OJC_OK != b.err) {
	if (0 != err) {
	    err->code = b.err;
	    strcpy(err->msg, ojc_error_str((ojcErrCode)b.err));
	}
	return -1;
    }
    *b.tail = '\0';

    return buf_len(&b);
}

void
ojc_write(ojcErr err, ojcVal val, int indent, int socket) {
    struct _Buf	b;
    
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return;
    }
    buf_init(&b, socket);
    fill_buf(&b, val, indent, 0);
    buf_finish(&b);
    if (OJC_OK != b.err) {
	err->code = b.err;
	strcpy(err->msg, ojc_error_str((ojcErrCode)b.err));
    }
}

void
ojc_fwrite(ojcErr err, ojcVal val, int indent, FILE *file) {
    ojc_write(err, val, indent, fileno(file));
}

ojcVal
ojc_duplicate(ojcVal val) {
    ojcVal		dup;
    struct _ojcErr	err = OJC_ERR_INIT;

    if (0 == val) {
	return 0;
    }
    dup = _ojc_val_create((ojcValType)val->type);
    switch (val->type) {
    case OJC_ARRAY:
	{
	    ojcVal	m;

	    for (m = val->members.head; 0 != m; m = m->next) {
		ojc_array_append(&err, dup, ojc_duplicate(m));
	    }
	}
	break;
    case OJC_OBJECT:
	{
	    ojcVal	m;
	    const char	*key;

	    for (m = val->members.head; 0 != m; m = m->next) {
		switch (m->key_type) {
		case STR_PTR:	key = m->key.str;	break;
		case STR_ARRAY:	key = m->key.ca;	break;
		case STR_BLOCK:	key = m->key.bstr->ca;	break;
		case STR_NONE:
		default:	key = "";		break;
		}
		ojc_object_append(&err, dup, key, ojc_duplicate(m));
	    }
	}
	break;
    case OJC_NULL:
    case OJC_TRUE:
    case OJC_FALSE:
	break;
    case OJC_STRING:
    case OJC_NUMBER:
	dup->str_type = val->str_type;
	switch (val->str_type) {
	case STR_PTR:
	    dup->str.str = strdup(val->str.str);
	    break;
	case STR_ARRAY:
	    strcpy(dup->str.ca, val->str.ca);
	    break;
	case STR_BLOCK:
	    dup->str.bstr = _ojc_bstr_create();
	    strcpy(dup->str.bstr->ca, val->str.bstr->ca);
	    break;
	case STR_NONE:
	default:
	    break;
	}
	break;
    case OJC_WORD:
	strcpy(dup->str.ca, val->str.ca);
	break;
    case OJC_FIXNUM:
	dup->fixnum = val->fixnum;
	break;
    case OJC_DECIMAL:
	dup->dub = val->dub;
	break;
    default:
	break;
    }
    return dup;
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
    case OJC_WORD:	return "word";
    default:		return "unknown";
    }
}

const char*
ojc_error_str(ojcErrCode code) {
    switch (code) {
    case OJC_OK:		return "ok";
    case OJC_TYPE_ERR:		return "type error";
    case OJC_PARSE_ERR:		return "parse error";
    case OJC_OVERFLOW_ERR:	return "overflow error";
    case OJC_WRITE_ERR:		return "write error";
    case OJC_MEMORY_ERR:	return "memory error";
    case OJC_UNICODE_ERR:	return "unicode error";
    case OJC_ABORT_ERR:		return "abort";
    case OJC_ARG_ERR:		return "argument error";
    default:			return "unknown";
    }
}
