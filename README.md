# OjC

Optimized JSON in C 

## Documentation

*Documentation*: http://www.ohler.com/ojc

## Source

*GitHub* *repo*: https://github.com/ohler55/ojc

Follow [@peterohler on Twitter](http://twitter.com/#!/peterohler) for announcements and news about the OjC.

### Current Release 1.0.1

 - Corrected compile errors with clang and made ojc.h C++ compatible.

### Release 1.0.0

 - Initial release.

## Description

Optimized JSON in C (OjC), as the name implies, was written to provide speed
optimized JSON handling. It is derived from the underlying C parser in
[Oj](http://www.ohler.com/oj). The intended use is for applications that need
the maximum performance when reading large JSON document from a file or socket.

Besides being a true streaming parser OjC produces a structure that handles all
JSON constructs. It does not use a Hash or Map to represent the JSON object type
but keeps all occurances of a pairs in the object element. Duplicates are
allowed just as they are in JSON documents.

Multiple JSON elements are allowed in a single stream or from a socket. A
callback mechanism is provided to return full JSON elements for each entry in a
JSON stream.

## Simple JSON Parsing Example

```c
include "ojc.h"

void
sample() {
    char                buf[256];
    ojcVal              val;
    struct _ojcErr      err;
    const char          *json = "[null,true,\"word\",{\"a\":1234}";

    ojc_err_init(&err);
    // no callback
    val = ojc_parse_str(&err, json, 0, 0);
    ojc_fill(&err, val, 0, buf, sizeof(buf));
    if (OJC_OK != err) {
        printf("error: %s\n", err.msg);
        return;
    }
    printf("parsed json '%s'\n", buf);
    ojc_destroy(val);
}

```

## Simple Callback JSON Parsing Example

```c
include "ojc.h"

static bool
each__callback(ojcVal val, void *ctx) {
    char                buf[64];
    struct _ojcErr      err;
    
    ojc_err_init(&err);
    ojc_fill(&err, val, 0, buf, sizeof(buf));
    if (OJC_OK != err) {
        printf("error: %s\n", err.msg);
    } else {
        printf("parsed json '%s'\n", buf);
    }
    return true;
}

void
sample() {
    struct _ojcErr      err;
    const char          *json = "{\"a\":111} {\"b\":222} {\"c\":333}";

    ojc_err_init(&err);
    ojc_parse_str(&err, json, each_callback, 0);
    if (OJC_OK != err) {
        printf("error: %s\n", err.msg);
    }
}

```

