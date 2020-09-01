// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "oj/oj.h"
#include "oj/buf.h"
#include "helper.h"

#define MAX_APPS	8
#define MAX_BAR		96

// Keep the maximum under 114 so it displays in a single github page without
// horizontal scrolling. Leaving 6 characters for the usec/iteration and 12
// for the name leaves us with 96. Each char is 3 byte and then on more for
// the \0 termination for 289.
static const char	bar[289] = "████████████████████████████████████████████████████████████████████████████████████████████████";
static const char	*bar_frac[] = { "", "▏", "▎", "▍", "▌", "▋", "▊", "▉" };

typedef struct _benchResult {
    const char		*name;
    const char		*mode;
    const char		*size;
    int64_t		cnt;
    int64_t		usec;
    const char		*err;
    const char		*mem;
} *BenchResult;

typedef struct _testResult {
    const char		*name;
    const char		*err;
    bool		pass;
} *TestResult;

typedef struct _app {
    const char	*path;

} *App;

typedef struct _result {
    const char		*mode;
    const char		*filename;
    const char		*size; // also use for test title
    long		iter;
    int			app_cnt;
    bool		expect_err;
    struct _benchResult	bench_results[MAX_APPS];
    struct _testResult	test_results[MAX_APPS];
} *Result;

extern int	genlog(FILE *f, long size);

static struct _result	results[] = {
    { .mode = "validate", .size = "small", .filename = "files/ca.json", .iter = 30000 },
    { .mode = "parse", .size = "small", .filename = "files/ca.json", .iter = 30000 },
    { .mode = "multiple-light", .size = "small", .filename = "files/1G.json", .iter = 1 },
    { .mode = "multiple-heavy", .size = "small", .filename = "files/1G.json", .iter = 1 },
    { .mode = "multiple-light", .size = "medium", .filename = "files/5G.json", .iter = 1 },
    { .mode = "multiple-heavy", .size = "medium", .filename = "files/5G.json", .iter = 1 },
    { .mode = "multiple-light", .size = "large", .filename = "files/10G.json", .iter = 1 },
    { .mode = "multiple-heavy", .size = "large", .filename = "files/10G.json", .iter = 1 },
    { .mode = "multiple-light", .size = "huge", .filename = "files/20G.json", .iter = 1 },
    { .mode = "multiple-heavy", .size = "huge", .filename = "files/20G.json", .iter = 1 },
    { .mode = "test", .size = "Large exponent (309)", .filename = "files/num-big-exp.json", .expect_err = false},
    { .mode = "test", .size = "Larger exponent (4000)", .filename = "files/num-bigger-exp.json", .expect_err = false},
    { .mode = "test", .size = "Large integer (20+ digits)", .filename = "files/num-long-int.json", .expect_err = false},
    { .mode = "test", .size = "Detect invalid (192.168.10.100)", .filename = "files/num-ip-addr.json", .expect_err = true },
    { .mode = "test", .size = "Detect invalid (1e2e3)", .filename = "files/num-eee.json", .expect_err = true },
    { .mode = "test", .size = "Detect invalid (-0.0-)", .filename = "files/num-kilroy.json", .expect_err = true},
    { .mode = "test", .size = "Detect invalid (uuid)", .filename = "files/num-uuid.json", .expect_err = true},
    { .mode = NULL },
};

static const char	*mode = NULL;
static const char	*size = NULL;
static const char	*modes[] = { "validate", "parse", "multiple-light", "multiple-heavy", "encode", "test", NULL };
static const char	*sizes[] = { "small", "large", "huge", NULL };
static int		mult = 1;
static bool		verbose = false;

static void
usage(const char *appName) {
    struct _ojBuf	buf;

    oj_buf_init(&buf, 0);
    for (const char **m = modes; NULL != *m; m++) {
	oj_buf_append_string(&buf, *m, strlen(*m));
	oj_buf_append(&buf, '|');
    }
    printf("%s [-c <comparison>] app-path...\n", appName);
    printf("\n");
    printf("  Compare JSON tools with some simple validation checks and benchmarks.\n");
    printf("\n");
    printf("  -o operation  (one of the operations), default all\n");
    printf("                test - run validation tests to assure compliance\n");
    printf("                validate - validate the file only\n");
    printf("                parse - parse and assure all elements have been parsed\n");
    printf("                multiple-light - parse and validate\n");
    printf("                multiple-heavy - parse and spend some time processing\n");
    printf("                encode - create a small set of objects in memory and encode\n");
    printf("\n");
    printf("  -s size       (one of the sizes), default all\n");
    printf("                small - multiple JSON entries about 1GB in size\n");
    printf("                large - multiple JSON entries about 10GB in size\n");
    printf("                huge - multiple JSON entries with size larger than machine memory\n");
    printf("\n");
    printf("  -m multiplier iteration multiplier\n");
    printf("\n");
    printf("  -v            verbose output\n");
    printf("\n");
    oj_buf_cleanup(&buf);
    exit(0);
}

static ojVal
run_app(const char *app, const char *m, const char *filename, long iter) {
    char		cmd[1024];
    char		out[4096];
    struct _ojErr	err = OJ_ERR_INIT;
    FILE		*f;

    snprintf(cmd, sizeof(cmd), "%s %s \"%s\" %ld\n", app, m, filename, iter);
    if (NULL == (f = popen(cmd, "r"))) {
	printf("*-*-* failed to run %s\n", cmd);
	return NULL;
    }
    size_t	len = fread(out, 1, sizeof(out), f);

    if (len < 0) {
	printf("*-*-* failed to read output from '%s'\n", cmd);
	return NULL;
    }
    out[len] = '\0';
    if (0 != pclose(f)) {
	printf("*-*-* exited with error '%s'\n", cmd);
	return NULL;
    }
    ojVal	val = oj_parse_str(&err, out, NULL);

    if (OJ_OK != err.code) {
	printf("*-*-* failed to parse result from '%s'. %s\n", cmd, err.msg);
	return NULL;
    }
    return val;
}

static void
run(const char **apps) {
    for (Result r = results; NULL != r->mode; r++) {
	r->app_cnt = 0;
	memset(r->bench_results, 0, sizeof(r->bench_results));
	memset(r->test_results, 0, sizeof(r->test_results));
	if (NULL != mode && 0 != strcmp(mode, r->mode)) {
	    continue;
	}
	if (0 == strcmp("test", r->mode)) {
	    if (verbose) {
		printf("\ntest %s with %s\n", r->size, r->filename);
	    }
	    for (const char **a = apps; NULL != *a; a++) {
		TestResult	tr = r->test_results + (a - apps);
		ojVal		val = run_app(*a, r->mode, r->filename, r->iter);

		r->app_cnt++;
		if (NULL != val) {
		    if (NULL != (tr->name = oj_str_get(oj_object_get(val, "name", 4)))) {
			tr->name = strdup(tr->name);
		    }
		    if (NULL != (tr->err = oj_str_get(oj_object_get(val, "err", 3)))) {
			tr->err = strdup(tr->err);
		    }
		    tr->pass = (r->expect_err ? NULL != tr->err : NULL == tr->err);
		    oj_destroy(val);
		    if (verbose) {
			printf("%10s: %s\n", tr->name, tr->pass ? "pass" : "fail");
		    }
		}
	    }
	    continue;
	}
	if (NULL != size && 0 != strcmp(size, r->size)) {
	    continue;
	}
	long	iter = r->iter;

	if (1 < iter && 1 < mult) {
	    iter *= mult;
	}
	if (verbose) {
	    printf("\n%s %s (%s) %ld times\n", r->mode, r->filename, r->size, iter);
	}
	for (const char **a = apps; NULL != *a; a++) {
	    if (0 == strncmp("multiple", r->mode, 8)) {
		// Load some large file not used for benchmarking to flush the
		// OS file caching and level the field no matter what order
		// runs are made.
		char	*buf = load_file("files/5G.json");

		free(buf);
	    }
	    BenchResult	br = r->bench_results + (a - apps);
	    ojVal	val = run_app(*a, r->mode, r->filename, r->iter);

	    r->app_cnt++;
	    br->mode = mode;
	    br->size = size;
	    if (NULL != val) {
		if (NULL != (br->name = oj_str_get(oj_object_get(val, "name", 4)))) {
		    br->name = strdup(br->name);
		}
		if (NULL != (br->err = oj_str_get(oj_object_get(val, "err", 3)))) {
		    br->err = strdup(br->err);
		}
		br->usec = oj_int_get(oj_object_get(val, "usec", 4));
		br->cnt = oj_int_get(oj_object_get(val, "iter", 4));
		if (NULL != (br->mem = oj_str_get(oj_object_get(val, "mem", 3)))) {
		    br->mem = strdup(br->mem);
		}
		oj_destroy(val);
		if (verbose) {
		    if (NULL != br->err) {
			printf("%10s: %s\n", br->name, br->err);
		    } else {
			double	per = (double)br->usec / (double)br->cnt;

			printf("%10s: %10lld entries in %8.3f msecs. (%7.1f usec/iterations) used %s of memory\n",
			       br->name, (long long)br->cnt, (double)br->usec / 1000.0, per, br->mem);
		    }
		}
	    }
	}
    }
    if (verbose) {
	printf("\n");
    }
}

static bool
includes(const char *s, const char **choices) {
    for (; NULL != *choices; choices++) {
	if (0 == strcmp(s, *choices)) {
	    return true;
	}
    }
    return false;
}

static int
check_write(const char *filename, size_t size) {
    FILE	*f = fopen(filename, "r");

    if (NULL == f) {
	if (NULL == (f = fopen(filename, "w"))) {
	    return errno;
	}
	genlog(f, size);
    }
    fclose(f);

    return 0;
}

static void
draw_bars(Result results, const char *prefix) {
    size_t	plen = strlen(prefix);
    double	max = 0.0;
    double	per;

    for (Result r = results; NULL != r->mode; r++) {
	if (0 != strncmp(prefix, r->mode, plen) || 0 == r->app_cnt) {
	    continue;
	}
	for (BenchResult br = r->bench_results; br - r->bench_results < r->app_cnt; br++) {
	    if (max < (per = (double)br->usec / (double)br->cnt)) {
		max = per;
	    }
	}
    }
    if (max <= 0.0) {
	return;
    }
    double	scale = (double)MAX_BAR  / max;
    double	dlen;
    size_t	blen;
    int		frac;

    for (Result r = results; NULL != r->mode; r++) {
	if (0 != strncmp(prefix, r->mode, plen) || 0 == r->app_cnt) {
	    continue;
	}
	printf("\n%s %s (%s) %ld times\n", r->mode, r->filename, r->size, r->iter);
	for (BenchResult br = r->bench_results; br - r->bench_results < r->app_cnt; br++) {
	    per = (double)br->usec / (double)br->cnt;
	    dlen = per * scale;
	    blen = (size_t)dlen;
	    frac = (int)((dlen - (double)(int)dlen) * 8.0);
	    printf("%10s ", br->name);
	    fwrite(bar, 3, blen, stdout);
	    printf("%s%5.1f: %s\n", bar_frac[frac], per, br->mem);
	}
    }
}

static void
summary() {
    bool	has = false;

    for (Result r = results; NULL != r->mode; r++) {
	if (0 == r->app_cnt || 0 != strcmp("test", r->mode)) {
	    continue;
	}
	if (!has) {
	    printf("\n");
	    printf("| Test                             |");
	    for (TestResult tr = r->test_results; tr - r->test_results < r->app_cnt; tr++) {
		printf(" %-10s |", tr->name);
	    }
	    printf("\n");
	    printf("| -------------------------------- |");
	    for (TestResult tr = r->test_results; tr - r->test_results < r->app_cnt; tr++) {
		printf(" ---------- |");
	    }
	    printf("\n");
	    has = true;
	}
	printf("| %-32s |", r->size);
	for (TestResult tr = r->test_results; tr - r->test_results < r->app_cnt; tr++) {
	    printf("     %s     |", tr->pass ? "✅" : "❌");
	}
	printf("\n");
    }
    draw_bars(results, "validate");
    draw_bars(results, "parse");
    draw_bars(results, "multiple");

    printf("\n");
}

int
main(int argc, char **argv) {
    char	*appName = *argv;
    char	*a;
    const char	*apps[MAX_APPS];
    int		app_cnt = 0;

    check_write("files/100K.json", 100LL * 1024LL);
    check_write("files/1G.json", 1024LL * 1024LL * 1024LL);
    check_write("files/5G.json", 1024LL * 1024LL * 1024LL * 5LL);
    check_write("files/10G.json", 1024LL * 1024LL * 1024LL * 10LL);
    check_write("files/20G.json", 1024LL * 1024LL * 1024LL * 20LL);

    memset(apps, 0, sizeof(apps));
    argc--;
    argv++;
    for (; 0 < argc; argc--, argv++) {
	a = *argv;
	if ('-' == *a) {
	    switch (a[1]) {
	    case 'v':
		verbose = true;
		break;
	    case 'o':
		argv++;
		argc--;
		mode = *argv;
		if (!includes(mode, modes)) {
		    printf("*-*-* %s is not a valid mode\n", mode);
		    usage(appName);
		    return 1;
		}
		break;
	    case 's':
		argv++;
		argc--;
		size = *argv;
		if (!includes(size, sizes)) {
		    printf("*-*-* %s is not a valid size\n", size);
		    usage(appName);
		    return 1;
		}
		break;
	    case 'm':
		argv++;
		argc--;
		mult = atoi(*argv);
		break;
	    default:
		usage(appName);
		break;
	    }
	} else {
	    if (MAX_APPS == app_cnt) {
		printf("*-*-* too many applications to evaluate\n");
		return 1;
	    }
	    apps[app_cnt] = a;
	    app_cnt++;
	}
    }
    run(apps);

    summary();

    return 0;
}
