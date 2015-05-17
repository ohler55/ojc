# OjC

Optimized JSON in C 

## Documentation

*Documentation*: http://www.ohler.com/ojc

## Source

*GitHub* *repo*: https://github.com/ohler55/ojc

Follow [@peterohler on Twitter](http://twitter.com/#!/peterohler) for announcements and news about the OjC.

### Release 1.10.0 - May 17, 2015

 - Added support for arrays in ojc_replace().

 - Added ojc_array_remove(), ojc_array_replace(), ojc_array_insert().

 - Changed ojc_object_remove_by_pos() to ojc_remove_by_pos() which now supports
   arrays.

 - Aliased ojc_set() to ojc_replace() and ojc_aset() to ojc_areplace().

### Release 1.9.0 - April 14, 2015

 - Changed ojc_object_replace to return a boolean.

 - Added ojc_append() and ojc_aappend() for appending to a tree with a path.
 
 - Added ojc_replace() and ojc_areplace() for replacing or append to a tree with a path.
 
### Release 1.8.0 - April 7, 2015

 - Added ojc_cmp() function that compares two ojcVal values.

 - Fixed follow parser to remain open as expected.

### Release 1.7.0 - January 1, 2015

 - Added ojc_duplicate() function that make a deep copy of an element (ojcVal).

### Release 1.6.0 - December 12, 2014

 - Added option to parse decimal numbers as strings instead of doubles.

 - Added ojc_number() function.
 
### Release 1.5.0 - October 23, 2014

 - Added parse function where the caller provides a read function. Planned for
   use with external libraries such as zlib.

### Release 1.4.6 - October 7, 2014

 - Cleaned up ubuntu/g++ errors.

### Release 1.4.5 - September 26, 2014

 - Fixed memory leak with number strings.

### Release 1.4.4 - September 22, 2014

 - Added an unbuffered parse function to work with tail -f.
 
### Release 1.4.3 - August 29, 2014

 - Attempting to extract from a NULL val no longer crashes.
 
### Release 1.4.2 - August 28, 2014

 - Fixed bug in parse destroy during callback parsing.
 
### Release 1.4.1 - August 21, 2014

 - Added cleanup function to free memory used in re-use pools.

### Release 1.4.0 - August 15, 2014

 - Added functions to get members of an object and an array.

### Release 1.3.0 -July 31, 2014

 - Added support for non quoted words as values if a flag is set to allow the
   non-standard feature. Functions create and get values from word values also
   added.

### Release 1.2.5 -July 21, 2014

 - Added object functions for changing the members of the object.

### Release 1.2.4 -July 6, 2014

 - Added function to set the key of a value.

### Release 1.2.3 -June 26, 2014

 - Fixed buffer overflow by 1 error.

### Release 1.2.2 -June 25, 2014

 - Allow raw \n in output when ojc newline ok is true.

### Release 1.2.1 -June 20, 2014

 - Allow raw \n in output.

### Release 1.2.0 -June 10, 2014

 - Added push and pop to array values.

### Release 1.1.0 - May 28, 2014

 - Fixed ubuntu compilation.

 - Changed callback for parsing to allow the parsing to be aborted by the callback.

### Release 1.0.2 - May 28, 2014

 - Added static initializer for ojcErr.

 - Added ojc_str().

### Release 1.0.1 - May 27, 2014

 - Corrected compile errors with clang and made ojc.h C++ compatible.

### Release 1.0.0 - May 26, 2014

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

