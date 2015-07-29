CPPS=main.cpp wavefile.cpp cable.cpp
OBJS=$(CPPS:.cpp=.o)
LIB=-lboost_thread -lboost_system -lportaudio
FLAGS=-g -O0 -std=c++11

cables: $(OBJS)
	g++ -o $@ $(OBJS) $(LIB)

.cpp.o:
	g++ $(FLAGS) -c $< -o $(<:.cpp=.o)

clean:
	@rm $(OBJS)

all:
	make clean && make