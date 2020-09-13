#CFLAGS+=-c -Wall -O3 -pedantic -DMEM_DEBUG
CFLAGS+=-c -Wall -O3 -pedantic

SRC_DIR=.
LIB_DIR=../../lib
INC_DIR=../../include
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
HEADERS=oj.h buf.h debug.h
OBJS=$(SRCS:.c=.o)
TARGET=$(LIB_DIR)/liboj.a

all: $(LIB_DIR) $(INC_DIR) $(TARGET)
	rsync -tupRE $(HEADERS) $(INC_DIR)/oj

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
	$(CC) -I$(SRC_DIR) $(CFLAGS) -o $@ $<
