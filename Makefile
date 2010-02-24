CC = gcc
CXX = g++


CFLAGS = -Wall -g
CXXFLAGS = -Wall -g
LDFLAGS = 

INCLUDES = -I.
LDLIBS = -lpthread

.PHONY : all
all : sim sim_shared sim-naive

sim : sim.o bin.o common.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sim.o : serial.cpp bin.h common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

sim_shared : sim_shared.o bin_shared.o common.o 
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sim_shared.o : shared.cpp bin_shared.h common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin_shared.o : bin_shared.cpp bin_shared.h common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

#pthread_barrier.o : pthread_barrier.c pthread_barrier.h
#	$(CC) $(CFLAGS) -c -o $@ $<

sim-naive : sim-naive.o common.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)


sim-naive.o : serial-naive.cpp common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<


bin.o : bin.cpp bin.h common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

common.o : common.cpp common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<


