/*******************************************************************************
 * Copyright 2009 by Peter Ohler
 * ALL RIGHTS RESERVED
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "ut.h"

static void	usage(const char *appName);
static Test	findTest(const char *name);

FILE		*ut_out = 0;
int		ut_verbose = 0;

static const char	*group = 0;
static Test		tests = 0;
static Test		currentTest = 0;

void
ut_print(const char *format, ...) {
    va_list	ap;

    va_start(ap, format);
    vfprintf(ut_out, format, ap);
    va_end(ap);
}

void
ut_init(int argc, char **argv, const char *groupName, Test allTests) {
    Test	t;
    char	*appName = *argv;
    char	*a;
    int		runAll = 1;

    ut_out = stdout;
    tests = allTests;
    group = groupName;
    for (t = tests; t->name != 0; t++) {
	t->pass = -1;
	t->run = 0;
    }
    argc--;
    argv++;
    for (; 0 < argc; argc--, argv++) {
	a = *argv;
	if (0 == strcmp("-o", a)) {
	    argc--;
	    argv++;
	    if (0 == (ut_out = fopen(*argv, "a"))) {
		printf("Failed to open %s\n", *argv);
		usage(appName);
	    }
	} else if (0 == strcmp("-c", a)) {
	    argc--;
	    argv++;
	    if (0 == (ut_out = fopen(*argv, "w"))) {
		printf("Failed to open %s\n", *argv);
		usage(appName);
	    }
	} else if (0 == strcmp("-v", a)) {
	    ut_verbose += 1;
	} else {
	    if (0 == (t = findTest(a))) {
		printf("%s does not contain test %s\n", group, a);
		usage(appName);
	    }
	    t->run = 1;
	    runAll = 0;
	}
    }
    if (runAll) {
	for (t = tests; t->name != 0; t++) {
	    t->run = 1;
	}
    }
    ut_print("%s tests started\n", group);

    for (currentTest = tests; currentTest->name != 0; currentTest++) {
	if (currentTest->run) {
	    if (2 <= ut_verbose) {
		ut_print(">>> %s\n", currentTest->name);
	    }
	    currentTest->func();
	    if (2 <= ut_verbose) {
		ut_print("<<< %s\n", currentTest->name);
	    }
	}
    }
}

void
ut_done(void) {
    char	nameFormat[32];
    Test	t;
    const char	*result = "Skipped";
    time_t	tt;
    int		cnt = 0;
    int		fail = 0;
    int		skip = 0;
    int		len, maxLen = 1;

    for (t = tests; t->name != 0; t++) {
	len = strlen(t->name);
	if (maxLen < len) {
	    maxLen = len;
	}
    }
    sprintf(nameFormat, "  %%%ds: %%s\n", maxLen);
    ut_print("Summary for %s:\n", group);
    for (t = tests; t->name != 0; t++) {
	switch (t->pass) {
	case -1:
	    result = "Skipped";
	    skip++;
	    break;
	case 0:
	    result = "FAILED";
	    fail++;
	    break;
	case 1:
	default:
	    result = "Passed";
	    break;
	}
	cnt++;
	ut_print(nameFormat, t->name, result);
    }
    ut_print("\n");
    if (0 < fail) {
	ut_print("%d out of %d tests failed for %s\n", fail, cnt, group);
	if (0 < skip) {
	    ut_print("%d out of %d skipped\n", skip, cnt);
	}
	ut_print("%d out of %d passed\n", (cnt - fail - skip), cnt);
    } else if (0 < skip) {
	ut_print("%d out of %d skipped\n", skip, cnt);
	ut_print("%d out of %d passed\n", (cnt - skip), cnt);
    } else {
	ut_print("All %d tests passed!\n", cnt);
    }
    tt = time(0);
    ut_print("%s tests completed %s\n", group, ctime(&tt));
    if (stdout != ut_out) {
	fclose(ut_out);
    }
    // exit((cnt << 16) | fail);
    exit(0);
}

int
ut_same(const char *expected, const char *actual) {
    const char	*e = expected;
    const char	*a = actual;
    int		line = 1;
    int		col = 1;
    int		pass = 1;

    if (expected == actual) {
	if (0 != currentTest->pass) {	// don't replace failed status
	    currentTest->pass = 1;
	}
	return 1;
    }
    if (NULL == actual || NULL == expected) {
	currentTest->pass = 0;
	return 0;
    }
    for (; '\0' != *e; e++, a++) {
	if (*e == *a || '?' == *e) {
	    if ('\n' == *a) {
		line++;
		col = 0;
	    }
	    col++;
	} else if ('$' == *e) {	// a digit
	    if ('0' <= *a && *a <= '9') {
		while ('0' <= *a && *a <= '9') {
		    a++;
		    col++;
		}
		a--;
	    } else {
		pass = 0;
		break;
	    }
	} else if ('#' == *e) {	// hexidecimal
	    if (('0' <= *a && *a <= '9') ||
		('a' <= *a && *a <= 'f') ||
		('A' <= *a && *a <= 'F')) {
		while (('0' <= *a && *a <= '9') ||
		       ('a' <= *a && *a <= 'f') ||
		       ('A' <= *a && *a <= 'F')) {
		    a++;
		    col++;
		}
		a--;
	    } else {
		pass = 0;
		break;
	    }
	} else if ('*' == *e) {
	    const char	ne = *(e + 1);

	    while (ne != *a && '\0' != *a) {
		a++;
		col++;
	    }
	    a--;
	} else {
	    pass = 0;
	    break;
	}
    }
    if ('\0' != *a) {
	if ('\0' == *e) {
	    ut_print("%s.%s Failed: Actual result longer than expected\n", group, currentTest->name);
	    pass = 0;
	    if (ut_verbose) {
		ut_print("expected: '%s'\n", expected);
		ut_print("  actual: '%s'\n\n", actual);
	    }
	} else {
	    ut_print("%s.%s Failed: Mismatch at line %d, column %d\n", group, currentTest->name, line, col);
	    if (ut_verbose) {
		ut_print("expected: '%s'\n", expected);
		ut_print("  actual: '%s'\n\n", actual);
	    }
	    pass = 0;
	}
    } else if ('\0' != *e) {
	ut_print("%s.%s Failed: Actual result shorter than expected\n", group, currentTest->name);
	pass = 0;
	if (ut_verbose) {
	    ut_print("expected: '%s'\n", expected);
	    ut_print("  actual: '%s'\n\n", actual);
	}
    }
    if (0 != currentTest->pass) {	// don't replace failed status
	currentTest->pass = pass;
    }
    return pass;
}

int
ut_same_int(int64_t expected, int64_t actual, const char *format, ...) {
    int	pass = 0;

    if (expected == actual) {
	pass = 1;
    } else {
	va_list	ap;
	char	buf[256];

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	ut_print("%s.%s Failed: %s\n  expected: %lld\n  actual: %lld\n",
		 group, currentTest->name, buf, expected, actual);
    }
    if (0 != currentTest->pass) {	// don't replace failed status
	currentTest->pass = pass;
    }
    return pass;
}

int
ut_same_double(long double expected, long double actual, long double prec, const char *format, ...) {
    int	pass = 0;

    if (expected - prec < actual && actual < expected + prec) {
	pass = 1;
    } else {
	va_list	ap;
	char	buf[256];

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	ut_print("%s.%s Failed: %s\n  expected: %Lg\n  actual: %Lg\n",
		 group, currentTest->name, buf, expected, actual);
    }
    if (0 != currentTest->pass) {	// don't replace failed status
	currentTest->pass = pass;
    }
    return pass;
}

void
ut_pass() {
    currentTest->pass = 1;
}

void
ut_fail() {
    ut_print("%s.%s Failed\n", group, currentTest->name);
    currentTest->pass = 0;
}

int
ut_true(bool condition) {
    if (!condition) {
	ut_print("%s.%s Failed: Condition was false\n", group, currentTest->name);
	currentTest->pass = 0;
    } else if (0 != currentTest->pass) {	// don't replace failed status
	currentTest->pass = 1;
    }
    return condition;
}

int
ut_false(bool condition) {
    if (condition) {
	ut_print("%s.%s Failed: Condition was true\n", group, currentTest->name);
	currentTest->pass = 0;
    } else if (0 != currentTest->pass) {
	currentTest->pass = 1;
    }
    return !condition;
}

char*
ut_loadFile(const char *filename) {
    FILE	*f;
    char	*buf;
    long	len;

    if (0 == (f = fopen(filename, "r"))) {
	ut_print("Error: failed to open %s.\n", filename);
	return 0;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    if (0 == (buf = (char*)malloc(len + 1))) {
	ut_print("Error: failed to allocate %ld bytes for file %s.\n", len, filename);
	return 0;
    }
    fseek(f, 0, SEEK_SET);
    if (len != fread(buf, 1, len, f)) {
	ut_print("Error: failed to read %ld bytes from %s.\n", len, filename);
	fclose(f);
	return 0;
    }
    fclose(f);
    buf[len] = '\0';

    return buf;
}

void
ut_reportTest(const char *testName) {
    Test	t = findTest(testName);
    const char	*result;

    if (0 == t) {
	if (0 != testName) {
	    return;
	}
	t = currentTest;
	if (0 == t) {
	    return;
	}
    }
    switch (t->pass) {
    case -1:
	result = "Skipped";
	break;
    case 0:
	result = "FAILED";
	break;
    case 1:
    default:
	result = "Passed";
	break;
    }
    ut_print("%s: %s", t->name, result);
}

void
ut_resetTest(const char *testName) {
    Test	t = findTest(testName);

    if (0 == t) {
	if (0 != testName) {
	    return;
	}
	t = currentTest;
	if (0 == t) {
	    return;
	}
    }
    t->pass = -1;
}

static void
usage(const char *appName) {
    printf("%s [-m] [-o file] [-c file]\n", appName);
    printf("  -o file  name of output file to append to\n");
    printf("  -c file  name of output file to create and write to\n");
    exit(0);
}

static Test
findTest(const char *name) {
    if (0 != name) {
	Test	t;

	for (t = tests; t->name != 0; t++) {
	    if (0 == strcmp(name, t->name)) {
		return t;
	    }
	}
    }
    return 0;
}

void
ut_hexDump(const unsigned char *data, int len) {
    const unsigned char	*b = data;
    const unsigned char	*end = data + len;
    char		buf[20];
    char		*s = buf;
    int			i;

    for (; b < end; b++) {
        printf("%02X ", *b);
	if (' ' <= *b && *b < 127) {
	    *s++ = (char)*b;
	} else {
	    *s++ = '.';
	}
	i = (b - data) % 16;
        if (7 == i) {
            printf(" ");
	    *s++ = ' ';
        } else if (15 == i) {
	    *s = '\0';
            printf("  %s\n", buf);
	    s = buf;
        }
    }
    i = (b - data) % 16;
    if (0 != i) {
	if (i < 8) {
	    printf(" ");
	}
	for (; i < 16; i++) {
	    printf("   ");
	}
	*s = '\0';
	printf("  %s\n", buf);
    }
}

// out must be 4.25 * len
void
ut_hexDumpBuf(const unsigned char *data, int len, char *out) {
    const unsigned char	*b = data;
    const unsigned char	*end = data + len;
    char		buf[20];
    char		*s = buf;
    int			i;

    for (; b < end; b++) {
        out += sprintf(out, "%02X ", *b);
	if (' ' <= *b && *b < 127) {
	    *s++ = (char)*b;
	} else {
	    *s++ = '.';
	}
	i = (b - data) % 16;
        if (7 == i) {
	    *out++ = ' ';
	    *s++ = ' ';
        } else if (15 == i) {
	    *s = '\0';
            out += sprintf(out, "  %s\n", buf);
	    s = buf;
        }
    }
    i = (b - data) % 16;
    if (0 != i) {
	if (i < 8) {
	    *out++ = ' ';
	}
	for (; i < 16; i++) {
	    *out++ = ' ';
	    *out++ = ' ';
	    *out++ = ' ';
	}
	*s = '\0';
	out += sprintf(out, "  %s\n", buf);
    }
    *out = '\0';
}

char*
ut_toCodeStr(const unsigned char *data, int len) {
    const unsigned char	*d;
    const unsigned char	*end = data + len;
    int			clen = 0;
    char		*str;
    char		*s;

    for (d = data; d < end; d++) {
	if (*d < ' ' || 127 <= *d) {
	    clen += 4;
	} else if ('"' == *d || '\\' == *d) {
	    clen += 3;
	} else {
	    clen++;
	}
    }
    if (0 == (str = (char*)malloc(clen + 1))) {
	return 0;
    }
    for (s = str, d = data; d < end; d++) {
	if (*d < ' ' || 127 <= *d) {
	    *s++ = '\\';
	    *s++ = '0' + (*d >> 6);
	    *s++ = '0' + ((*d >> 3) & 0x07);
	    *s++ = '0' + (*d & 0x07);
	} else if ('"' == *d || '\\' == *d) {
	    *s++ = '\\';
	    *s++ = '\\';
	    *s++ = *d;
	} else {
	    *s++ = *d;
	}
    }
    *s = '\0';

    return str;
}

bool
ut_handle_error(ojcErr err) {
    if (OJC_OK != err->code) {
	ut_print("[%d] %s\n", err->code, err->msg);
	ut_fail();
	return true;
    }
    if (0 != currentTest->pass) {	// don't replace failed status
	currentTest->pass = 1;
    }
    return false;
}

bool
ut_handle_oj_error(ojErr err) {
    if (OJ_OK != err->code) {
	ut_print("[%d] %s\n", err->code, err->msg);
	ut_fail();
	return true;
    }
    if (0 != currentTest->pass) {	// don't replace failed status
	currentTest->pass = 1;
    }
    return false;
}

static uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

void
ut_benchmark(const char *label, int64_t iter, void (*func)(int64_t iter, void *ctx), void *ctx) {
    int64_t	start = clock_micro();
    int64_t	dt;

    func(iter, ctx);

    dt = clock_micro() - start;
    ut_pass();

    printf("%s: %lld iterations in %0.3f msecs. (%g iterations/msec)\n",
	   label, (long long)iter, (double)dt / 1000.0, (double)iter * 1000.0 / (double)dt);
}
