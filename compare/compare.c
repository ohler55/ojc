// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "oj/oj.h"
#include "oj/buf.h"

typedef struct _benchResult {
    struct _benchResult	*next;
    const char		*name;
    int64_t		cnt;
    int64_t		usec;
    const char		*err;
    const char		*mem;
} *BenchResult;

typedef struct _app {
    const char	*path;

} *App;

typedef struct _result {
    const char	*mode;
    const char	*filename;
    const char	*size;
    long	iter;
    BenchResult	results; // one for each app
} *Result;

static struct _result	results[] = {
    { .mode = "parse", .size = "small", .filename = "files/ca.json", .iter = 10000, .results = NULL },
    { .mode = NULL },
};

static const char	*mode = NULL;
static const char	*size = NULL;
static const char	*modes[] = { "validate", "extract", "parse", "encode", "test", NULL };
static const char	*sizes[] = { "small", "medium", "5GB", "large", NULL };

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
    printf("  -o operation  (one of the operations), default all.\n");
    printf("                test - run validation tests to assure compliance\n");
    printf("                validate - validate the file only\n");
    printf("                extract - parse and extract a single value\n");
    printf("                parse - parse and assure all elements have been parsed\n");
    printf("                encode - create a small set of objects in memory and encode\n");
    printf("\n");
    printf("  -s size       (one of the sizes), default all.\n");
    printf("                small - single JSON entry less than 100KB in memory\n");
    printf("                medium - multiple JSON entries just under 4GB in size\n");
    printf("                5GB - multiple JSON entries about 5GB in size\n");
    printf("                large - multiple JSON entries with size larger than machine memory\n");
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
    ojVal	val = oj_parse_str_reuse(&err, out, NULL);

    if (OJ_OK != err.code) {
	printf("*-*-* failed to parse result from '%s'. %s\n", cmd, err.msg);
	return NULL;
    }
    return val;
}

static void
run(const char **apps) {
    for (Result r = results; NULL != r->mode; r++) {
	if (NULL != mode && 0 != strcmp(mode, r->mode)) {
	    continue;
	}
	if (NULL != size && 0 != strcmp(size, r->size)) {
	    continue;
	}
	printf("Parse %s (%s) %ld times\n", r->filename, r->size, r->iter);
	for (const char **a = apps; NULL != *a; a++) {
	    BenchResult	br = (BenchResult)calloc(1, sizeof(struct _benchResult));
	    ojVal	val = run_app(*a, "parse", r->filename, r->iter);

	    if (NULL != val) {
		br->name = oj_str_get(oj_object_get(val, "name", 4));
		br->err = oj_str_get(oj_object_get(val, "err", 3));
		br->usec = oj_int_get(oj_object_get(val, "usec", 4));
		br->cnt = oj_int_get(oj_object_get(val, "iter", 4));
		br->mem = oj_str_get(oj_object_get(val, "mem", 3));

		if (NULL != br->err) {
		    printf("%-10s: %s\n", br->name, br->err);
		} else {
		    double	per = (double)br->usec / (double)br->cnt;

		    printf("%-10s: %10lld entries in %8.3f msecs. (%7.1f usec/iterations) used %s of memory\n",
			   br->name, (long long)br->cnt, (double)br->usec / 1000.0, per, br->mem);
		}
		oj_destroy(val);
	    }
	    // TBD add br to results
	}
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

int
main(int argc, char **argv) {
    char	*appName = *argv;
    char	*a;
    const char	*apps[16];
    int		app_cnt = 0;

    memset(apps, 0, sizeof(apps));
    argc--;
    argv++;
    for (; 0 < argc; argc--, argv++) {
	a = *argv;
	if ('-' == *a) {
	    switch (a[1]) {
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
	    default:
		usage(appName);
		break;
	    }
	} else {
	    apps[app_cnt] = a;
	    app_cnt++;
	}
    }
    run(apps);

    return 0;
}
