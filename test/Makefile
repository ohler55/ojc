CFLAGS=-c -Wall -O3 -pedantic

SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../include
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)
TEST_OBJS=oj_tests.o validate_tests.o pushpop_tests.o parse_tests.o chunk_tests.o write_tests.o ut.o
LIBS=-lojc -loj -lm -lpthread
TARGET=run_tests
OJ_TARGET=oj_tests
BENCH=bench

all: $(TARGET) $(OJ_TARGET) $(BENCH)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
	$(RM) $(OJ_TARGET)

$(TARGET): tests.o ut.o
	$(CC) -o $@ tests.o ut.o -L$(LIB_DIR) $(LIBS)

$(OJ_TARGET): $(TEST_OBJS)
	$(CC) -O3 -o $@ $(TEST_OBJS) -L$(LIB_DIR) $(LIBS)

$(BENCH): benchmark.o
	$(CC) -O3 -o $@ benchmark.o -L$(LIB_DIR)  $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/libojc.a $(LIB_DIR)/liboj.a
	$(CC) -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<

test: all
	./$(TARGET)
