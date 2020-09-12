CFLAGS=-c -Wall -O3 -pedantic

SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../include
SRCS=compare.c genlog.c helper.c
OBJS=$(SRCS:.c=.o)
LIBS=-loj -lm -lpthread
TARGET=compare

all: $(TARGET)
	$(MAKE) -C oj
	$(MAKE) -C simdjson

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
	$(MAKE) -C oj clean
	$(MAKE) -C simdjson clean

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) -L$(LIB_DIR) $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/liboj.a
	$(CC) -O3 -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<
