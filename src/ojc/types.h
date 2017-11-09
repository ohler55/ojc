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
    
/**
 * The value types found in JSON documents and in the __ojcVal__ type.
 * @see ojcVal
 */
typedef enum {
    /** null */
    OJC_NULL	= 0,
    /** string */
    OJC_STRING	= 's',
    /** number as a string */
    OJC_NUMBER	= 'N',
    /** integer */
    OJC_FIXNUM	= 'n',
    /** decimal number */
    OJC_DECIMAL	= 'd',
    /** true */
    OJC_TRUE	= 't',
    /** false */
    OJC_FALSE	= 'f',
    /** array */
    OJC_ARRAY	= 'a',
    /** object */
    OJC_OBJECT	= 'o',
    /** word with a maximum of 15 characters */
    OJC_WORD	= 'w',
    /** opaque which is ignored on write */
    OJC_OPAQUE	= 'i',
    /** freed */
    OJC_FREE	= '#',
} ojcValType;

/**
 * Error codes for the __code__ field in __ojcErr__ structs.
 * @see ojcErr
 */
typedef enum {
    /** okay, no error */
    OJC_OK		= 0,
    /** type error */
    OJC_TYPE_ERR	= 't',
    /** parse error */
    OJC_PARSE_ERR	= 'p',
    /** incomplete parse error */
    OJC_INCOMPLETE_ERR	= 'i',
    /** buffer overflow error */
    OJC_OVERFLOW_ERR	= 'o',
    /** write error */
    OJC_WRITE_ERR	= 'w',
    /** memory error */
    OJC_MEMORY_ERR	= 'm',
    /** unicode error */
    OJC_UNICODE_ERR	= 'u',
    /** abort callback parsing */
    OJC_ABORT_ERR	= '@',
    /** argument error */
    OJC_ARG_ERR		= 'a',
} ojcErrCode;

/**
 * The struct used to report errors or status after a function returns. The
 * struct must be initialized before use as most calls that take an err argument
 * will return immediately if an error has already occurred.
 *
 * @see ojcErrCode
 */
typedef struct _ojcErr {
    /** Error code identifying the type of error. */
    int		code;
    /** Error message associated with a failure if the code is not __OJC_OK__. */
    char	msg[256];
} *ojcErr;

/**
 * The type representing a value in a JSON file. Values will be one of the
 * __ojcValType__ enum.
 * @see ojcValType
 */
typedef struct _ojcVal	*ojcVal;

/**
 * When using the parser callbacks, the callback must be of this type. The
 * arguments will be the context provided when calling the parser and the value
 * extracted from the JSON stream.
 *
 * A return value of __true__ indicates the parser should free the values after
 * the callback returns. If the return value is false then the callback is
 * responsible for freeing the value when no longer needed.
 *
 * @see ojc_parse_str, ojc_parse_stream, ojc_parse_socket
 */
typedef bool	(*ojcParseCallback)(ojcErr err, ojcVal val, void *ctx);

/**
 * Function type to use when providing a custom reader to the parser. Useful for
 * hooking up zlib or similar libraries.
 *
 * @see ojc_parse_reader
 */
typedef ssize_t	(*ojcReadFunc)(void *src, char *buf, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* __OJC_TYPES_H__ */
