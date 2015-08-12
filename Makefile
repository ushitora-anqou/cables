CXX=g++
CPPS=glutview.cpp helper.cpp main.cpp units.cpp wavefile.cpp socket.cpp portaudio.cpp
OBJS=$(CPPS:.cpp=.o)
LIB=-lboost_thread -lboost_system -lportaudio -lncurses -lglut -lGLU -lGL -lm
FLAGS=-g -O0 -std=c++11

cables: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIB)

.cpp.o:
	$(CXX) $(FLAGS) -c $< -o $(<:.cpp=.o)

clean: 
	@rm -f $(OBJS)

all: clean cables
