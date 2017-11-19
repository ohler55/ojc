/* ojc.h
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

#ifndef __OJC_H__
#define __OJC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "types.h"

/**
 * Current version of OjC.
 */
#define OJC_VERSION	"3.1.0"
#define OJC_ERR_INIT	{ 0, { 0 } }

    extern bool		ojc_newline_ok;
    extern bool		ojc_word_ok;
    extern bool		ojc_decimal_as_number;
    extern bool		ojc_case_insensitive;
    extern bool		ojc_write_opaque;
    extern bool		ojc_write_end_with_newline;

    extern const char*	ojc_version(void);
    extern void		ojc_cleanup(void);
    extern ojcVal	ojc_parse_str(ojcErr err, const char *json, ojcParseCallback cb, void *ctx);
    extern ojcVal	ojc_parse_strp(ojcErr err, const char **jsonp);
    extern ojcVal	ojc_parse_file(ojcErr err, FILE *file, ojcParseCallback cb, void *ctx);
    extern ojcVal	ojc_parse_fd(ojcErr err, int socket, ojcParseCallback cb, void *ctx);
    extern void		ojc_parse_file_follow(ojcErr err, FILE *file, ojcParseCallback cb, void *ctx);
    extern ojcVal	ojc_parse_reader(ojcErr err, void *src, ojcReadFunc rf, ojcParseCallback cb, void *ctx);
    extern int		ojc_destroy(ojcVal val);
    extern ojcVal	ojc_get(ojcVal val, const char *path);
    extern ojcVal	ojc_aget(ojcVal val, const char **path);
    extern void		ojc_append(ojcErr err, ojcVal anchor, const char *path, ojcVal val);
    extern void		ojc_aappend(ojcErr err, ojcVal anchor, const char **path, ojcVal val);
    extern bool		ojc_replace(ojcErr err, ojcVal anchor, const char *path, ojcVal val);
    extern bool		ojc_set(ojcErr err, ojcVal anchor, const char *path, ojcVal val);
    extern bool		ojc_remove(ojcErr err, ojcVal anchor, const char *path);
    extern bool		ojc_areplace(ojcErr err, ojcVal anchor, const char **path, ojcVal val);
    extern bool		ojc_aset(ojcErr err, ojcVal anchor, const char **path, ojcVal val);
    extern bool		ojc_aremove(ojcErr err, ojcVal anchor, const char **path);
    extern ojcValType	ojc_type(ojcVal val);
    extern bool		ojc_bool(ojcErr err, ojcVal val);
    extern int64_t	ojc_int(ojcErr err, ojcVal val);
    extern double	ojc_double(ojcErr err, ojcVal val);
    extern const char*	ojc_number(ojcErr err, ojcVal val);
    extern int		ojc_number_len(ojcErr err, ojcVal val);
    extern const char*	ojc_str(ojcErr err, ojcVal val);
    extern int		ojc_str_len(ojcErr err, ojcVal val);
    extern const char*	ojc_word(ojcErr err, ojcVal val);
    extern void*	ojc_opaque(ojcErr err, ojcVal val);
    extern ojcVal	ojc_members(ojcErr err, ojcVal val);
    extern ojcVal	ojc_next(ojcVal val);
    extern int		ojc_member_count(ojcErr err, ojcVal val);
    extern bool		ojc_has_key(ojcVal val);
    extern const char*	ojc_key(ojcVal val);
    extern int		ojc_key_len(ojcVal val);
    extern void		ojc_set_key(ojcVal val, const char *key);
    extern ojcVal	ojc_create_object(void);
    extern ojcVal	ojc_create_array(void);
    extern ojcVal	ojc_create_str(const char *str, int len);
    extern ojcVal	ojc_create_word(const char *str, int len);
    extern ojcVal	ojc_create_int(int64_t num);
    extern ojcVal	ojc_create_double(double num);
    extern ojcVal	ojc_create_number(const char *num, int len);
    extern ojcVal	ojc_create_null(void);
    extern ojcVal	ojc_create_bool(bool boo);
    extern ojcVal	ojc_create_opaque(void *opaque);
    extern void		ojc_object_append(ojcErr err, ojcVal object, const char *key, ojcVal val);
    extern void		ojc_object_nappend(ojcErr err, ojcVal object, const char *key, int klen, ojcVal val);
    extern bool		ojc_object_replace(ojcErr err, ojcVal object, const char *key, ojcVal val);
    extern void		ojc_object_insert(ojcErr err, ojcVal object, int before, const char *key, ojcVal val);
    extern bool		ojc_remove_by_pos(ojcErr err, ojcVal object, int pos);
    extern bool		ojc_object_remove_by_key(ojcErr err, ojcVal object, const char *key);
    extern ojcVal	ojc_object_take(ojcErr err, ojcVal object, const char *key);
    extern ojcVal	ojc_object_get_by_key(ojcErr err, ojcVal object, const char *key);
    extern void		ojc_merge(ojcErr err, ojcVal primary, ojcVal other);
    extern ojcVal	ojc_get_member(ojcErr err, ojcVal val, int pos);
    extern void		ojc_array_append(ojcErr err, ojcVal array, ojcVal val);
    extern void		ojc_array_push(ojcErr err, ojcVal array, ojcVal val);
    extern ojcVal	ojc_array_pop(ojcErr err, ojcVal array);
    extern bool		ojc_array_replace(ojcErr err, ojcVal array, int pos, ojcVal val);
    extern void		ojc_array_insert(ojcErr err, ojcVal array, int pos, ojcVal val);
    extern char*	ojc_to_str(ojcVal val, int indent);
    extern int		ojc_fill(ojcErr err, ojcVal val, int indent, char *buf, int len);
    extern void		ojc_buf(struct _Buf *buf, ojcVal val, int indent, int depth);
    extern int		ojc_write(ojcErr err, ojcVal val, int indent, int socket);
    extern int		ojc_fwrite(ojcErr err, ojcVal val, int indent, FILE *file);
    extern ojcVal	ojc_duplicate(ojcVal val);
    extern bool		ojc_equals(ojcVal v1, ojcVal v2);
    extern int		ojc_cmp(ojcVal v1, ojcVal v2);
    extern const char*	ojc_type_str(ojcValType type);
    extern const char*	ojc_error_str(ojcErrCode code);

    static inline void
    ojc_err_init(ojcErr err) {
	err->code = OJC_OK;
	*err->msg = '\0';
    }

#ifdef __cplusplus
}
#endif
#endif /* __OJC_H__ */
