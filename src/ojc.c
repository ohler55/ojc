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
bool		ojc_case_insensitive = false;
bool		ojc_write_opaque = false;
bool		ojc_write_end_with_newline = true;

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
    if (NULL != val) {
	_ojc_destroy(val);
    }
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
    ojc_reader_init_str(&pi.err, &pi.rd, json);
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
ojc_parse_strp(ojcErr err, const char **jsonp) {
    const char		*json = *jsonp;
    struct _ParseInfo	pi;
    ojcVal		val;

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return 0;
    }
    parse_init(&pi.err, &pi, NULL, NULL);
    ojc_reader_init_str(&pi.err, &pi.rd, json);
    if (OJC_OK != pi.err.code) {
	return 0;
    }
    ojc_parse(&pi);
    val = *pi.stack.head;
    if (OJC_OK != pi.err.code && 0 != err) {
	err->code = pi.err.code;
	memcpy(err->msg, pi.err.msg, sizeof(pi.err.msg));
    } else {
	*jsonp = pi.rd.tail;
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
    ojc_reader_init_stream(err, &pi.rd, file);
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
    ojc_reader_init_socket(err, &pi.rd, socket);
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
    ojc_reader_init_follow(err, &pi.rd, file);
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
    ojc_reader_init_func(err, &pi.rd, src, rf);
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
    const char		*start;
    volatile ojcVal	m;

    if (0 == path || 0 == val) {
	return val;
    }
    if ('/' == *path || '.' == *path) {
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

	    for (; '\0' != *path && '/' != *path && '.' != *path; path++) {
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

	    for (; '\0' != *path && '/' != *path && '.' != *path; path++) {
	    }
	    plen = path - start;
	    for (m = val->members.head; ; m = m->next) {
		if (0 == m) {
		    return 0;
		}
		key = ojc_key(m);
		if (0 != key &&
		    (ojc_case_insensitive ?
		     0 == strncasecmp(start, key, plen) :
		     0 == strncmp(start, key, plen)) &&
		    '\0' == key[plen]) {
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
		key = ojc_key(m);
		if (0 != key &&
		    (ojc_case_insensitive ?
		     0 == strcasecmp(*path, key) :
		     0 == strcmp(*path, key))) {
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

static ojcVal
get_parent(ojcVal val, const char *path, const char **keyp) {
    const char	*start;
    ojcVal	m;
    
    if ('/' == *path || '.' == *path) {
	path++;
    }
    if ('\0' == *path) {
	// Indication to the caller that it is the parent.
	return 0;
    }
    start = path;
    switch (val->type) {
    case OJC_ARRAY: {
	unsigned int	index = 0;

	for (; '/' != *path && '.' != *path; path++) {
	    if ('\0' == *path) { // this is the parent
		*keyp = start;
		return val;
	    }
	    if (*path < '0' || '9' < *path) {
		return 0;
	    }
	    index = index * 10 + (unsigned int)(*path - '0');
	    if (MAX_INDEX < index) {
		return 0;
	    }
	}
	for (m = val->members.head; 0 < index && 0 != m; m = m->next, index--) {
	}
	if (0 != m) {
	    ojcVal	p = get_parent(m, path, keyp);

	    if (0 != p) {
		return p;
	    }
	}
	return 0;
    }
    case OJC_OBJECT: {
	const char	*key;
	int		plen;
	struct _ojcErr	err = OJC_ERR_INIT;
	ojcVal		child;

	for (; '/' != *path && '.' != *path; path++) {
	    if ('\0' == *path) { // this is the parent
		*keyp = start;
		return val;
	    }
	}
	plen = path - start;
	for (m = val->members.head; 0 != m; m = m->next) {
	    key = ojc_key(m);
	    if (0 != key &&
		(ojc_case_insensitive ?
		 0 == strncasecmp(start, key, plen) :
		 0 == strncmp(start, key, plen)) &&
		'\0' == key[plen]) {
		ojcVal	p = get_parent(m, path, keyp);

		if (0 != p) {
		    return p;
		}
	    }
	}
	child = ojc_create_object();
	ojc_object_nappend(&err, val, start, plen, child); 
	return get_parent(child, path, keyp);
    }
    default:
	break;
    }
    return 0;
}

static ojcVal
get_aparent(ojcVal val, const char **path, const char **keyp) {
    ojcVal	m;
    const char	**pn = path + 1;
    
    if (0 == *path) {
	return 0;
    }
    switch (val->type) {
    case OJC_ARRAY: {
	unsigned int	index = 0;
	const char	*p;

	if (0 == *pn) {
	    *keyp = *path;
	    return val;
	}
	for (p = *path; '\0' != *p; p++) {
	    if (*p < '0' || '9' < *p) {
		return 0;
	    }
	    index = index * 10 + (int)(*p - '0');
	    if (MAX_INDEX < index) {
		return 0;
	    }
	}
	for (m = val->members.head; 0 < index && 0 != m; m = m->next, index--) {
	}
	if (0 != m) {
	    ojcVal	parent = get_aparent(m, pn, keyp);

	    if (0 != parent) {
		return parent;
	    }
	}
	return 0;
    }
    case OJC_OBJECT: {
	const char	*key;
	struct _ojcErr	err = OJC_ERR_INIT;
	ojcVal		child;

	if (0 == *pn) {
	    *keyp = *path;
	    return val;
	}
	for (m = val->members.head; 0 != m; m = m->next) {
	    key = ojc_key(m);
	    if (0 != key &&
		(ojc_case_insensitive ?
		 0 == strcasecmp(*path, key) :
		 0 == strcmp(*path, key))) {
		ojcVal	parent = get_aparent(m, pn, keyp);

		if (0 != parent) {
		    return parent;
		}
	    }
	}
	child = ojc_create_object();
	ojc_object_append(&err, val, *path, child); 
	return get_aparent(child, pn, keyp);
    }
    default:
	break;
    }
    return 0;
}


void
ojc_append(ojcErr err, ojcVal anchor, const char *path, ojcVal val) {
    const char	*key = 0;
    ojcVal	p;

    if (0 != err && OJC_OK != err->code) {
	return;
    }
    if (0 == anchor || 0 == path || 0 == val) {
	if (0 != err) {
	    err->code = OJC_ARG_ERR;
	    snprintf(err->msg, sizeof(err->msg), "NULL argument to ojc_append");
	}
	return;
    }
    p = get_parent(anchor, path, &key);
    if (0 == p || 0 == key) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Failed to get the parent node for %s", path);
	}
	return;
    }
    if (OJC_OBJECT != p->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not append to an elements of a %s", ojc_type_str(p->type));
	}
	return;
    }
    ojc_object_append(err, p, key, val);
}

void
ojc_aappend(ojcErr err, ojcVal anchor, const char **path, ojcVal val) {
    const char	*key = 0;
    ojcVal	p = get_aparent(anchor, path, &key);

    if (0 != err && OJC_OK != err->code) {
	return;
    }
    if (0 == anchor || 0 == path || 0 == val) {
	if (0 != err) {
	    err->code = OJC_ARG_ERR;
	    snprintf(err->msg, sizeof(err->msg), "NULL argument to ojc_aappend");
	}
	return;
    }
    if (0 == p || 0 == key) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Failed to get the parent node");
	}
	return;
    }
    if (OJC_OBJECT != p->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not append to an elements of a %s", ojc_type_str(p->type));
	}
	return;
    }
    ojc_object_append(err, p, key, val);
}

bool
ojc_replace(ojcErr err, ojcVal anchor, const char *path, ojcVal val) {
    return ojc_set(err, anchor, path, val);
}

bool
ojc_set(ojcErr err, ojcVal anchor, const char *path, ojcVal val) {
    const char	*key = 0;
    ojcVal	p;

    if (NULL != err && OJC_OK != err->code) {
	return false;
    }
    if (NULL == anchor || NULL == path || NULL == val) {
	if (0 != err) {
	    err->code = OJC_ARG_ERR;
	    snprintf(err->msg, sizeof(err->msg), "NULL argument to ojc_set");
	}
	return false;
    }
    p = get_parent(anchor, path, &key);
    if (NULL == p || NULL == key) {
	if (NULL != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Failed to get the parent node for %s", path);
	}
	return false;
    }
    if (OJC_OBJECT == p->type) {
	return ojc_object_replace(err, p, key, val);
    } else if (OJC_ARRAY == p->type) {
	unsigned int	pos = 0;
	const char	*k = key;

	for (; '\0' != *k; k++) {
	    if (*k < '0' || '9' < *k) {
		if (0 != err) {
		    err->code = OJC_ARG_ERR;
		    snprintf(err->msg, sizeof(err->msg), "Can not convert '%s' into an array index in ojc_set", key);
		}
		return false;
	    }
	    pos = pos * 10 + (unsigned int)(*k - '0');
	}
	if (MAX_INDEX < pos) {
	    if (0 != err) {
		err->code = OJC_ARG_ERR;
		snprintf(err->msg, sizeof(err->msg), "'%s' is too large for an array index in ojc_set", key);
	    }
	    return false;
	}
	return ojc_array_replace(err, p, pos, val);
    }
    if (0 != err) {
	err->code = OJC_TYPE_ERR;
	snprintf(err->msg, sizeof(err->msg), "Can not set an elements of a %s", ojc_type_str(p->type));
    }
    return false;
}

bool
ojc_remove(ojcErr err, ojcVal anchor, const char *path) {
    const char	*key = 0;
    ojcVal	p;

    if (0 != err && OJC_OK != err->code) {
	return 0;
    }
    if (0 == anchor || 0 == path) {
	if (0 != err) {
	    err->code = OJC_ARG_ERR;
	    snprintf(err->msg, sizeof(err->msg), "NULL argument to ojc_remove");
	}
	return false;
    }
    p = get_parent(anchor, path, &key);
    if (0 == p || 0 == key) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Failed to get the parent node for %s", path);
	}
	return false;
    }
    if (OJC_OBJECT == p->type) {
	return ojc_object_remove_by_key(err, p, key);
    } else if (OJC_ARRAY == p->type) {
	unsigned int	pos = 0;
	const char	*k = key;

	for (; '\0' != *k; k++) {
	    if (*k < '0' || '9' < *k) {
		if (0 != err) {
		    err->code = OJC_ARG_ERR;
		    snprintf(err->msg, sizeof(err->msg), "Can not convert '%s' into an array index in ojc_remove", key);
		}
		return false;
	    }
	    pos = pos * 10 + (unsigned int)(*k - '0');
	}
	if (MAX_INDEX < pos) {
	    if (0 != err) {
		err->code = OJC_ARG_ERR;
		snprintf(err->msg, sizeof(err->msg), "'%s' is too large for an array index in ojc_remove", key);
	    }
	    return false;
	}
	return ojc_remove_by_pos(err, p, pos);
    }
    if (0 != err) {
	err->code = OJC_TYPE_ERR;
	snprintf(err->msg, sizeof(err->msg), "Can not remove an elements of a %s", ojc_type_str(p->type));
    }
    return false;
}

bool
ojc_areplace(ojcErr err, ojcVal anchor, const char **path, ojcVal val) {
    return ojc_aset(err, anchor, path, val);
}

bool
ojc_aset(ojcErr err, ojcVal anchor, const char **path, ojcVal val) {
    const char	*key = 0;
    ojcVal	p = get_aparent(anchor, path, &key);

    if (0 != err && OJC_OK != err->code) {
	return false;
    }
    if (0 == anchor || 0 == path || 0 == val) {
	if (0 != err) {
	    err->code = OJC_ARG_ERR;
	    snprintf(err->msg, sizeof(err->msg), "NULL argument to ojc_aset");
	}
	return false;
    }
    if (0 == p || 0 == key) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Failed to get the parent node");
	}
	return false;
    }
    if (OJC_OBJECT == p->type) {
	return ojc_object_replace(err, p, key, val);
    } else if (OJC_ARRAY == p->type) {
	unsigned int	pos = 0;
	const char	*k = key;

	for (; '\0' != *k; k++) {
	    if (*k < '0' || '9' < *k) {
		if (0 != err) {
		    err->code = OJC_ARG_ERR;
		    snprintf(err->msg, sizeof(err->msg), "Can not convert '%s' into an array index in ojc_aset", key);
		}
		return false;
	    }
	    pos = pos * 10 + (unsigned int)(*k - '0');
	}
	if (MAX_INDEX < pos) {
	    if (0 != err) {
		err->code = OJC_ARG_ERR;
		snprintf(err->msg, sizeof(err->msg), "'%s' is too large for an array index in ojc_aset", key);
	    }
	    return false;
	}
	return ojc_array_replace(err, p, pos, val);
    }
    if (0 != err) {
	err->code = OJC_TYPE_ERR;
	snprintf(err->msg, sizeof(err->msg), "Can not set an elements of a %s", ojc_type_str(p->type));
    }
    return false;
}

bool
ojc_aremove(ojcErr err, ojcVal anchor, const char **path) {
    const char	*key = 0;
    ojcVal	p = get_aparent(anchor, path, &key);

    if (0 != err && OJC_OK != err->code) {
	return false;
    }
    if (0 == anchor || 0 == path) {
	if (0 != err) {
	    err->code = OJC_ARG_ERR;
	    snprintf(err->msg, sizeof(err->msg), "NULL argument to ojc_aremove");
	}
	return false;
    }
    if (0 == p || 0 == key) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Failed to get the parent node");
	}
	return false;
    }
    if (OJC_OBJECT == p->type) {
	return ojc_object_remove_by_key(err, p, key);
    } else if (OJC_ARRAY == p->type) {
	unsigned int	pos = 0;
	const char	*k = key;

	for (; '\0' != *k; k++) {
	    if (*k < '0' || '9' < *k) {
		if (0 != err) {
		    err->code = OJC_ARG_ERR;
		    snprintf(err->msg, sizeof(err->msg), "Can not convert '%s' into an array index in ojc_aremove", key);
		}
		return false;
	    }
	    pos = pos * 10 + (unsigned int)(*k - '0');
	}
	if (MAX_INDEX < pos) {
	    if (0 != err) {
		err->code = OJC_ARG_ERR;
		snprintf(err->msg, sizeof(err->msg), "'%s' is too large for an array index in ojc_aremove", key);
	    }
	    return false;
	}
	return ojc_remove_by_pos(err, p, pos);
    }
    if (0 != err) {
	err->code = OJC_TYPE_ERR;
	snprintf(err->msg, sizeof(err->msg), "Can not remove an elements of a %s", ojc_type_str(p->type));
    }
    return false;
}

ojcValType
ojc_type(ojcVal val) {
    if (NULL == val) {
	return OJC_NULL;
    }
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

bool
ojc_bool(ojcErr err, ojcVal val) {
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return false;
    }
    if (0 == val) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not get a boolean from NULL");
	}
	return false;
    }
    switch (val->type) {
    case OJC_TRUE:
	return true;
    case OJC_FALSE:
	return false;
    default:
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg),
		     "Can not get a boolean from a %s", ojc_type_str((ojcValType)val->type));
	}
	return false;
    }
    return false;
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

void*
ojc_opaque(ojcErr err, ojcVal val) {
    if (!is_type_ok(err, val, OJC_OPAQUE)) {
	return NULL;
    }
    return val->opaque;
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
    
    if (NULL == str) {
	val->type = OJC_NULL;
	return val;
    }
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

    if (NULL == num) {
	val->type = OJC_NULL;
	return val;
    }
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

ojcVal
ojc_create_opaque(void *opaque) {
    ojcVal	val = _ojc_val_create(OJC_OPAQUE);

    val->opaque = opaque;

    return val;
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
	    snprintf(err->msg, sizeof(err->msg), "Can not object %s on NULL", op);
	}
	return true;
    }
    if (OJC_OBJECT != object->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not object %s on a %s",
		     op, ojc_type_str((ojcValType)object->type));
	}
	return true;
    }
    return false;
}

// returns false if val has members, true if there is an error
static bool
has_no_members(ojcErr err, ojcVal val, const char *op) {
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return true;
    }
    if (0 == val) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not %s on NULL", op);
	}
	return true;
    }
    if (OJC_OBJECT != val->type && OJC_ARRAY != val->type) {
	if (0 != err) {
	    err->code = OJC_TYPE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "Can not %s on a %s",
		     op, ojc_type_str((ojcValType)val->type));
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
    val->next = NULL;
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
    ojc_object_nappend(err, object, key, strlen(key), val);
}

bool
ojc_object_replace(ojcErr err, ojcVal object, const char *key, ojcVal val) {
    ojcVal	m;
    ojcVal	prev = 0;

    if (bad_object(err, object, "replace")) {
	return false;
    }
    _ojc_set_key(val, key, 0);
    for (m = object->members.head; 0 != m; m = m->next) {
	if (ojc_case_insensitive ?
	    0 == strcasecmp(key, ojc_key(m)) :
	    0 == strcmp(key, ojc_key(m))) {
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
	    return true;
	}
	prev = m;
    }
    // nothing replaces so append
    ojc_object_append(err, object, key, val);

    return false;
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

bool
ojc_remove_by_pos(ojcErr err, ojcVal val, int pos) {
    ojcVal	m;
    ojcVal	prev = 0;
    ojcVal	next;

    if (has_no_members(err, val, "remove by position")) {
	return false;
    }
    if (0 > pos) {
	int	cnt = ojc_member_count(err, val);

	pos += cnt;
	if (0 > pos) {
	    if (0 != err) {
		err->code = OJC_ARG_ERR;
		snprintf(err->msg, sizeof(err->msg), "No element at position %d.", pos - cnt);
	    }
	    return false;
	}
    }
    for (m = val->members.head; 0 != m; m = next, pos--) {
	next = m->next;
	if (0 == pos) {
	    if (0 == prev) {
		val->members.head = m->next;
	    } else {
		prev->next = m->next;
	    }
	    if (m == val->members.tail) {
		val->members.tail = prev;
	    }
	    m->next = 0;
	    ojc_destroy(m);
	    return true;
	}
	prev = m;
    }
    if (0 != err) {
	err->code = OJC_ARG_ERR;
	snprintf(err->msg, sizeof(err->msg), "No element at position %d.", pos);
    }
    return false;
}

ojcVal
ojc_object_take(ojcErr err, ojcVal object, const char *key) {
    ojcVal	m;
    ojcVal	prev = 0;
    ojcVal	next;

    if (bad_object(err, object, "take by key")) {
	return NULL;
    }
    for (m = object->members.head; 0 != m; m = next) {
	next = m->next;
	if (ojc_case_insensitive ?
	    0 == strcasecmp(key, ojc_key(m)) :
	    0 == strcmp(key, ojc_key(m))) {
	    if (0 == prev) {
		object->members.head = m->next;
	    } else {
		prev->next = m->next;
	    }
	    if (m == object->members.tail) {
		object->members.tail = prev;
	    }
	    m->next = 0;
	    break;
	}
	prev = m;
    }
    return m;
}

bool
ojc_object_remove_by_key(ojcErr err, ojcVal object, const char *key) {
    ojcVal	m;
    ojcVal	prev = 0;
    ojcVal	next;

    if (bad_object(err, object, "remove by key")) {
	return false;
    }
    for (m = object->members.head; 0 != m; m = next) {
	next = m->next;
	if (ojc_case_insensitive ?
	    0 == strcasecmp(key, ojc_key(m)) :
	    0 == strcmp(key, ojc_key(m))) {
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
    return (NULL != m);
}

ojcVal
ojc_object_get_by_key(ojcErr err, ojcVal object, const char *key) {
    ojcVal	m;

    if (bad_object(err, object, "get by key")) {
	return 0;
    }
    for (m = object->members.head; 0 != m; m = m->next) {
	if (ojc_case_insensitive ?
	    0 == strcasecmp(key, ojc_key(m)) :
	    0 == strcmp(key, ojc_key(m))) {
	    break;
	}
    }
    return m;
}

void
ojc_merge(ojcErr err, ojcVal primary, ojcVal other) {
    ojcVal	m;
    ojcVal	pm;
    const char	*key;

    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return;
    }
    if (ojc_type(primary) != ojc_type(other)) {
	err->code = OJC_TYPE_ERR;
	snprintf(err->msg, sizeof(err->msg), "Can not merge elements of different type. %s vs %s",
		 ojc_type_str((ojcValType)primary->type), ojc_type_str((ojcValType)other->type));
	return;
    }
    if (OJC_OBJECT == ojc_type(primary)) {
	for (m = other->members.head; NULL != m; m = m->next) {
	    key = ojc_key(m);
	    if (NULL == (pm = ojc_object_get_by_key(err, primary, key))) {
		ojc_object_append(err, primary, key, ojc_duplicate(m));
	    } else if (m->type != pm->type) {
		if ((OJC_TRUE == m->type && OJC_FALSE == pm->type) ||
		    (OJC_FALSE == m->type && OJC_TRUE == pm->type)) {
		    pm->type = m->type;
		} else {
		    if (NULL != err) {
			err->code = OJC_TYPE_ERR;
			snprintf(err->msg, sizeof(err->msg), "Can not merge when types do not match for %s.", key);
		    }
		    return;
		}
	    } else {
		switch (ojc_type(m)) {
		case OJC_STRING:
		case OJC_NUMBER:
		case OJC_FIXNUM:
		case OJC_DECIMAL:
		case OJC_WORD:
		case OJC_OPAQUE:
		    ojc_object_replace(err, primary, key, ojc_duplicate(m));
		    break;
		case OJC_OBJECT:
		    ojc_merge(err, pm, m);
		    break;
		case OJC_ARRAY:
		    for (ojcVal am = m->members.head; NULL != am; am = am->next) {
			ojc_array_append(NULL, pm, ojc_duplicate(am));
		    }
		    break;
		default:
		    break;
		}
	    }
	}
    } else if (OJC_ARRAY == ojc_type(primary)) {
	bool	found;
	ojcVal	add = NULL;
	ojcVal	add_end = NULL;
	ojcVal	x;
	
	for (m = other->members.head; NULL != m; m = m->next) {
	    found = false;
	    for (pm = primary->members.head; NULL != pm; pm = pm->next) {
		if (ojc_equals(m, pm)) {
		    found = true;
		    break;
		}
	    }
	    if (!found) {
		x = ojc_duplicate(m);
		if (NULL == add_end) {
		    add = x;
		    add_end = add;
		} else {
		    add_end->next = x;
		    add_end = x;
		}
	    }
	}
	while (NULL != (x = add)) {
	    ojc_array_append(NULL, primary, x);
	    add = add->next;
	}
    } else {
	err->code = OJC_TYPE_ERR;
	snprintf(err->msg, sizeof(err->msg), "Can not merge %s elements", ojc_type_str((ojcValType)primary->type));
	return;
    }
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
	    array->members.tail = NULL;
	}
	val->next = 0;

	return val;
    }
    return 0;
}

bool
ojc_array_replace(ojcErr err, ojcVal array, int pos, ojcVal val) {
    ojcVal	m;
    ojcVal	prev = 0;

    if (bad_array(err, array, "replace")) {
	return false;
    }
    if (0 > pos) {
	pos += ojc_member_count(err, array);
    }
    if (0 > pos) {
	ojc_array_push(err, array, val);
	return false;
    }
    for (m = array->members.head; 0 != m; m = m->next, pos--) {
	if (0 == pos) {
	    val->next = m->next;
	    if (0 == prev) {
		array->members.head = val;
	    } else {
		prev->next = val;
	    }
	    if (0 == val->next) {
		array->members.tail = val;
	    }
	    m->next = 0;
	    ojc_destroy(m);
	    return true;
	}
	prev = m;
    }
    ojc_array_append(err, array, val);

    return false;
}

void
ojc_array_insert(ojcErr err, ojcVal array, int pos, ojcVal val) {
    ojcVal	m;
    ojcVal	prev = 0;

    if (bad_array(err, array, "insert")) {
	return;
    }
    if (0 > pos) {
	pos += ojc_member_count(err, array) + 1;
    }
    if (0 > pos) {
	ojc_array_push(err, array, val);
	return;
    }
    for (m = array->members.head; 0 != m; m = m->next, pos--) {
	if (0 == pos) {
	    if (0 == prev) {
		val->next = array->members.head;
		array->members.head = val;
	    } else {
		prev->next = val;
		val->next = m;
	    }
	    return;
	}
	prev = m;
    }
    ojc_array_append(err, array, val);
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
opaque_fill(Buf bb, void *v) {
    char	buf[32];
    char	*end = buf + sizeof(buf) - 1;
    char	*b = end;
    uint64_t	num = (uint64_t)v;

    *b-- = '\0';
    if (0 < num) {
	for (; 0 < num; num /= 10, b--) {
	    *b = (num % 10) + '0';
	}
	b++;
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
    case OJC_OPAQUE:
	if (ojc_write_opaque) {
	    opaque_fill(buf, val->opaque);
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

    int	cnt = buf_len(&b);

    return cnt;
}

int
ojc_write(ojcErr err, ojcVal val, int indent, int socket) {
    struct _Buf	b;
    
    if (0 != err && OJC_OK != err->code) {
	// Previous call must have failed or err was not initialized.
	return -1;
    }
    buf_init(&b, socket);
    fill_buf(&b, val, indent, 0);
    if (ojc_write_end_with_newline) {
	buf_append(&b, '\n');
    }
    int	cnt = buf_len(&b);

    buf_finish(&b);
    buf_cleanup(&b);

    if (OJC_OK != b.err) {
	err->code = b.err;
	strcpy(err->msg, ojc_error_str((ojcErrCode)b.err));
    }
    return cnt;
}

int
ojc_fwrite(ojcErr err, ojcVal val, int indent, FILE *file) {
    return ojc_write(err, val, indent, fileno(file));
}

ojcVal
ojc_duplicate(ojcVal val) {
    ojcVal		dup;
    struct _ojcErr	err = OJC_ERR_INIT;

    if (NULL == val) {
	return NULL;
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
    case OJC_OPAQUE:
	dup->opaque = val->opaque;
	break;
    default:
	break;
    }
    return dup;
}

bool
ojc_equals(ojcVal v1, ojcVal v2) {
    struct _ojcErr	err = OJC_ERR_INIT;

    if (ojc_type(v1) != ojc_type(v2)) {
	return false;
    }
    switch (ojc_type(v1)) {
    case OJC_STRING: {
	const char	*s1 = ojc_str(&err, v1);
	const char	*s2 = ojc_str(&err, v2);

	if (NULL == s1) {
	    s1 = "";
	}
	if (NULL == s2) {
	    s2 = "";
	}
	return 0 == (ojc_case_insensitive ? strcasecmp(s1, s2) : strcmp(s1, s2));
    }
    case OJC_NUMBER: {
	const char	*s1 = ojc_number(&err, v1);
	const char	*s2 = ojc_number(&err, v2);

	if (NULL == s1) {
	    s1 = "";
	}
	if (NULL == s2) {
	    s2 = "";
	}
	return 0 == strcmp(s1, s2);
    }
    case OJC_FIXNUM:
	return ojc_int(&err, v1) == ojc_int(&err, v2);
    case OJC_DECIMAL:
	return ojc_double(&err, v1) == ojc_double(&err, v2);
    case OJC_ARRAY: {
	ojcVal	m1 = ojc_members(&err, v1);
	ojcVal	m2 = ojc_members(&err, v2);

	for (; 0 != m1 && 0 != m2; m1 = m1->next, m2 = m2->next) {
	    if (!ojc_equals(m1, m2)) {
		return false;
	    }
	}
	if (NULL != m1 || NULL != m2) {
	    return false;
	}
	return true;
    }
    case OJC_OBJECT: {
	ojcVal	m1 = ojc_members(&err, v1);
	ojcVal	m2 = ojc_members(&err, v2);

	for (; 0 != m1 && 0 != m2; m1 = m1->next, m2 = m2->next) {
	    if ((0 != (ojc_case_insensitive ?
		       strcasecmp(ojc_key(m1), ojc_key(m2)) :
		       strcmp(ojc_key(m1), ojc_key(m2)))) ||
		!ojc_equals(m1, m2)) {
		return false;
	    }
	}
	if (NULL != m1 || NULL != m2) {
	    return false;
	}
	return true;
    }
    case OJC_WORD: {
	const char	*s1 = ojc_word(&err, v1);
	const char	*s2 = ojc_word(&err, v2);

	if (NULL == s1) {
	    s1 = "";
	}
	if (NULL == s2) {
	    s2 = "";
	}
	return 0 == (ojc_case_insensitive ? strcasecmp(s1, s2) : strcmp(s1, s2));
    }
    case OJC_OPAQUE:
	return v1->opaque == v2->opaque;
    case OJC_TRUE:
    case OJC_FALSE:
    case OJC_NULL:
    default:
	return true;
    }
    return false;
}

int
ojc_cmp(ojcVal v1, ojcVal v2) {
    struct _ojcErr	err = OJC_ERR_INIT;

    switch (ojc_type(v1)) {
    case OJC_STRING: {
	const char	*s1 = ojc_str(&err, v1);
	const char	*s2 = NULL;

	switch (ojc_type(v2)) {
	case OJC_STRING:
	    s2 = ojc_str(&err, v2);
	    break;
	case OJC_WORD:
	    s2 = ojc_word(&err, v2);
	    break;
	default:
	    return (int)ojc_type(v1) - (int)ojc_type(v2);
	}
	if (NULL == s1) {
	    s1 = "";
	}
	if (NULL == s2) {
	    s2 = "";
	}
	return (ojc_case_insensitive ? strcasecmp(s1, s2) : strcmp(s1, s2));
    }
    case OJC_NUMBER: {
	// TBD convert to int or double
	break;
    }
    case OJC_FIXNUM: {
	int64_t	n1 = ojc_int(&err, v1);

	switch (ojc_type(v2)) {
	case OJC_FIXNUM:
	    return n1 - ojc_int(&err, v2);
	case OJC_DECIMAL: {
	    double	d = (double)n1 - ojc_double(&err, v2);

	    return (0.0 == d) ? 0 : (0.0 < d) ? 1 : -1;
	}
	case OJC_NUMBER: {
	    char	*end;
	    const char	*ns = ojc_number(&err, v2);
	    int64_t	n2 = strtoll(ns, &end, 10);

	    if (n1 != n2) {
		return n1 - n2;
	    }
	    if ('\0' != *end) {
		if ('.' == *end) {
		    end++;
		    for (; '\0' != *end; end++) {
			if ('0' != *end) {
			    return (0 <= n1 ? -1 : 1);
			}
		    }
		}

	    }
	    return 0;
	}
	default:
	    return (int)ojc_type(v1) - (int)ojc_type(v2);
	}
    }
    case OJC_DECIMAL: {
	double	d1 = ojc_double(&err, v1);
	double	d2 = 0.0;

	switch (ojc_type(v2)) {
	case OJC_FIXNUM:
	    d2 = (double)ojc_int(&err, v2);
	    break;
	case OJC_DECIMAL: {
	    d2 = ojc_double(&err, v2);
	    break;
	}
	case OJC_NUMBER: {
	    char	*end;
	    const char	*ns = ojc_number(&err, v2);

	    d2 = strtod(ns, &end);
	    if ('\0' != *end) {
		return (int)ojc_type(v1) - (int)ojc_type(v2);
	    }
	}
	default:
	    return (int)ojc_type(v1) - (int)ojc_type(v2);
	}
	double	d = d1 - d2;

	return (0.0 == d) ? 0 : (0.0 < d) ? 1 : -1;
    }
    case OJC_ARRAY: {
	ojcVal	m1 = ojc_members(&err, v1);
	ojcVal	m2 = ojc_members(&err, v2);
	int	x;

	for (; 0 != m1 && 0 != m2; m1 = m1->next, m2 = m2->next) {
	    if (0 != (x = ojc_cmp(m1, m2))) {
		return x;
	    }
	}
	if (0 != m1) {
	    return 1;
	}
	if (0 != m2) {
	    return -1;
	}
	break;
    }
    case OJC_OBJECT: {
	ojcVal	m1 = ojc_members(&err, v1);
	ojcVal	m2 = ojc_members(&err, v2);
	int	x;

	for (; 0 != m1 && 0 != m2; m1 = m1->next, m2 = m2->next) {
	    if (0 != (x = (ojc_case_insensitive ?
			   strcasecmp(ojc_key(m1), ojc_key(m2)) :
			   strcmp(ojc_key(m1), ojc_key(m2)))) ||
		0 != (x = ojc_cmp(m1, m2))) {
		return x;
	    }
	}
	if (0 != m1) {
	    return 1;
	}
	if (0 != m2) {
	    return -1;
	}
	break;
    }
    case OJC_WORD: {
	const char	*s1 = ojc_word(&err, v1);
	const char	*s2 = NULL;

	switch (ojc_type(v2)) {
	case OJC_STRING:
	    s2 = ojc_str(&err, v2);
	    break;
	case OJC_WORD:
	    s2 = ojc_word(&err, v2);
	    break;
	default:
	    return (int)ojc_type(v1) - (int)ojc_type(v2);
	}
	if (NULL == s1) {
	    s1 = "";
	}
	if (NULL == s2) {
	    s2 = "";
	}
	return (ojc_case_insensitive ? strcasecmp(s1, s2) : strcmp(s1, s2));
    }
    case OJC_OPAQUE:
	return (int)((int64_t)v1->opaque - (int64_t)v2->opaque);
    case OJC_TRUE:
    case OJC_FALSE:
    case OJC_NULL:
    default:
	return (int)ojc_type(v1) - (int)ojc_type(v2);
    }
    return 0;
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
    case OJC_OPAQUE:	return "opaque";
    default:		return "unknown";
    }
}

const char*
ojc_error_str(ojcErrCode code) {
    switch (code) {
    case OJC_OK:		return "ok";
    case OJC_TYPE_ERR:		return "type error";
    case OJC_PARSE_ERR:		return "parse error";
    case OJC_INCOMPLETE_ERR:	return "incomplete parse error";
    case OJC_OVERFLOW_ERR:	return "overflow error";
    case OJC_WRITE_ERR:		return "write error";
    case OJC_MEMORY_ERR:	return "memory error";
    case OJC_UNICODE_ERR:	return "unicode error";
    case OJC_ABORT_ERR:		return "abort";
    case OJC_ARG_ERR:		return "argument error";
    default:			return "unknown";
    }
}
