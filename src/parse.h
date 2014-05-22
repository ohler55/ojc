/* parse.h
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

#ifndef __OJC_PARSE_H__
#define __OJC_PARSE_H__

#include <stdarg.h>
#include <stdio.h>

#include "ojc.h"
#include "reader.h"
#include "val_stack.h"

typedef struct _ParseInfo {
    struct _Reader	rd;
    struct _ojcErr	err;
    //struct _Options	options;
    struct _ValStack	stack;
    ojcParseCallback	each_cb;
    void		*each_ctx;
    struct _List	free_vals;
    char		*key;
    char		karray[256];
    size_t		klen;
    bool		kalloc;
} *ParseInfo;

extern void	ojc_parse(ParseInfo pi);

inline static void
parse_init(ojcErr err, ParseInfo pi, ojcParseCallback cb, void *ctx) {
    ojc_err_init(&pi->err);
    pi->each_cb = cb;
    pi->each_ctx = ctx;
    pi->free_vals.head = 0;
    pi->free_vals.tail = 0;
    pi->key = 0;
    pi->klen = 0;
    pi->kalloc = false;
    stack_init(&pi->stack);
}


inline static void
parse_cleanup(ParseInfo pi) {
    reader_cleanup(&pi->rd);
    stack_cleanup(&pi->stack);
    _ojc_val_return(&pi->free_vals);
}

#endif /* __OJC_PARSE_H__ */
