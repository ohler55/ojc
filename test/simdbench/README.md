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
