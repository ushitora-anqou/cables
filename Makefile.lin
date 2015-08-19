TARGET=cables
CXX=g++
CPPS=glutview.cpp main.cpp units.cpp wavefile.cpp socket.cpp portaudio.cpp asio_network.cpp
OBJS=$(CPPS:.cpp=.o)
DEPS=$(CPPS:.cpp=.d)
LIB=-lboost_thread -lboost_system -lboost_regex -lboost_serialization -lportaudio -lncurses -lglut -lGLU -lGL -lm
FLAGS=-g -O0 -std=c++11

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIB)

%.o: %.cpp
	$(CXX) $(FLAGS) -c -MMD -MP $<

clean: 
	@rm -f $(OBJS) $(DEPS) $(TARGET)

all: clean $(TARGET)

-include $(DEPS)
