CXX=g++
CPPS=main.cpp units.cpp wavefile.cpp socket.cpp helper.cpp portaudio.cpp
OBJS=$(CPPS:.cpp=.o)
LIB=-lboost_thread -lboost_system -lportaudio -lncurses
FLAGS=-g -O0 -std=c++11

cables: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIB)

.cpp.o:
	$(CXX) $(FLAGS) -c $< -o $(<:.cpp=.o)

clean: 
	@rm -f $(OBJS)

all: clean cables
