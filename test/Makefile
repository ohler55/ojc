CC=$(shell if [ `uname` = "Darwin" ]; then echo "cc"; elif [ `uname` = "Linux" ]; then echo "gcc-6"; fi;)
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
ifeq ($(build),release)
	CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic
else
	CFLAGS=-c -Wall -g -O0 -std=$(CV) -pedantic
endif

SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../src
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)
LIBS=-lojc -lm
TARGET=run_tests
BENCH=bench

all: $(TARGET) $(BENCH)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(TARGET): tests.o ut.o
	$(CC) -o $@ tests.o ut.o -L$(LIB_DIR) $(LIBS)

$(BENCH): benchmark.o
	$(CC) -o $@ benchmark.o -L$(LIB_DIR)  $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/libojc.a
	$(CC) -O3 -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<

test: all
	$(TARGET)
