#CC=$(shell if [ `uname` = "Darwin" ]; then echo "cc"; elif [ `uname` = "Linux" ]; then echo "gcc"; fi;)
#CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
#CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic
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

$(TARGET): compare.o
	$(CC) -o $@ compare.o -L$(LIB_DIR) $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/liboj.a
	$(CC) -O3 -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<
