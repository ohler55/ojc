# simdjson vs ojc benchmarks

This is the first try at the benchmarks. The apps are separate for now
but will be combined later.

To build the simdbench first download the simdjson.cpp and sindjson.h to this directory:

```
wget https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.h https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.cpp https://raw.githubusercontent.com/simdjson/simdjson/master/jsonexamples/twitter.json
```

Then compile:

```
c++ simdbench.cpp simdjson.cpp -o simdbench
``

To build the ojcbench type:

```
make
```

Running each on resulting in:

simdjson_parse   1000000 entries in 5119.579 msecs. (  195 iterations/msec)
ojc_parse_str    1000000 entries in 1945.454 msecs. (  514 iterations/msec)

OjC is 2.6 times faster.
