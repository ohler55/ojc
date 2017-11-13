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

#ifndef __OJC_TYPES_H__
#define __OJC_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

    struct _Buf;
    
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
	OJC_WORD	= 'w',
	OJC_OPAQUE	= 'i',
	OJC_FREE	= '#',
    } ojcValType;

    typedef enum {
	OJC_OK			= 0,
	OJC_TYPE_ERR		= 't',
	OJC_PARSE_ERR		= 'p',
	OJC_INCOMPLETE_ERR	= 'i',
	OJC_OVERFLOW_ERR	= 'o',
	OJC_WRITE_ERR		= 'w',
	OJC_MEMORY_ERR		= 'm',
	OJC_UNICODE_ERR		= 'u',
	OJC_ABORT_ERR		= '@',
	OJC_ARG_ERR		= 'a',
    } ojcErrCode;

    typedef struct _ojcErr {
	int	code;
	char	msg[256];
    } *ojcErr;

    typedef struct _ojcVal	*ojcVal;
    typedef bool		(*ojcParseCallback)(ojcErr err, ojcVal val, void *ctx);
    typedef ssize_t		(*ojcReadFunc)(void *src, char *buf, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* __OJC_TYPES_H__ */
