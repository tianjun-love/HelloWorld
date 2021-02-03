# Creation by TianJun.
# 
# # # Revision

CXX = g++
CXXFLAGS = -DNDEBUG -O3 -Wall -std=c++11
INCLUDE = -I../include -I$(BASE_LIB_ROOT)
LIBS = -L./
LIB = 
TARGET = ../lib/libExample.r.a
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
	ar -cru $(TARGET) $(OBJS)
	ranlib $(TARGET)

include $(SRCS:.cpp=.r.d)

.PHONY:clean
clean:
	rm -f $(SRC_PATH)/*.r.o $(SRC_PATH)/*.r.d $(TARGET)
