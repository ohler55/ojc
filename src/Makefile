#CC=clang
CC=cc
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
#CFLAGS=-c -Wall -g -O1 -std=$(CV) -pedantic
CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic


SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../include
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
HEADERS=$(shell find $(SRC_DIR) -type f -name "*.h" -print)
OBJS=$(SRCS:.c=.o)
TARGET=$(LIB_DIR)/libojc.a

all: $(LIB_DIR) $(INC_DIR) $(TARGET)
	rsync -tupRE $(HEADERS) $(INC_DIR)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(LIB_DIR):
	mkdir -p $@

$(INC_DIR):
	mkdir -p $@

$(TARGET): $(OBJS)
	ar -rcs $@ $(OBJS)

%.o : %.c  $(HEADERS)
	$(CC) -I. -I$(SRC_DIR) $(CFLAGS) -o $@ $<
