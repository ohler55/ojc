// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>

#include "oj/oj.h"

// Parse a JSON string using callbacks on each element. For every element the
// push callback is made. The pop callback is called when an array or object
// close is encountered. This approach is similar to the XML SAX parsers or
// the Ruby Oj SCP parser. It can be more efficient if there is no need to
// build a full JSON structure in memory. It is also useful when parsing a
// string that contains multiple JSON documents.

typedef struct _cnt {
    int	iter;
    int	depth;
} *Cnt;

static void
push(ojVal val, void *ctx) {
    switch (val->type) {
    case OJ_OBJECT:
    case OJ_ARRAY:
	((Cnt)ctx)->depth++;
	break;
    }
    printf("push a %s\n", oj_type_str(val->type));
}

static void
pop(void *ctx) {
    Cnt	c = (Cnt)ctx;

    c->depth--;
    if (c->depth <= 0) {
	c->iter++;
    }
    printf("pop\n");
}

int
main(int argc, char **argv) {
    const char		*str =
	"{\n"
	"  \"num\": 12.34e567\n"
	"}\n"
	"{\n"
	"  \"string\": \"a string\"\n"
	"}";
    struct _ojErr	err = OJ_ERR_INIT;
    ojStatus		status;
    struct _cnt		c = { .depth = 0, .iter = 0 };

    printf("Parsing %s\n", str);
    status = oj_pp_parse_str(&err, str, push, pop, &c);
    if (OJ_OK == status) {
	printf("Parsed %d documents\n", c.iter);
    } else {
	printf("Parse error: %s at %d:%d\n", err.msg, err.line, err.col);
    }
    oj_cleanup();

    return err.code;
}
