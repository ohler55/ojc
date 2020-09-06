// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>

#include "oj/oj.h"

// Parse a JSON string using callbacks on each document. For every document
// the callback is invoked. This is an effective way to iterate over multiple
// documents in a string.

static ojCallbackOp
callback(ojVal val, void *ctx) {
    *(int*)ctx = *(int*)ctx + 1;

    // The return value indicates to the parser what whould be done after the
    // callback. If the document (val) is no longer needed then OJ_DESTROY is
    // returned. If the parsing should be stopped then OJ_STOP is returned. To
    // both stop and cleanup the two values are ORed together (OJ_DESTROY |
    // OJ_STOP).
    return OJ_DESTROY;
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
    int			iter = 0;

    printf("Parsing %s\n", str);
    status = oj_parse_str_cb(&err, str, callback, &iter);
    if (OJ_OK == status) {
	printf("Parsed %d documents\n", iter);
    } else {
	printf("Parse error: %s at %d:%d\n", err.msg, err.line, err.col);
    }
    oj_cleanup();

    return err.code;
}
