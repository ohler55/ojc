// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include "oj/oj.h"

uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

char*
mem_use(char *buf, size_t size) {
    struct rusage	usage;

    *buf = '\0';
    // TBD round to at least 2 places, adjust for linux vs macOS
    if (0 == getrusage(RUSAGE_SELF, &usage)) {
	long	mem = usage.ru_maxrss;
#ifndef __unix__
	mem /= 1024; // results are in KB, macOS in bytes
#endif
	if (1024 * 1024 * 10 < mem) {
	    snprintf(buf, size, "%ldGB", (mem + (1024 * 1024) / 2) / (1024 * 1024));
	} else if (1024 * 1024 < mem) {
	    snprintf(buf, size, "%0.1fGB", ((double)mem + (1024.0 * 1024.0) / 20.0) / (1024.0 * 1024.0));
	} else if (1024 * 10 < mem) {
	    snprintf(buf, size, "%ldMB", (mem + 512) / 1024);
	} else if (1024 < mem) {
	    snprintf(buf, size, "%0.1fMB", ((double)mem + 51.2) / 1024.0);
	} else {
	    snprintf(buf, size, "%ldKB", mem);
	}
    }
    return buf;
}

char*
load_file(const char *filename) {
    FILE	*f = fopen(filename, "r");
    long	len;
    char	*buf;

    if (NULL == f) {
	printf("*-*-* failed to open file %s\n", filename);
	exit(1);
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (NULL == (buf = malloc(len + 1))) {
	printf("*-*-* not enough memory to load file %s\n", filename);
	exit(1);
    }
    if (len != fread(buf, 1, len, f)) {
	printf("*-*-* reading file %s failed\n", filename);
	exit(1);
    }
    buf[len] = '\0';
    fclose(f);

    return buf;
}

void
form_json_results(const char *name, long long iter, long long usec, const char *mem, const char *err) {
    if (NULL != err) {
	printf("{\"name\":\"oj\",\"err\":\"%s\"}\n", err);
    } else {
	printf("{\"name\":\"%s\",\"usec\":%lld,\"iter\":%lld,\"mem\":\"%s\"}\n",
	       name, usec, iter, mem);
    }
}
