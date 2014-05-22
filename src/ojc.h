/* ojc.h
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

#ifndef __OJC_H__
#define __OJC_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OJC_NULL	= 0,
    OJC_STRING	= 's',
    OJC_NUMBER	= 'N',
    OJC_FIXNUM	= 'n',
    OJC_DECIMAL	= 'd',
    OJC_TRUE	= 't',
    OJC_FALSE	= 'f',
    OJC_ARRAY	= 'a',
    OJC_OBJECT	= 'o',
} ojcValType;

typedef enum {
    OJC_OK		= 0,
    OJC_TYPE_ERR	= 't',
    OJC_PARSE_ERR	= 'p',
    OJC_OVERFLOW_ERR	= 'o',
} ojcErrCode;

typedef struct _ojcErr {
    int		code; // TBD make enum
    char	msg[256];
} *ojcErr;

typedef struct _ojcVal	*ojcVal;

// If true is returned val should be freed. If false then let the caller or
// callback deal with it.
typedef bool	(*ojcParseCallback)(ojcVal val, void *ctx);

extern ojcVal		ojc_parse_str(ojcErr err, const char *json, ojcParseCallback cb, void *ctx);
extern ojcVal		ojc_parse_stream(ojcErr err, FILE *file, ojcParseCallback cb, void *ctx);
extern ojcVal		ojc_parse_socket(ojcErr err, int socket, ojcParseCallback cb, void *ctx);

extern void		ojc_destroy(ojcVal val);

extern ojcVal		ojc_get(ojcVal val, const char *path);
extern ojcVal		ojc_aget(ojcVal val, const char **path);

extern ojcValType	ojc_type(ojcVal val);
extern int64_t		ojc_int(ojcErr err, ojcVal val);
extern double		ojc_double(ojcErr err, ojcVal val);
extern ojcVal		ojc_members(ojcErr err, ojcVal val);
extern ojcVal		ojc_next(ojcVal val);
extern int		ojc_member_count(ojcErr err, ojcVal val);
extern bool		ojc_has_key(ojcVal val);
extern const char*	ojc_key(ojcVal val);

extern ojcVal		ojc_create_object(void);
extern ojcVal		ojc_create_array(void);
extern ojcVal		ojc_create_str(const char *str, size_t len);
extern ojcVal		ojc_create_int(int64_t num);
extern ojcVal		ojc_create_double(double num);
extern ojcVal		ojc_create_number(const char *num, size_t len);
extern ojcVal		ojc_create_null(void);
extern ojcVal		ojc_create_bool(bool boo);

extern void		ojc_object_append(ojcErr err, ojcVal object, const char *key, ojcVal val);
extern void		ojc_object_nappend(ojcErr err, ojcVal object, const char *key, int klen, ojcVal val);
extern void		ojc_array_append(ojcErr err, ojcVal array, ojcVal val);

extern char*		ojc_to_str(ojcVal val, int indent);
extern int		ojc_fill(ojcErr err, ojcVal val, int indent, char *buf, size_t len);
extern void		ojc_write(ojcErr err, ojcVal val, int indent, int socket);
extern void		ojc_fwrite(ojcErr err, ojcVal val, int indent, FILE *file);

extern const char*	ojc_type_str(ojcValType type);

// TBD stream write functions someday

static inline void
ojc_err_init(ojcErr err) {
    err->code = OJC_OK;
    *err->msg = '\0';
}

#endif /* __OJC_H__ */
