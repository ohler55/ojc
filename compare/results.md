## Test executed on 2020-09-06

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

```
validate files/ca.json (small) 30000 times
        oj █████████████████████████████████████████████████████▍ 36.3: 1.2MB
  simdjson ███████████████████████████████████████▎ 26.7: 3.5MB

validate files/mesh.pretty.json (small) 1000 times
        oj ███████████████████████████████████████████████████████████████████████████████████████████▌1024.4: 3.1MB
  simdjson ████████████████████████████████████████████████████████████████████████████████████████████████1074.7: 7.9MB

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

## Side Note

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
