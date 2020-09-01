CC=$(shell if [ `uname` = "Darwin" ]; then echo "c++"; elif [ `uname` = "Linux" ]; then echo "g++"; fi;)
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c++17"; elif [ `uname` = "Linux" ]; then echo "c++17"; fi;)
#CFLAGS=-Wall -O3 -std=$(CV)
CFLAGS=-Wall -O3 -DSIMDJSON_THREADS_ENABLED

SRCS=$(shell find . -type f -name "*.cpp" -print) ../helper.c
LIBS=-lm -pthread
TARGET=simdjson

all: $(TARGET)

clean:
	$(RM) *.o
	$(RM) $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I../../include -o $(TARGET) $(SRCS) $(LIBS)
