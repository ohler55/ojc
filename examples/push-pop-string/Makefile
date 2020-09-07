CFLAGS=-c -Wall -O3 -pedantic

LIB_DIR=../../lib
INC_DIR=../../include
LIBS=-loj -lm -lpthread
TARGET=example
OBJS=example.o

all: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -O3 -o $@ $(OBJS) -L$(LIB_DIR) $(LIBS)

%.o : %.c $(LIB_DIR)/liboj.a
	$(CC) -I. -I$(INC_DIR) $(CFLAGS) -o $@ $<
