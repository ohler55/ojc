/* val_stack.h
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

#ifndef __OJC_VAL_STACK_H__
#define __OJC_VAL_STACK_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "val.h"

#define STACK_INC	128

typedef struct _ValStack {
    ojcVal	base[STACK_INC];
    ojcVal	*head;	// current stack
    ojcVal	*end;	// stack end
    ojcVal	*tail;	// pointer to one past last element name on stack
} *ValStack;

extern const char*	_ojc_stack_next_str(ValNext n);

inline static void
stack_init(ValStack stack) {
    stack->head = stack->base;
    stack->end = stack->base + STACK_INC;
    stack->tail = stack->head;
    *stack->head = 0;
}

inline static int
stack_empty(ValStack stack) {
    return (stack->head == stack->tail);
}

inline static void
stack_cleanup(ValStack stack) {
    if (stack->base != stack->head) {
        free(stack->head);
    }
}

inline static void
stack_push(ValStack stack, ojcVal val) {
    if (stack->end <= stack->tail) {
	size_t	len = stack->end - stack->head;
	size_t	toff = stack->tail - stack->head;
	ojcVal	*head = stack->head;

	if (stack->base == stack->head) {
	    head = (ojcVal*)malloc(sizeof(ojcVal) * (len + STACK_INC));
	    memcpy(head, stack->base, sizeof(ojcVal) * len);
	} else {
	    head = (ojcVal*)realloc(head, sizeof(ojcVal) * (len + STACK_INC));
	}
	stack->head = head;
	stack->tail = stack->head + toff;
	stack->end = stack->head + len + STACK_INC;
    }
    switch (val->type) {
    case OJC_ARRAY:	val->expect = NEXT_ARRAY_NEW;	break;
    case OJC_OBJECT:	val->expect = NEXT_OBJECT_NEW;	break;
    default:		val->expect = NEXT_NONE;	break;
    }
    *stack->tail = val;
    stack->tail++;
}

inline static size_t
stack_size(ValStack stack) {
    return stack->tail - stack->head;
}

inline static ojcVal
stack_peek(ValStack stack) {
    if (stack->head < stack->tail) {
	return *(stack->tail - 1);
    }
    return 0;
}

inline static ojcVal
stack_peek_up(ValStack stack) {
    if (stack->head < stack->tail - 1) {
	return *(stack->tail - 2);
    }
    return 0;
}

inline static ojcVal
stack_prev(ValStack stack) {
    return *stack->tail;
}

inline static ojcVal
stack_head(ValStack stack) {
    return *stack->head;
}

inline static ojcVal
stack_pop(ValStack stack) {
    if (stack->head < stack->tail) {
	stack->tail--;
	return *stack->tail;
    }
    return 0;
}

#endif /* __OJC_VAL_STACK_H__ */
