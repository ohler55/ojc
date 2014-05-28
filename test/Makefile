CC=clang
#CC=cc
CFLAGS=-c -Wall

SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../src
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)
TARGET=run_tests

all: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(TARGET): tests.o ut.o
	$(CC) -o $@ tests.o ut.o -L$(LIB_DIR) -lojc

benchmark: benchmark.o
	$(CC) -o $@ benchmark.o -L$(LIB_DIR) -lojc

%.o : %.c  $(HEADERS) $(LIB_DIR)/libojc.a
	$(CC) -O3 -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<

