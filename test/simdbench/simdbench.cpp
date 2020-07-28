#include <stdio.h>
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
    simdjson::dom::parser	parser;
    int64_t			dt;
    int64_t			start = clock_micro();
    long			len = (long)strlen(str);
    int64_t			ts;
    int64_t			mt;
    int				zero_cnt = 0;

    for (int i = iter; 0 < i; i--) {
	simdjson::dom::element	doc = parser.parse(str, len, true);
	zero_cnt += walk_json(doc);
    }
    dt = clock_micro() - start;
    if (0 != zero_cnt) {
	printf("zero count: %d\n", zero_cnt);
    }
    printf("simdjson_parse   %lld entries in %8.3f msecs. (%5d iterations/msec)\n",
	   (long long)iter, (double)dt / 1000.0, (int)((double)iter * 1000.0 / (double)dt));
}

int
main(int argc, char **argv) {
    int64_t	iter = 1000000LL;
    const char	*str = "{\"level\":\"INFO\",\"message\":\"This is a log message that is long enough to be representative of an actual message.\",\"msgType\":1,\"source\":\"Test\",\"thread\":\"main\",\"timestamp\":1400000000000000000,\"version\":1,\"where\":[{\"file\":\"my-file.c\",\"line\":123}]}";

    simd_parse(str, iter);

    return 0;
}
