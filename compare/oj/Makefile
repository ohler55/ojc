CFLAGS=-c -Wall -O3 -pedantic

SRC_DIR=.
LIB_DIR=../../lib
INC_DIR=../../include
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)
LIBS=-loj -lm -lpthread
TARGET=ojbench

all: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(TARGET): ojbench.o
	$(CC) -o $@ ojbench.o -L$(LIB_DIR) $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/liboj.a
	$(CC) -O3 -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<
