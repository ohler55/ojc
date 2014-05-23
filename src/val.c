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

#include <stdlib.h>
#include <pthread.h>

#include "val.h"

static struct _List	free_vals = { 0, 0 };
static pthread_mutex_t	free_val_mutex = PTHREAD_MUTEX_INITIALIZER;

ojcVal
_ojc_val_create(ojcValType type) {
    ojcVal	val;

    if (free_vals.head == free_vals.tail) {
	val = (ojcVal)malloc(sizeof(struct _ojcVal));
    } else {
	pthread_mutex_lock(&free_val_mutex);
	if (free_vals.head == free_vals.tail) {
	    val = (ojcVal)malloc(sizeof(struct _ojcVal));
	} else {
	    val = free_vals.head;
	    free_vals.head = free_vals.head->next;
	}
	pthread_mutex_unlock(&free_val_mutex);
    }
    val->next = 0;
    val->key_type = STR_NONE;
    val->str_type = STR_NONE;
    val->members.head = 0;
    val->members.tail = 0;
    val->type = type;
    val->expect = NEXT_NONE;

    return val;
}

void
_ojc_val_create_batch(size_t cnt, List vals) {
    ojcVal	v;

    if (free_vals.head != free_vals.tail) {
	pthread_mutex_lock(&free_val_mutex);
	if (free_vals.head != free_vals.tail) {
	    ojcVal	prev = free_vals.head;

	    vals->head = prev;
	    for (v = prev; 0 < cnt && v != free_vals.tail; cnt--, v = v->next) {
		prev = v;
	    }
	    free_vals.head = v;
	    vals->tail = prev;
	    vals->tail->next = 0;
	}
	pthread_mutex_unlock(&free_val_mutex);
    }
    for (; 0 < cnt; cnt--) {
	v = _ojc_val_create(OJC_NULL);
	if (0 == vals->head) {
	    vals->head = v;
	} else {
	    vals->tail->next = v;
	}
	vals->tail = v;
    }
}

void
_ojc_val_destroy(ojcVal val, List freed) {
    if (STR_PTR == val->key_type) {
	free(val->key.str);
    }
    if (OJC_STRING == val->type && STR_PTR == val->str_type) {
	free(val->str.str);
    }
    if (OJC_ARRAY == val->type || OJC_OBJECT == val->type) {
	ojcVal	m;
	ojcVal	next;

	for (m = val->members.head; 0 != m; m = next) {
	    next = m->next;
	    _ojc_val_destroy(m, freed);
	}
    }
    if (0 == freed->head) {
	freed->head = val;
    } else {
	freed->tail->next = val;
    }
    val->next = 0;
    freed->tail = val;
}

void
_ojc_destroy(ojcVal val) {
    struct _List	freed = { 0, 0 };

    _ojc_val_destroy(val, &freed);

    pthread_mutex_lock(&free_val_mutex);
    if (0 == free_vals.head) {
	free_vals.head = freed.head;
    } else {
	free_vals.tail->next = freed.head;
    }
    free_vals.tail = freed.tail;
    pthread_mutex_unlock(&free_val_mutex);
}

void
_ojc_val_return(List freed) {
    if (0 == freed->head) {
	return;
    }
    pthread_mutex_lock(&free_val_mutex);
    if (0 == free_vals.head) {
	free_vals.head = freed->head;
    } else {
	free_vals.tail->next = freed->head;
    }
    free_vals.tail = freed->tail;
    pthread_mutex_unlock(&free_val_mutex);
}
