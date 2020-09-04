// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>

#include "oj/oj.h"

// Validate a JSON string.

int
main(int argc, char **argv) {
    const char		*str =
	"{\n"
	"  \"num\": 12.34e567\n"
	"}";
    struct _ojErr	err = OJ_ERR_INIT;
    ojStatus		status;

    printf("Validating %s\n", str);
    status = oj_validate_str(&err, str);
    if (OJ_OK == status) {
	printf("Valid!\n");
    } else {
	printf("Validation error: %s at %d:%d\n", err.msg, err.line, err.col);
    }
    return err.code;
}
