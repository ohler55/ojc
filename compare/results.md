## Test executed on 2020-09-06

First some basic tests were run to make sure both parsers were able to
detect errors that some parsers (such as the earlier release of
simdjosn) miss. Some features are also listed for comparison.

| Test                             | oj         | simdjson   |
| -------------------------------- | ---------- | ---------- |
| Valid unicode                    |     ✅     |     ✅     |
| Valid UTF-8                      |     ✅     |     ✅     |
| Detect invalid UTF-8             |     ✅     |     ✅     |
| Large exponent (309)             |     ✅     |     ❌     |
| Larger exponent (4000)           |     ✅     |     ❌     |
| Large integer (20+ digits)       |     ✅     |     ❌     |
| Detect invalid (192.168.10.100)  |     ✅     |     ✅     |
| Detect invalid (1e2e3)           |     ✅     |     ✅     |
| Detect invalid (-0.0-)           |     ✅     |     ✅     |
| Detect invalid (uuid)            |     ✅     |     ✅     |

| Feature                          | oj         | simdjson   |
| -------------------------------- | ---------- | ---------- |
| Multiple JSON (one per line)     |     ✅     |     ✅     |
| Multiple JSON (any format)       |     ✅     |     ❌     |
| Build JSON object                |     ✅     |     ❌     |
| Formatted write                  |     ✅     |     ❌     |
| Parse file larger than memory    |     ✅     |     ❌     |


Next are some benchmarks. Starting simple validation then on to
parsing a single JSON string. Note a file is listed but time on the
parsing does not start until the file has been loaded into
memory. Next is parsing files that contain multiple JSON
documents. Since simdjson does not support reading general multiple
JSON files the benchmarks were limited to file that have exactly one
JSON document per line. Benchmarks were done on mock log files
mimicking a server that logs GraphQL requests and responses. Three file sizes were used;

 - 1GB file representing a relatively small log file
 - 10GB file for a larger task and to check memory use
 - 20GB file to represent a file too large to fit into memory (the benchmark machine had 16GB)

To make the test more interesting both a light and heavy load
benchmarks were run. The light load consisted of just counting the
number of documents and touching each JSON element so pretty much no
load at all. The heavy load spun for 8 microseconds to simulate some
processing on the parsed document. That seemed like a reasonable, if
not a light representation of what a real application might do.

```
validate files/ca.json (small) 30000 times
        oj █████████████████████████████████████████████████████▍ 36.3: 1.2MB
  simdjson ███████████████████████████████████████▎ 26.7: 3.5MB

parse files/ca.json (small) 30000 times
        oj ████████████████████████████████████████████████████████████████████████████████████████████████ 65.3: 2.5MB
  simdjson ███████████████████████████████████████▋ 27.0: 3.5MB

multiple-light files/1G.json (small) 1 times
        oj ███████████████████████████████████████████▏  7.3: 2.0MB
  simdjson ██████████████████████████████████████████████████████████▊  9.9: 1.1GB

multiple-heavy files/1G.json (small) 1 times
        oj ██████████████████████████████████████████████████████████████████████▍ 11.9: 3.3MB
  simdjson ████████████████████████████████████████████████████████████████████████████████████████████████ 16.2: 1.1GB

multiple-light files/10G.json (large) 1 times
        oj ███████████████████████████████████████▌  6.6: 1.9MB
  simdjson ████████████████████████████████████████████████████████▎  9.5: 10GB

multiple-heavy files/10G.json (large) 1 times
        oj █████████████████████████████████████████████████████████████████ 10.9: 5.2MB
  simdjson ██████████████████████████████████████████████████████████████████████████████████████████████▍ 15.9: 10GB

multiple-light files/20G.json (huge) 1 times
        oj █████████████████████████████████████▋  6.3: 1.8MB
  simdjson Error allocating memory, we're most likely out of memory

multiple-heavy files/20G.json (huge) 1 times
        oj ████████████████████████████████████████████████████████████████████████████████████▌ 14.2: 16MB
  simdjson Error allocating memory, we're most likely out of memory
```

 Lower values (shorter bars) are better in all cases. The bar graph
 compares the parsing performance. The parsing time microsecond per
 line/JSON is listed at the end of the bar along with the memory used.

Tests run on:
```
 OS:              Ubuntu 18.04.5 LTS
 Processor:       Intel(R) Core(TM) i7-8700 CPU
 Cores:           12
 Processor Speed: 3.20GHz
 Memory:          16 GB
 Disk:            KINGSTON SA400S37240G (240 GB SSD)
```

## Side Notes

The large file benchmark was run with the files on an external SSD
with these similar results:

```
multiple-light /media/ohler/backup/bench-files/10G.json (large) 1 times
        oj █████████████████████████████████████████████████████████████▉ 18.0: 2.1MB
  simdjson █████████████████████████████████████████████████████████████████████▍ 20.2: 10GB

multiple-heavy /media/ohler/backup/bench-files/10G.json (large) 1 times
        oj █████████████████████████████████████████████████████████████▊ 18.0: 8.2MB
  simdjson ████████████████████████████████████████████████████████████████████████████████████████████████ 27.9: 10GB
```

One of the simdjson benchmarks files was use to rerun the
validation. OjC performed better than simdjson but since the file is
mostly number and not a mix of different types it was not used in the
comparison.

```
validate files/mesh.pretty.json (small) 1000 times
        oj ███████████████████████████████████████████████████████████████████████████████████████████▌1024.4: 3.1MB
  simdjson ████████████████████████████████████████████████████████████████████████████████████████████████1074.7: 7.9MB
```
