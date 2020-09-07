/*******************************************************************************
 * Copyright 2009 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#ifndef __UT_UT_H__
#define __UT_UT_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "oj/oj.h"

typedef struct _Test {
    const char	*name;
    void	(*func)(void);
    int		pass;
    bool	run;
} *Test;

extern bool		ut_init(int argc, char **argv, const char *groupName, Test allTests);
extern Test		ut_append(Test tests, const char *name, void (*func)(void));

extern void		ut_print(const char *format, ...);
extern void		ut_done(void);
extern int		ut_same(const char *expected, const char *actual);
extern int		ut_same_int(int64_t expected, int64_t actual, const char *format, ...);
extern int		ut_same_double(long double expected, long double actual, long double prec, const char *format, ...);
extern int		ut_true(bool condition);
extern int		ut_false(bool condition);
extern void		ut_pass(void);
extern void		ut_fail(void);
extern char*		ut_loadFile(const char *filename);
extern void		ut_reportTest(const char *testName);
extern void		ut_resetTest(const char *testName);
extern void		ut_hexDump(const unsigned char *data, int len);
extern void		ut_hexDumpBuf(const unsigned char *data, int len, char *out);
extern char*		ut_toCodeStr(const unsigned char *data, int len);
extern bool		ut_handle_oj_error(ojErr err);
extern bool		ut_handle_errno();
extern const char*	ut_name();

extern void		ut_benchmark(const char *label, int64_t iter, void (*func)(int64_t iter, void *ctx), void *ctx);

extern FILE		*ut_out;
extern int		ut_verbose;

#endif /* __UT_UT_H__ */
