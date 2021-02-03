# Creation by TianJun.
# 
# # # Revision

CXX = g++
CXXFLAGS = -ggdb -D_DEBUG -O1 -Wall -std=c++11
INCLUDE = -I./ -I../include -I$(BASE_LIB_ROOT)
LIBS = -L./
LIB = 
TARGET = ../lib/libExample.d.a
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
	ar -cru $(TARGET) $(OBJS)
	ranlib $(TARGET)

include $(SRCS:.cpp=.d.d)

.PHONY:clean
clean:
	rm -f $(SRC_PATH)/*.d.o $(SRC_PATH)/*.d.d $(TARGET)
