#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <ostream>

#include "simdjson.h"
#include "../helper.h"

typedef struct _mode {
    const char	*key;
    void	(*func)(const char *filename, long long iter);
} *Mode;

static int
walk(simdjson::dom::element doc) {
    int	cnt = 0;

    switch (doc.type()) {
    case simdjson::dom::element_type::ARRAY:
	for (simdjson::dom::element child : simdjson::dom::array(doc)) {
	    cnt += walk(child);
	}
	break;
    case simdjson::dom::element_type::OBJECT:
	for (simdjson::dom::key_value_pair field : simdjson::dom::object(doc)) {
	    cnt += walk(field.value);
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

static void
validate(const char *filename, long long iter) {
    int64_t			dt;
    // It's not clear whether calculating the length outside the timing is the
    // more common use case. It certainly is if given a string from some
    // source that knows the length but not if the caller just has the string
    // itself as would be the case if called from some other package as most
    // parsers would be expected to parse any string, not just those that
    // already have the length calculated.
    char			*buf = load_file(filename);
    long			len = (long)strlen(buf);
    simdjson::dom::parser	parser;
    int64_t			start = clock_micro();
    const char			*err = NULL;

    for (int i = iter; 0 < i; i--) {
	try {
	    parser.parse(buf, len, true);
	} catch (const std::exception &x) {
	    err = x.what();
	    break;
	}
    }
    dt = clock_micro() - start;
    form_json_results("simdjson", iter, dt, err);
    if (NULL != buf) {
	free(buf);
    }
}

static void
parse(const char *filename, long long iter) {
    int64_t			dt;
    // It's not clear whether calculating the length outside the timing is the
    // more common use case. It certainly is if given a string from some
    // source that knows the length but not if the caller just has the string
    // itself as would be the case if called from some other package as most
    // parsers would be expected to parse any string, not just those that
    // already have the length calculated.
    char			*buf = load_file(filename);
    long			len = (long)strlen(buf);
    simdjson::dom::parser	parser;
    int64_t			start = clock_micro();
    const char			*err = NULL;

    for (int i = iter; 0 < i; i--) {
	try {
	    simdjson::dom::element	doc = parser.parse(buf, len, true);
	    if (walk(doc) < 0) {
		std::cout << "--- should never happen\n" << std::endl;
	    }
	} catch (const std::exception &x) {
	    err = x.what();
	    break;
	}
    }
    dt = clock_micro() - start;
    form_json_results("simdjson", iter, dt, err);
    if (NULL != buf) {
	free(buf);
    }
}

static void
parse_one(const char *filename, long long iter) {
    int64_t				dt;
    simdjson::dom::parser		parser;
    simdjson::dom::document_stream	docs;
    int64_t				start = clock_micro();
    const char				*err = NULL;

    iter = 0;
    try {
	docs = parser.load_many(filename);
	for (simdjson::dom::element doc : docs) {
	    iter++;
	    if (simdjson::dom::element_type::OBJECT == doc.type()) {
		auto ts = doc["timestamp"];
		if (simdjson::dom::element_type::INT64 == ts.type() && int64_t(ts) < 1000000LL) {
		    std::cout << "--- timestamp out of bounds ---\n" << std::endl;
		}
	    }
	}
    } catch (const std::exception &x) {
	err = x.what();
    }
    dt = clock_micro() - start;
    form_json_results("simdjson", iter, dt, err);
}

static void
parse_each(const char *filename, long long iter) {
    int64_t				dt;
    simdjson::dom::parser		parser;
    simdjson::dom::document_stream	docs;
    int64_t				start = clock_micro();
    const char				*err = NULL;

    iter = 0;
    try {
	docs = parser.load_many(filename);
	for (simdjson::dom::element doc : docs) {
	    iter++;
	    if (walk(doc) < 0) {
		std::cout << "--- should never happen\n" << std::endl;
	    }
	}
    } catch (const std::exception &x) {
	err = x.what();
    }
    dt = clock_micro() - start;
    form_json_results("simdjson", iter, dt, err);
}

static void
parse_heavy(const char *filename, long long iter) {
    int64_t				dt;
    simdjson::dom::parser		parser;
    simdjson::dom::document_stream	docs;
    int64_t				start = clock_micro();

    iter = 0;
    try {
	docs = parser.load_many(filename);
	for (simdjson::dom::element doc : docs) {
	    int		delay = walk(doc) / 100 + 5;
	    int64_t	done = clock_micro() + delay;

	    while (clock_micro() < done) {
		continue;
	    }
	    iter++;
	}
	dt = clock_micro() - start;
	form_json_results("simdjson", iter, dt, NULL);
    } catch (const std::exception &x) {
	dt = clock_micro() - start;
	form_json_results("simdjson", iter, dt, x.what());
    }
}

static void
test(const char *filename, long long iter) {
    char			*buf = load_file(filename);
    long			len = (long)strlen(buf);
    simdjson::dom::parser	parser;

    try {
	parser.parse(buf, len, true);
	form_json_results("simdjson", 1, 0, NULL);
    } catch (const std::exception &x) {
	form_json_results("simdjson", 1, 0, x.what());
    }
    if (NULL != buf) {
	free(buf);
    }
}

static struct _mode	mode_map[] = {
    { .key = "validate", .func = validate },
    { .key = "parse", .func = parse },
    { .key = "multiple-one", .func = parse_one },
    { .key = "multiple-each", .func = parse_each },
    { .key = "multiple-heavy", .func = parse_heavy },
    { .key = "test", .func = test },
    { .key = NULL },
};

int
main(int argc, char **argv) {
    if (4 != argc) {
	printf("{\"name\":\"oj\",\"err\":\"expected 3 arguments\"}\n");
	exit(1);
    }
    const char	*mode = argv[1];
    const char	*filename = argv[2];
    long long	iter = strtoll(argv[3], NULL, 10);

    for (Mode m = mode_map; NULL != m->key; m++) {
	if (0 == strcmp(mode, m->key)) {
	    m->func(filename, iter);
	}
    }
    return 0;
}
