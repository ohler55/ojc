#CC=$(shell if [ `uname` = "Darwin" ]; then echo "cc"; elif [ `uname` = "Linux" ]; then echo "gcc"; fi;)
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
ifeq ($(build),release)
	CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic
else
	CFLAGS=-c -Wall -g -O0 -std=$(CV) -pedantic
endif

SRC_DIR=.
LIB_DIR=../../lib
INC_DIR=../../include
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
HEADERS=ojc.h buf.h types.h
OBJS=$(SRCS:.c=.o)
TARGET=$(LIB_DIR)/libojc.a

all: $(LIB_DIR) $(INC_DIR) $(TARGET)
	rsync -tupRE $(HEADERS) $(INC_DIR)/ojc

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
