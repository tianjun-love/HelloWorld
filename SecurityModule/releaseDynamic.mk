CXX = g++
CXXFLAGS = -Wall -O3 -std=c++11 -shared -fPIC
INCLUDE = -I../PublicInclude/windows
LIBS = 
LIB = 
TARGET = ../lib/libSecurityModule.r.so
SRC_PATH = ./src

SRCS := $(wildcard $(SRC_PATH)/*.cpp)
OBJS := $(patsubst %.cpp, %.r.o, $(SRCS))

%.r.d : %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) $(INCLUDE) $< > $@.$$$$; \
	sed 's,.*\.o[ ]*:,$*.o $@ :,g'< $@.$$$$ > $@; \
	rm -f $@.$$$$
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $*.r.o $<

$(TARGET):$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIB) $(LIBS)

include $(SRCS:.cpp=.r.d)

.PHONY:clean
clean:
	rm -f ./src/*.r.[do] $(TARGET)
