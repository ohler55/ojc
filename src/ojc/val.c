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
#include <string.h>

#include "val.h"

static struct _List	free_vals = LIST_INIT;
static struct _MList	free_bstrs = LIST_INIT;

ojcVal
_ojc_val_create(ojcValType type) {
    ojcVal	val = NULL;

    // Carelessly check to see if a new val is needed. It doesn't matter if we
    // get it wrong here.
    if (NULL == free_vals.head || free_vals.head == free_vals.tail) {
	val = (ojcVal)malloc(sizeof(struct _ojcVal));
    } else {
	// Looks like we need to lock it down for a moment using the atomic busy
	// flag.
	while (atomic_flag_test_and_set(&free_vals.busy)) {
	}
	if (NULL == free_vals.head || free_vals.head == free_vals.tail) {
	    val = (ojcVal)malloc(sizeof(struct _ojcVal));
	} else {
	    val = free_vals.head;
	    free_vals.head = free_vals.head->next;
	}
	atomic_flag_clear(&free_vals.busy);
    }
    val->next = NULL;
    val->key_type = STR_NONE;
    val->str_type = STR_NONE;
    val->members.head = NULL;
    val->members.tail = NULL;
    val->type = type;
    val->expect = NEXT_NONE;

    return val;
}

void
_ojc_val_create_batch(size_t cnt, List vals) {
    ojcVal	v;

    if (free_vals.head != free_vals.tail) {
	while (atomic_flag_test_and_set(&free_vals.busy)) {
	}
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
	atomic_flag_clear(&free_vals.busy);
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

static void
free_key(ojcVal val, MList freed_bstrs) {
    switch (val->key_type) {
    case STR_PTR:
	free(val->key.str);
	break;
    case STR_BLOCK:
	if (0 == freed_bstrs->head) {
	    freed_bstrs->head = val->key.bstr;
	} else {
	    freed_bstrs->tail->next = val->key.bstr;
	}
	val->key.bstr->next = 0;
	freed_bstrs->tail = val->key.bstr;
	break;
    default:
	break;
    }
    val->key_type = STR_NONE;
}

void
_ojc_set_key(ojcVal val, const char *key, int klen) {
    struct _MList	freed_bstrs = { 0, 0 };

    free_key(val, &freed_bstrs);
    while (atomic_flag_test_and_set(&free_bstrs.busy)) {
    }
    if (0 != freed_bstrs.head) {
	if (0 == free_bstrs.head) {
	    free_bstrs.head = freed_bstrs.head;
	} else {
	    free_bstrs.tail->next = freed_bstrs.head;
	}
	free_bstrs.tail = freed_bstrs.tail;
    }
    atomic_flag_clear(&free_bstrs.busy);

    if (0 != key) {
	if (0 >= klen) {
	    klen = strlen(key);
	}
	if ((int)sizeof(union _Bstr) <= klen) {
	    val->key_type = STR_PTR;
	    val->key.str = strndup(key, klen);
	    val->key.str[klen] = '\0';
	} else if ((int)sizeof(val->key.ca) <= klen) {
	    val->key_type = STR_BLOCK;
	    val->key.bstr = _ojc_bstr_create();
	    memcpy(val->key.bstr->ca, key, klen);
	    val->key.bstr->ca[klen] = '\0';
	} else {
	    val->key_type = STR_ARRAY;
	    memcpy(val->key.ca, key, klen);
	    val->key.ca[klen] = '\0';
	}
    }
}

int
_ojc_val_destroy(ojcVal val, List freed, MList freed_bstrs) {
    if (OJC_FREE == val->type) {
	// TBD
	printf("*** already freed\n");
	return OJC_MEMORY_ERR;
    }
    free_key(val, freed_bstrs);
    if (OJC_STRING == val->type || OJC_NUMBER == val->type) {
	switch (val->str_type) {
	case STR_PTR:
	    free(val->str.str);
	    break;
	case STR_BLOCK:
	    if (0 == freed_bstrs->head) {
		freed_bstrs->head = val->str.bstr;
	    } else {
		freed_bstrs->tail->next = val->str.bstr;
	    }
	    val->str.bstr->next = 0;
	    freed_bstrs->tail = val->str.bstr;
	    break;
	default:
	    break;
	}
    }
    if (OJC_ARRAY == val->type || OJC_OBJECT == val->type) {
	ojcVal	m;
	ojcVal	next;
	int	err;

	for (m = val->members.head; 0 != m; m = next) {
	    next = m->next;
	    if (0 != (err = _ojc_val_destroy(m, freed, freed_bstrs))) {
		return err;
	    }
	}
    }
    if (0 == freed->head) {
	freed->head = val;
    } else {
	freed->tail->next = val;
    }
    val->next = 0;
    val->type = OJC_FREE;
    val->str_type = STR_NONE;
    val->key_type = STR_NONE;
    freed->tail = val;

    return 0;
}

void
_ojc_val_return(List freed, MList freed_bstrs) {
    if (0 == freed->head) {
	return;
    }
    while (atomic_flag_test_and_set(&free_vals.busy)) {
    }
    if (0 == free_vals.head) {
	free_vals.head = freed->head;
    } else {
	free_vals.tail->next = freed->head;
    }
    free_vals.tail = freed->tail;

    if (0 != freed_bstrs->head) {
	if (0 == free_bstrs.head) {
	    free_bstrs.head = freed_bstrs->head;
	} else {
	    free_bstrs.tail->next = freed_bstrs->head;
	}
	free_bstrs.tail = freed_bstrs->tail;
    }
    atomic_flag_clear(&free_vals.busy);
}

int
_ojc_destroy(ojcVal val) {
    struct _List	freed = { NULL, NULL };
    struct _MList	freed_bstrs = { NULL, NULL };
    int			err =_ojc_val_destroy(val, &freed, &freed_bstrs);

    if (0 == err) {
	_ojc_val_return(&freed, &freed_bstrs);
    }
    return err;
}

Bstr
_ojc_bstr_create() {
    Bstr	bstr;

    if (free_bstrs.head == free_bstrs.tail) {
	bstr = (Bstr)malloc(sizeof(union _Bstr));
    } else {
	while (atomic_flag_test_and_set(&free_bstrs.busy)) {
	}
	if (free_bstrs.head == free_bstrs.tail) {
	    bstr = (Bstr)malloc(sizeof(union _Bstr));
	} else {
	    bstr = free_bstrs.head;
	    free_bstrs.head = free_bstrs.head->next;
	}
	atomic_flag_clear(&free_bstrs.busy);
    }
    *bstr->ca = '\0';

    return bstr;
}

void
_ojc_bstr_create_batch(size_t cnt, MList list) {
    Bstr	v;

    if (free_bstrs.head != free_bstrs.tail) {
	while (atomic_flag_test_and_set(&free_bstrs.busy)) {
	}
	if (free_bstrs.head != free_bstrs.tail) {
	    Bstr	prev = free_bstrs.head;

	    list->head = prev;
	    for (v = prev; 0 < cnt && v != free_bstrs.tail; cnt--, v = v->next) {
		prev = v;
	    }
	    free_bstrs.head = v;
	    list->tail = prev;
	    list->tail->next = 0;
	}
	atomic_flag_clear(&free_bstrs.busy);
    }
    for (; 0 < cnt; cnt--) {
	v = _ojc_bstr_create();
	v->next = 0;
	if (0 == list->head) {
	    list->head = v;
	} else {
	    list->tail->next = v;
	}
	list->tail = v;
    }
}

void
_ojc_bstr_return(MList freed) {
    if (0 == freed->head) {
	return;
    }
    while (atomic_flag_test_and_set(&free_bstrs.busy)) {
    }
    if (0 == free_bstrs.head) {
	free_bstrs.head = freed->head;
    } else {
	free_bstrs.tail->next = freed->head;
    }
    free_bstrs.tail = freed->tail;
    atomic_flag_clear(&free_bstrs.busy);
}

void
_ojc_val_cleanup() {
    ojcVal	v;
    ojcVal	head;
    Bstr	bhead;
    Bstr	b;

    while (atomic_flag_test_and_set(&free_vals.busy)) {
    }
    head = free_vals.head;
    free_vals.head = NULL;
    free_vals.tail = NULL;
    atomic_flag_clear(&free_vals.busy);
    while (atomic_flag_test_and_set(&free_bstrs.busy)) {
    }
    bhead = free_bstrs.head;
    free_bstrs.head = NULL;
    free_bstrs.tail = NULL;
    atomic_flag_clear(&free_bstrs.busy);

    while (0 != head) {
	v = head;
	head = v->next;
	free(v);
    }

    while (0 != bhead) {
	b = bhead;
	bhead = b->next;
	free(b);
    }
}

