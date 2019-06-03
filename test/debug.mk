CXX = g++
CXXFLAGS = -ggdb -Wall -D_DEBUG -O1 -std=c++11
INCLUDE = -I../
LIBS = -L../lib
LIB = -lpthread -lrt -lcrypto -lssl
TARGET = ../bin/test.d
SRC_PATH = ./src

SRCS := $(wildcard $(SRC_PATH)/*.cpp)
OBJS := $(patsubst %.cpp, %.d.o, $(SRCS))

%.d.d : %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) $(INCLUDE) $< > $@.$$$$; \
	sed 's,.*\.o[ ]*:,$*.o $@ :,g'< $@.$$$$ > $@; \
	rm -f $@.$$$$
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $*.d.o $<

$(TARGET):$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIB) $(LIBS)

include $(SRCS:.cpp=.d.d)

.PHONY:clean
clean:
	rm -f ./src/*.d.[do] $(TARGET)
