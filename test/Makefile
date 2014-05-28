CC=clang
#CC=cc
CFLAGS=-c -Wall

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

