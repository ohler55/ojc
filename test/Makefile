CFLAGS+=-Wall -O3 -pedantic

SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../include
#INC_DIR=../src
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)
TEST_OBJS=$(SRCS)
LIBS=-loj -lm -lpthread
TARGET=oj_tests

all: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -L$(LIB_DIR) $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/liboj.a
	$(CC) -c -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<

test: all
	./$(TARGET)
