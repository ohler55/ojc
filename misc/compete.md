# OjC vs Simdjson as a Track and Field Events

It's a lovely cool day here are Benchmark stadium. I'm Pat and next to
me is Chris. We will be covering the exciting competition between the
OjC JSON library and the Simdjson parser as they challenge each other
in multiple events.

### The Venue

All the events will take place in the Benchmark stadium and in the
surrounding town which covers 16GB of memory and a Kingston 240GB
SSD. A 12 core Intel i7-8700 clocked at 3.20GHz will keep the
competitors going along with Ubuntu 18.04. I'm sure the town's modern
utilities will allow the runners to demonstrate their prowness.

Lets look at whats on the schedule for today. The first even, as might
be expected is a look at the qualifications of each contestant. Each
parser must be a validating parser that check basic structure as well
as checking for UTF-8 and numbers. Following the qualification
confirmations the parsing begins. The courses range from short 44K
sprint and go up to a 20GB half marathon. Different handicaps are
applied to see how well each parser handles added processing
load. More on that later. Each race is designed to represent a real
use case. Addition build and write events have been cancelled for
today as the Simdjson team was not able to field contestants in those
events and it wouldn't be very exciting to watch a single OjC
contestant participate in those events.

Each event is kicked off by the [`compare`](../compare) application
that lets each team pick a parser to compete. The team application is
given a course to run (file to parse), a method to use (loading), and
the number of laps (iterations) to run. The results are collected and
placed on the score board.

### The Teams

Now lets look at the teams. Chris, can you tell us about the teams we
have here today?

Sure Pat, with pleasure.

Lets look at team OjC first since this is their home town. Team OjC
has quite a few atheletes here today. They are supported by Pete
Ohler, the coach and a recent contributor for a rather small team. The
athletes is heavy on longer distance runners or rather parsers but
Vicky validator will be running the validation benchmarks while Pamela
Parser, the other sprinter on the team will be the OjC entry for the
44K parse. For the longer 1GB, 10GB, and 20GB races Peter Paul Parser,
also know as Triple-P, will take on the challengers for the lightly
loaded courses and Porter will take up the torch for the heavier
loaded races. Bob the builder and Whitney the writer will be taking
the day off since team Simdjson is not fielding a builder or writer
today. That pretty much rounds out the OjC team for today.

On the other side of the stadium we have team Simdjson. They enter the
field with two parsers, Milly, an in memory parser that will compete
against Vicky and Triple-P in the sprints and Manny who will take on
the OjC long distance parsers. Backing up the parsers are two head
coaches and a huge support team of another 61 members. They are
confident with their earlier sprint results. It will be interesting to
see how the claims of more than 2GB per second holds up on different
courses.

With the teams covered, back to you Pat for the start of the
qualifying rounds.

### Qualifiers

It is expected that each contestant will qualify although not long ago
the Simdjson parsers were a bit lax with numeric parsing. They claim
to have tightened up the numeric parsing though so we should expect no
issues with both teams qualifying. The results are just coming in from
the validation tests. Lets see if our expectations were on the money.

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

Well, both teams passed the structure, unicode, and UTF-8 tests with
flying colors. The numeric validations tests are not so clear
though. It seems Simdjson is limited to a 64 bit double and rejects or
ignores values or digits that fall outside that space. Without big
number support it falsely reports errors on what are commonly referred
to as big numbers or big decimals. The judges are discussing whether
that will be a show stopper for todays events it could certainly be an
issue for some scientific data. Team OjC on the otherhand supports up
to a long double and then reverts to it's own version of big numbers
giving it a solid acceptance in the qualification tests.

While we are waiting for the judges lets look at the results of the
feature interviews. The features are generally for information only
but one item does stand out. Usually when claiming to support parsing
a file containing multiple JSON documents it is assumed that the
documents can be formatted in any valid fashion. We can see that
Manny, the Simdjson competitor has restrictions on what course he will
run. The course can not includes any new lines in the JSON documents
themselves and each one must be separated by exactly one new
line. This was known going into the events and it was agreed ahead of
time that the courses would be groomed to support the restricted
format.

| Feature                          | oj         | simdjson   |
| -------------------------------- | ---------- | ---------- |
| Multiple JSON (one per line)     |     ✅     |     ✅     |
| Multiple JSON (any format)       |     ✅     |     ❌     |
| Build JSON object                |     ✅     |     ❌     |
| Formatted write                  |     ✅     |     ❌     |
| Parse file larger than memory    |     ✅     |     ❌     |

While we were telling all of you about the features the judges have
come to a decision. Since a majority of use cases fall within what
Simdjson is capable of parsing all large number obstacles will be
removed from the course with a footnote indicating the limitation made
for the races. The decision was expected. After all everyone showed up
today to see some races. It wouldn't do to cancel at the last moment.

I'm going to hand it back to you Chris while I give my throat a rest.

### Validation

The first race is a sprint with nothing to slow down the runners. It
is a straight out check with no other restrictions. The course is
composed of one file that is an array of information on cities in
Canada. Each array elments looks like this sample:

```json
  {
    "city": "Toronto",
    "admin": "Ontario",
    "population_proper": 3934421,
    "capital": "admin",
    "lat": 43.666667,
    "lng": -79.416667,
    "population": 5213000
  },
```

It is a mixture of strings and numbers and sometimes a null. There are
no big numbers on the course but there are unicode characters.

The file is loaded into memory first so the run is strictly in
memory. Earlier results between the two contestants were closely
matches with Vicky from OjC talking the numeric heavy file like a mesh
file and Milly from Simdjson taking the string heavy files. With the
`ca.json` course being a mix the outcome is still waiting to be
determined.

As the runners get ready the course is cleared. The gun goes off and
the runner leap into action. Milly takes the lead. Vicky is trying
hard to catch her but Milly is just too fast on this course. Simdjson
takes the validation race.

```
validate files/ca.json (small) 30000 times
        oj █████████████████████████████████████████████████████▍ 36.3: 1.2MB
  simdjson ███████████████████████████████████████▎ 26.7: 3.5MB
```

### 44K Parse Sprint

Miily from team Simdjson is again at the starting blocks. She was fast
with the validation and we expect much the same with the trivial amout
of checking required for the parsing run on the same course. Pamela
for OjC sets value elements while running which is helpful if the
values are to be used after the race but for this course there is no
need for that so Pamela will be doing extra work for nothing.

As the starting gun goes off it is clear Pamela is out matched. Milly
leaves her in the dust. This one again goes to team Simdjson.

```
parse files/ca.json (small) 30000 times
        oj ████████████████████████████████████████████████████████████████████████████████████████████████ 65.3: 2.5MB
  simdjson ███████████████████████████████████████▋ 27.0: 3.5MB
```

### Long Distance

Well, team Simdjson was the clear winner on the sprints or small JSON
documents in memory. Next up are the longer distance races. The
courses are not as flat the track that the sprints were run on. The
longer courses simulate a GraphQL server log with request and response
log entries repeated for the length of the course. Each course is run
unencoumbered (just touch each element and counts the entries) and
then loaded with a backpack (simulate some non-trivial processing for
each entry as if the caller was actually parsing for some
reason).

Three distances are being run today starting with a short 1GB course
representing a case where the load on the stadium (machine) is not
expected to be very disruptive. Following the 1GB event will be the
10GB run. This course goes outside the stadium but stays on the
stadium grounds which cover 16GB. The third long distance race is just
short of a half marathon at 20GB. The 20GB course, the favorite of the
locals takes the runners through the scenic town of Benchmark where
crowds are expected to line the streets cheering on the runners.

Okay, Chris, it looks like the runners are lining up. Why don't you
keep the listeners up to date on the race.

#### 1GB Log File

Thanks Pat. The first race is the 1GB log file race. Triple-P from OjC
is going to be running first. He has to run the course and touch each
element in the JSON documents on the course. As a callback parser he
will be shouting out the elements as he runs. We are expected a steady
stream of chatter.

And he's off. With no hesitation at the start he is literally buzzing
around the course. As short as the course is it doesn't take long to
finish. He completed 412,900 documents course in about 3 seconds for
an average of 7.3 microseconds per document. Thats a pretty decent
showing. Let see how Manny from the Simdjson team does.

Manny is asking to clear the course area of all spectators. He needs a
clear view of the whole course before getting started. There is a
little grumbling but the course is only 1GB so is no time at all the
course is clear and the race can begin.

The starting gun fires and Manny seems to be hesitating.

There is goes now. He is running at a pretty good clip. Faster than
Triple-P but that delay at the start is going to be tough to make up
for.

That delay at the start was too much to overcome. His average was only
9.9 microseconds per document. It looks like OjC takes this one.

```
multiple-light files/1G.json (small) 1 times
        oj ███████████████████████████████████████████▏  7.3: 2.0MB
  simdjson ██████████████████████████████████████████████████████████▊  9.9: 1.1GB
```

#### 1GB Log File Under Load

The next race is on the same 1GB course. The course has been cleared
and is ready for Porter from team OjC. Porter has shouldered his
backpack now. Remember this is mean to mimick processing load an
application might perform for each JSON document. While we are
interested in how fast each runner (parse) is they also have to work
with others in the real world so this race is meant to show not only
how fast each runner is but how well they play with others while
parsing. A 8 microsecond load is a pretty light load but enough to
have make a difference when running all out.

And there is the gun. Like Triple-P earlier, Porter starts out fast. A
little more hesitation than Triple-P but he is moving along faster
than one would expect with that extra load. His pace is pretty
consistent though as he heads around the last bend in the course. A
finish of 11.9 microseconds per document. Quite a slowdown from
Triple-P's time but faster than the 15 microseconds per document you
get by simply adding the load and document time for a light
load. Amazingly only 3.3MB were used by Porter during the run.

Manny is stepping up to the starting line with backpack in place. The
course has been clear and the gun goes off. Manny is waiting for
something again but finally starts running. He seems to be suffering
under the load handing off the load on every document before
continuing.

And there's the finish at 16.2 microseconds per document after using
1.1GB of memory for the duration of the run.

```
multiple-heavy files/1G.json (small) 1 times
        oj ██████████████████████████████████████████████████████████████████████▍ 11.9: 3.3MB
  simdjson ████████████████████████████████████████████████████████████████████████████████████████████████ 16.2: 1.1GB
```

I'm going to hand it back to you Pat for the 10GB race.

#### 10GB Log File

Now we are getting into some of my favorites. A 10GB log file run is
long enough to challenge the sprinters and shorter distance runners
but not necessarily long enough to let the marathon and ultra runners
shine. It will be interesting to see if Manny from team Simdjson
handles this one. He insisted on clearing the 1GB course before
running. If he takes the same approach on this one there are going to
be some angry spectators when they get told to leave the stadium.

With the runner order established with the first race, Triple-P is up
first. He looks confident after taking the 1GB race as he steps up to
the line.

Triple-P steps off the line at a steady pace. It looks exactly like
the pace he held for the 1GB. We might as well sit back, this will
take a little while.

And Triple-P comes across the finish line a bit faster than the 1GB
run. Maybe that was a warm-up. His time is 6.6 microseconds per
document while still maintaining less than 2MB of memory use.

Manny seems a little more on edge this time. Oh, no, he is asking that
the stadium and surrounding grounds be cleared of all spectators. He
needs the full 10GB for his run. He is going to get what he asked for
though as people make their way out of the stadium and back to town.

The gun was fired by Manny is just standing there. He is looking at
his shoes. Now bending over to retie his shoes. Finally he is off and
running. He is fast, burning up the course.

With a final average of 9.5 microseconds he was a fraction faster than
the 1GB run but using 10GB of memory did not garner any new fans.

```
multiple-light files/10G.json (large) 1 times
        oj ███████████████████████████████████████▌  6.6: 1.9MB
  simdjson ████████████████████████████████████████████████████████▎  9.5: 10GB
```

#### 1GB Log File Under Load

With the unloaded 10GB being so similar to the 1GB we might expect
similar results for the parse under load.

Sure enough, Triple-P has started with the same pace as with the 1GB
run. As he comes across the finish line his time is actually a little
better at 10.9 microseconds per JSON document. It might be the
variation in the course or some maybe just how he was feeling. In any
case it is nice to see he used up a little more memory at 5.2MB which
is just enough to keep the course clear a few meters (MBs) in front of
him.

Most of the spectators stayed in town enjoying a beer while waiting
for Manny to run the loaded 10GB. He jumped around a little at the
start. Maybe testing the load or getting in the right frame of mind
after the gun went off but he did eventually start running. His pace
with the load was pretty much the same as the 1GB with a finish
average of 15.9 microseconds per document.

```
multiple-heavy files/10G.json (large) 1 times
        oj █████████████████████████████████████████████████████████████████ 10.9: 5.2MB
  simdjson ██████████████████████████████████████████████████████████████████████████████████████████████▍ 15.9: 10GB
```

#### 20GB Log File

All those folks that had to leave the stadium earlier are in town
waiting for the half marathon to get started. The race director must
have foreseen the way thing would go because the course goes past
pretty much ever bar and pub with an outdoor patio in town.

Triple-P loves the crowds and they love him as he uses only 1.8MBs of
memory so the crowds can get as close as they want, keeping the
acceptable social distance of course.

Triple-P seems to be running even better than he did on the 1GB and
10GB runs. What an entertainer. At least he didn't accept any of the
beers offered from the spectators.

Crossing the finish with a 6.3 microsecond average will likely assure
him a win but Manny with team Simdjson could surprise us.

Well surprise he did. Manny wants the whole town cleared out to give
him the space he needs to run. After kicking everyone out of the bars
and clearing out the town he realized there was still not enough space
to run and gave up. It's disappointing he brought the town to a
standstill before giving up. The race goes to OjC.

```
multiple-light files/20G.json (huge) 1 times
        oj █████████████████████████████████████▋  6.3: 1.8MB
  simdjson Error allocating memory, we're most likely out of memory
```

#### 20GB Log File Under Load

With the town empty the next event, the 20GB under load will have to
wait until people return. Not to be deterred though, the race must go
on. Porter from team OjC is ready at the starting line. He claims a
20GB is nothing and will do as well as the 1GB and 10GB. We'll see if
that holds true or not.

It seems all the atheletes on team OjC likes spectators. Porter starts
out with a few MBs of memory and never uses more than 4.9MBs. He keeps
a steady pace through town and finishes with an impressive 10.8
microseconds per document.

Now it's Manny's turn again. He seems to have gained confidence
because he is going to try again. Everyone is again cleared out of the
town. Just like the previous time Manny is surprised that there is not
enough memory to run and gives up after halting the whole town while
he tried to grab all the memory in town. I don't think the judges will
allow team Simdjson to attempt a long distance run again any time
soon.

```
multiple-heavy files/20G.json (huge) 1 times
        oj ████████████████████████████████████████████████████████████████████████████████████████████████ 12.8: 4.3MB
  simdjson Error allocating memory, we're most likely out of memory
```

### Awards

Well, Chris, with the races over all that's left is the awards
ceremony. While those are going on lets look at the results we saw
today.

The qualifications and feature checks highlighted a few differences
with the teams we had here today. I have to say that each team is
fairly strong but team Simdjson came up a little short on numeric
parsing by not being able to handle big numbers. It is understandable
that Simdjson is strictly a parser so not have a build and write
feature can't really be counted against the team although it does
limit how the parse can be used. What was disappointing was the
limited multiple JSON document support by Simdjson as it could only
parse a restricted format of multiple document JSON files.

The races were quite interesting though. Validation was fairly close
but the sprint for small file parsing put Simdjson far ahead of
OjC. As we moved to the longer races the story changed though. OjC
dominated the long larger file races just as Simdjson dominated the
sprints.

Lets review the awards for each race.

| Race                             | oj         | simdjson   |
| -------------------------------- | ---------- | ---------- |
| Validate small file              |  🥈        |  🥇        |
| Parse small file                 |  🥈        |  🥇        |
| Parse Multiple JSON small file   |  🥇        |  🥈        |
| Parse Multiple JSON small loaded |  🥇        |  🥈        |
| Parse Multiple JSON large file   |  🥇        |  🥈        |
| Parse Multiple JSON large loaded |  🥇        |  🥈        |
| Parse Multiple JSON huge file    |  🥇        |     ❌     |
| Parse Multiple JSON huge loaded  |  🥇        |     ❌     |

There isn't much overlap in the specialities of each team. Team
Simdjson clearly takes the honors for parsing smaller JSON data that
is already in memory that do no contain large numbers. On the other
hand, OjC stands out as the preferred choice in file parsing, handling
larger numeric elements, and optimizing JSON processing. OjC also
supports building, modifying, and formatting JSON when writing.

Pat and I enjoyed todays events. We hope to see you all next year.
