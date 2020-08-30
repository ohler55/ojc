CFLAGS=-c -Wall -O3 -pedantic

SRC_DIR=.
LIB_DIR=../../lib
INC_DIR=../../include
LIBS=-loj -lm -lpthread
TARGET=ojparse
OBJS=ojparse.o ../helper.o

all: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -O3 -o $@ $(OBJS) -L$(LIB_DIR) $(LIBS)

%.o : %.c  $(HEADERS) $(LIB_DIR)/liboj.a
	$(CC) -I. -I$(INC_DIR) -I$(SRC_DIR) $(CFLAGS) -o $@ $<
