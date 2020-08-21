#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <ostream>

#include "simdjson.h"

static uint64_t
clock_micro() {
    struct timeval	tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static int
walk_json(simdjson::dom::element doc) {
    int	cnt = 0;

    switch (doc.type()) {
    case simdjson::dom::element_type::ARRAY:
	for (simdjson::dom::element child : simdjson::dom::array(doc)) {
	    cnt += walk_json(child);
	}
	break;
    case simdjson::dom::element_type::OBJECT:
	for (simdjson::dom::key_value_pair field : simdjson::dom::object(doc)) {
	    if (0 == std::string_view(field.key).size()) {
		cnt++;
	    }
	    cnt += walk_json(field.value);
	}
	break;
    case simdjson::dom::element_type::INT64:
	if (0 == int64_t(doc)) {
	    cnt++;
	}
	break;
    case simdjson::dom::element_type::UINT64:
	if (0 == uint64_t(doc)) {
	    cnt++;
	}
	break;
    case simdjson::dom::element_type::DOUBLE:
	if (0.0 == double(doc)) {
	    cnt++;
	}
	break;
    case simdjson::dom::element_type::STRING:
	if (0 == std::string_view(doc).size()) {
	    cnt++;
	}
	break;
    case simdjson::dom::element_type::BOOL:
	if (!bool(doc)) {
	    cnt++;
	}
	break;
    case simdjson::dom::element_type::NULL_VALUE:
	cnt++;
	break;
    }
    return cnt;
}

static int
simd_parse(const char *str, int64_t iter) {
    int64_t			dt;
    int				zero_cnt = 0;
    // It's not clear whether calculating the length outside the timing is the
    // more common use case. It certainly is if given a string from some
    // source that knows the length but not if the caller just has the string
    // itself as would be the case if called from some other package as most
    // parsers would be expected to parse any string, not just those that
    // already have the length calculated.
    long			len = (long)strlen(str);
    int64_t			start = clock_micro();
    simdjson::dom::parser	parser;
    int64_t			ts;
    int64_t			mt;

    for (int i = iter; 0 < i; i--) {
	// It seems simdjson accepts some invalid JSON and does not report
	// errors. Try 1.2e3e4. Neither parse nor converting to a double
	// detect the error.
	try {
	    simdjson::dom::element	doc = parser.parse(str, len, true);
	    walk_json(doc);
	} catch (const std::exception &x) {
	    std::cout << "simdjson_parse failed: " << x.what() << std::endl;
	    return 1;
	}
    }
    dt = clock_micro() - start;
    if (0 != zero_cnt) {
	printf("zero count: %d\n", zero_cnt);
    }
    printf("simdjson_parse   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));

    return 0;
}

static char*
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

static int
simd_parse_file(const char *filename) {
    int64_t				dt;
    int					zero_cnt = 0;
    int64_t				cnt = 0;
    int64_t				start = clock_micro();
    simdjson::dom::parser		parser;
    simdjson::dom::document_stream	docs;
    char				mem[16];

    try {
	docs = parser.load_many(filename);
	for (simdjson::dom::element doc : docs) {
	    cnt++;
	    zero_cnt += walk_json(doc);
	}
    } catch (const std::exception &x) {
	std::cout << "simdjson_parse failed: " << x.what() << std::endl;
	return 1;
    }
    dt = clock_micro() - start;
    if (0 != zero_cnt) {
	printf("zero count: %d\n", zero_cnt);
    }
    printf("simdjson_parse   %lld entries in %8.3f msecs. (%5d iterations/msec) used %s of memory\n",
	   (long long)cnt, (double)dt / 1000.0, (int)((double)cnt * 1000.0 / (double)dt), mem_use(mem, sizeof(mem)));
    return 0;
}

int
main(int argc, char **argv) {
    const char	*filename = NULL;
    int64_t	iter = 1000000LL;
    int		status = 0;
    char	*buf = NULL;
    const char	*str = "{\"level\":\"INFO\",\"message\":\"This is a log message that is long enough to be representative of an actual message.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1,\"where\":[{\"file\":\"my-file.c\",\"line\":123}]}";

    if (1 < argc) {
	filename = argv[1];
	if (2 < argc) {
	    iter = strtoll(argv[2], NULL, 10);
	}
    }
    if (NULL != filename) {
	if (1 < iter) {
	    FILE	*f = fopen(filename, "r");
	    long	len;

	    if (NULL == f) {
		printf("*-*-* failed to open file %s\n", filename);
		exit(1);
	    }
	    fseek(f, 0, SEEK_END);
	    len = ftell(f);
	    fseek(f, 0, SEEK_SET);

	    if (NULL == (buf = (char*)malloc(len + 1))) {
		printf("*-*-* not enough memory to load file %s\n", filename);
		exit(1);
	    }
	    if (len != fread(buf, 1, len, f)) {
		printf("*-*-* reading file %s failed\n", filename);
		exit(1);
	    }
	    buf[len] = '\0';
	    fclose(f);
	    str = buf;
	} else {
	    return simd_parse_file(filename);
	}
    }
    status = simd_parse(str, iter);

    if (NULL != buf) {
	free(buf);
    }
    return status;
}
