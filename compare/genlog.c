// Copyright (c) 2020 by Peter Ohler, ALL RIGHTS RESERVED

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "oj/oj.h"
#include "oj/buf.h"

#ifndef CLOCK_REALTIME_COURSE
#define CLOCK_REALTIME_COURSE	CLOCK_REALTIME
#endif

static const char	log_req_str[] = "{\"level\":\"INFO\",\"message\":\"This is a GraphQL request.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1,\"attachment\":\"{schema{types{name,fields{name,type{name,kind,ofType{name,kind}}}}}\"}";

static int64_t
ntime() {
    struct timespec	ts;

    clock_gettime(CLOCK_REALTIME_COURSE, &ts);

    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

int
genlog(FILE *f, long size) {
    struct _ojErr	err = OJ_ERR_INIT;

    ojVal		res = oj_parse_file(&err, "files/response.json", NULL);
    ojVal		req = oj_parse_str(&err, log_req_str, NULL);
    ojVal		req_ts = oj_object_find(req, "timestamp", 9);
    ojVal		res_ts = oj_object_find(res, "timestamp", 9);
    struct _ojBuf	buf;
    size_t		len;
    int64_t		nsec = 0;
    int64_t		now;

    oj_buf_init(&buf, 0);
    do {
	oj_buf_reset(&buf);
	now = ntime();
	if (nsec < now) {
	    nsec = now;
	} else {
	    nsec++;
	}
	// TBD use set function
	req_ts->num.fixnum = nsec;
	req_ts->num.len = 0;
	res_ts->num.fixnum = nsec;
	res_ts->num.len = 0;

	oj_buf(&buf, req, 0, 0);
	oj_buf_append(&buf, '\n');
	oj_buf(&buf, res, 0, 0);
	oj_buf_append(&buf, '\n');
	len = oj_buf_len(&buf);
	size -= len;

	fwrite(buf.head, 1, len, f);
    } while (0 < size);

    oj_buf_cleanup(&buf);

    return 0;
}
