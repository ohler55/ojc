#CC=$(shell if [ `uname` = "Darwin" ]; then echo "c++"; elif [ `uname` = "Linux" ]; then echo "g++"; fi;)
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
#CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic
CFLAGS=-c -Wall -O3

SRC_DIR=.
LIB_DIR=../../lib
INC_DIR=../../src
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)
LIBS=-lojc -lm
TARGET=run_tests
BENCH=ojcbench

all: $(BENCH)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(BENCH): ojcbench.o
	$(CC) -o $@ ojcbench.o -L$(LIB_DIR)  $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/libojc.a
	$(CC) -O3 -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<

test: all
	./$(TARGET)
