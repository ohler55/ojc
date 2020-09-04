// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>

#include "oj/oj.h"

// This example demonstrates the use of the ojBuilder to construct a JSON
// document as well as writing the document to a file as indented JSON.

int
main(int argc, char **argv) {
    struct _ojBuilder	b = OJ_BUILDER_INIT;

    oj_build_object(&b, NULL);
    oj_build_double(&b, "num", 12.34e567L);
    oj_build_array(&b, "list");
    oj_build_bool(&b, NULL, true);
    oj_build_bool(&b, NULL, false);
    oj_build_pop(&b);
    oj_build_pop(&b);

    if (OJ_OK != b.err.code) {
	printf("Build error: %s at %d:%d\n", b.err.msg, b.err.line, b.err.col);
	return 1;
    }
    char		buf[256];
    struct _ojErr	err = OJ_ERR_INIT;

    oj_fill(&err, b.top, 2, buf, sizeof(buf));

    printf("Built %s\n", buf);

    // Once written, the file should look like the output displayed by printf.
    oj_fwrite(&err, b.top, 2, "out.json");
    if (OJ_OK != err.code) {
	printf("Build error: %s at %d:%d\n", err.msg, err.line, err.col);
	return 1;
    }
    return 0;
}
