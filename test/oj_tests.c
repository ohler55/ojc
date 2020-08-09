// Copyright (c) 2020, Peter Ohler, All rights reserved.

#include <string.h>

#include "ut.h"

extern void	append_validate_tests(Test tests);
extern void	append_pushpop_tests(Test tests);
extern void	append_parse_tests(Test tests);
extern void	append_chunk_tests(Test tests);

int
main(int argc, char **argv) {
    struct _Test	tests[1024] = { { NULL, NULL } };

    append_validate_tests(tests);
    append_pushpop_tests(tests);
    append_parse_tests(tests);
    append_chunk_tests(tests);

    ut_init(argc, argv, "oj", tests);

    ut_done();
    oj_cleanup();

    return 0;
}
