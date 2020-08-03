#CC=$(shell if [ `uname` = "Darwin" ]; then echo "cc"; elif [ `uname` = "Linux" ]; then echo "gcc"; fi;)
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic

SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../include
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)
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

$(OJ_TARGET): oj_tests.o ut.o
	$(CC) -o $@ oj_tests.o ut.o -L$(LIB_DIR) $(LIBS)

$(BENCH): benchmark.o
	$(CC) -o $@ benchmark.o -L$(LIB_DIR)  $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/libojc.a $(LIB_DIR)/liboj.a
	$(CC) -O3 -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<

test: all
	./$(TARGET)
