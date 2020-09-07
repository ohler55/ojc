# [![{}j](http://www.ohler.com/dev/images/ojc.svg)](http://www.ohler.com/ojc)

[![Build Status](https://img.shields.io/travis/ohler55/ojc/master.svg?logo=travis)](http://travis-ci.org/ohler55/ojc?branch=master)

Optimized JSON in C

## Description

Optimized JSON in C (OjC), as the name implies, was written to provide
optimized JSON handling. It is derived from the underlying C parser in
[Oj](http://www.ohler.com/oj) and more recently
[OjG](https://github.com/ohler55/ojg). The intended use is for
applications that need the maximum performance when reading large JSON
document from a file or socket.

Besides being a true streaming parser OjC produces a structure that handles all
JSON constructs. It does not use a Hash or Map to represent the JSON object type
but keeps all occurances of a pairs in the object element.

Multiple JSON elements are allowed in a single stream or from a socket. A
callback mechanism is provided to return full JSON elements for each entry in a
JSON stream.

## Simple JSON Parsing Example

```c
#include <stdio.h>
#include "oj/oj.h"

int
main(int argc, char **argv) {
    const char      *str ="{\"num\": 12.34e567}"
    struct _ojErr   err = OJ_ERR_INIT;
    ojVal           val;

    if (NULL == (val = oj_parse_str(&err, str, NULL))) {
        printf("Parse error: %s at %d:%d\n", err.msg, err.line, err.col);
    } else {
        oj_destroy(val);
    }
    oj_cleanup();
    return err.code;
}
```

More example can be be found in the [examples](examples) directory.

## Benchmarks and Comparisons

A comparison of OjC and simdjson is in the [compare](compare)
directory with results in the [results.md](compare/results.md) file.

## Documentation

*Documentation*: http://www.ohler.com/ojc

## Source

*GitHub* *repo*: https://github.com/ohler55/ojc

## Releases

See [CHANGELOG](CHANGELOG.md)
