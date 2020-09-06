// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>

#include "oj/oj.h"

// Parse a JSON string and return an JSON element as an ojVal. ojVal is the
// type used to represent JSON element in the Oj library,

int
main(int argc, char **argv) {
    const char		*str =
	"{\n"
	"  \"num\": 12.34e567\n"
	"}";
    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		val;

    printf("Parsing %s\n", str);
    val = oj_parse_str(&err, str, NULL);
    if (NULL != val) { // could check err.code instead for OJ_OK
	printf("Success!\n");
	// Proper memory management releases memoory when no longer
	// needed. This is one way to cleanup.
	oj_destroy(val);
    } else {
	printf("Parse error: %s at %d:%d\n", err.msg, err.line, err.col);
    }

    // Another way to return memory is to use the ojReuser. This is slightly
    // more efficient. Parsing the same string with ojReuser looks like this:
    struct _ojReuser	reuser;

    oj_err_init(&err);
    printf("Parsing %s\n", str);
    val = oj_parse_str(&err, str, &reuser);
    if (OJ_OK == err.code) {
	printf("Success!\n");
    } else {
	printf("Parse error: %s at %d:%d\n", err.msg, err.line, err.col);
    }
    oj_reuse(&reuser);
    oj_cleanup();

    return err.code;
}
