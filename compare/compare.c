// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "oj/oj.h"
#include "oj/buf.h"

typedef struct _benchResult {
    long	cnt;
    double	msecs;
    long	bytes;
} *BenchResult;

typedef struct _app {
    const char	*path;


} *App;

static const char	*mode = NULL;
static const char	*modes[] = { "parse", "small", "medium", "large", "validate", "write", NULL };

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
    printf("  -c comparison   (%s), default all.\n", buf.head);
    printf("\n");
    oj_buf_cleanup(&buf);
    exit(0);
}

static ojVal
run_app(App app, const char *m, const char *filename, long iter) {
    char	cmd[1024];
    char	out[4096];
    FILE	*f;

    snprintf(cmd, sizeof(cmd), "%s %s \"%s\" %ld\n", app->path, m, filename, iter);
    if (NULL == (f = popen(cmd, "r"))) {
	printf("*-*-* failed to run %s\n", cmd);
	return NULL;
    }
    size_t	len = fread(out, 1, sizeof(out), f);

    if (len < 0) {
	printf("*-*-* failed to read output from %s\n", cmd);
	return NULL;
    }
    out[len] = '\0';
    if (0 != pclose(f)) {
	printf("*-*-* exited with error %s\n", cmd);
	return NULL;
    }

    printf("*** out: '%s'\n", out);
    // TBD parse as JSON
    //
    return NULL;
}

static void
run_parse(App app) {
    // TBD parse list of files, collect and display results
    for (; NULL != app->path; app++) {
	ojVal	val = run_app(app, "parse", "files/ca.json", 1000);
	// extract data needed then display
	oj_destroy(val);
    }
}

int
main(int argc, char **argv) {
    char	*appName = *argv;
    char	*a;
    struct _app	apps[16];
    int		app_cnt = 0;

    memset(apps, 0, sizeof(apps));
    argc--;
    argv++;
    for (; 0 < argc; argc--, argv++) {
	a = *argv;
	if ('-' == *a) {
	    switch (a[1]) {
	    case 'c':
		argv++;
		argc--;
		mode = *argv;
		// TBD check value is one of modes
		break;
	    default:
		usage(appName);
		break;
	    }
	} else {
	    apps[app_cnt].path = a;
	    app_cnt++;
	}
    }
    if (NULL == mode || 0 == strcmp(mode, "parse")) {
	run_parse(apps);
    }

    //
    // TBD results for
    // validate same file for performance
    // parse small file (100M log file)
    // parse medium file (5G log file)
    // parse larger file (20G log file)
    // validate small files with errors or not to check validation


    return 0;
}
