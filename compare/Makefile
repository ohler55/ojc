CFLAGS=-c -Wall -O3 -pedantic

SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../include
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)
LIBS=-loj -lm -lpthread
TARGET=compare

all: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(TARGET): compare.o genlog.o
	$(CC) -o $@ compare.o genlog.o -L$(LIB_DIR) $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/liboj.a
	$(CC) -O3 -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<
