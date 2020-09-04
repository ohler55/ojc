// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>
#include <stdlib.h>

#include "oj/oj.h"

// Parse a JSON file and return an JSON element as an ojVal. ojVal is the
// type used to represent JSON element in the Oj library,

int
main(int argc, char **argv) {
    const char		*filename = "sample.json";
    struct _ojErr	err = OJ_ERR_INIT;
    ojVal		val;

    printf("Parsing %s\n", filename);
    val = oj_parse_file(&err, filename, NULL);
    if (NULL != val) { // could check err.code instead for OJ_OK
	char	*str = oj_to_str(val, 0);

	printf("Parsed %s\n", str);
	free(str);
	// Proper memory management releases memoory when no longer
	// needed. This is one way to cleanup.
	oj_destroy(val);
    } else {
	printf("Parse error: %s at %d:%d\n", err.msg, err.line, err.col);
    }

    // Another way to return memory is to use the ojReuser. This is slightly
    // more efficient. Parsing the same file with ojReuser looks like this:
    struct _ojReuser	reuser;

    oj_err_init(&err);
    printf("Parsing %s\n", filename);
    val = oj_parse_file(&err, filename, &reuser);
    if (OJ_OK == err.code) {
	char	*str = oj_to_str(val, 0);

	printf("Parsed %s\n", str);
	free(str);
    } else {
	printf("Parse error: %s at %d:%d\n", err.msg, err.line, err.col);
    }
    oj_reuse(&reuser);

    return err.code;
}
