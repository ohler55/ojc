CC=$(shell if [ `uname` = "Darwin" ]; then echo "c++"; elif [ `uname` = "Linux" ]; then echo "g++"; fi;)
#CV=$(shell if [ `uname` = "Darwin" ]; then echo "c++17"; elif [ `uname` = "Linux" ]; then echo "c++17"; fi;)
#CFLAGS=-Wall -O3 -std=$(CV)

SRCS=$(shell find . -type f -name "*.cpp" -print)
LIBS=-lm
TARGET=simdbench

all: $(TARGET)

clean:
	$(RM) *.o
	$(RM) $(TARGET)

$(TARGET): $(SRCS)
	$(CC) -O3 -Wall -o $(TARGET) $(SRCS) $(LIBS)
