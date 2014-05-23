/* val.h
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

#ifndef __OJC_VAL_H__
#define __OJC_VAL_H__

#include <stdint.h>

#include "ojc.h"

#define STR_NONE	'\0'
#define STR_PTR		'p'
#define STR_ARRAY	'a'
#define STR_1K		'1'
#define STR_4K		'4'

typedef enum {
    NEXT_NONE		= 0,
    NEXT_ARRAY_NEW	= 'a',
    NEXT_ARRAY_ELEMENT	= 'e',
    NEXT_ARRAY_COMMA	= ',',
    NEXT_OBJECT_NEW	= 'h',
    NEXT_OBJECT_KEY	= 'k',
    NEXT_OBJECT_COLON	= ':',
    NEXT_OBJECT_VALUE	= 'v',
    NEXT_OBJECT_COMMA	= 'n',
} ValNext;

union _Key {
    struct {
	char	*str;
	size_t	len;
    };
    // first char is the length
    char	ca[16];
};

union _Str {
    struct {
	char	*str;
	size_t	len;
    };
    // first char is the length
    char	ca[32];
};

typedef struct _List {
    struct _ojcVal	*head;
    struct _ojcVal	*tail;
} *List;

struct _ojcVal {
    struct _ojcVal	*next;
    union _Key		key;
    union {
	union _Str	str;
	struct _List	members;
	int64_t		fixnum;
	double		dub;
    };
    uint8_t		key_type;
    uint8_t		str_type;
    uint8_t		type;	// ojcValType
    uint8_t		expect; // ValNext
};

extern ojcVal	_ojc_val_create(ojcValType type);
extern void	_ojc_destroy(ojcVal val);
extern void	_ojc_val_return(List freed);
extern void	_ojc_val_destroy(ojcVal val, List freed);
extern void	_ojc_val_create_batch(size_t cnt, List vals);

#endif /* __OJC_VAL_H__ */
