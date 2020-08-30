CC=$(shell if [ `uname` = "Darwin" ]; then echo "c++"; elif [ `uname` = "Linux" ]; then echo "g++"; fi;)
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c++17"; elif [ `uname` = "Linux" ]; then echo "gnu++17"; fi;)
CFLAGS=-Wall -O3 -std=$(CV)
#CFLAGS=-Wall -O3

SRCS=$(shell find . -type f -name "*.cpp" -print)
OBJS=$(SRCS:.cpp=.o)
LIBS=-lm
TARGET=simdbench

all: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

$(TARGET): simdbench.o
	$(CC) -O3 -o $@ simdbench.o $(LIBS)

%.o : %.cpp  $(HEADERS)
	$(CC) -O3 -I. -I$(SRC_DIR) $(CFLAGS) -o $@ $<
