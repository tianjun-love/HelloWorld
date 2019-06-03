CXX = g++
CXXFLAGS = -g -D_DEBUG -Wall -O1 -std=c++11 -shared -fPIC
INCLUDE = -I../PublicInclude/windows
LIBS = 
LIB = 
TARGET = ../lib/libSecurityModule.d.so
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
