CXX = g++

CXXFLAGS = -Wall -g
LDFLAGS = 

INCLUDES = -I.
LDLIBS =

.PHONY : all
all : sim sim-naive

sim : sim.o bin.o common.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sim.o : serial.cpp bin.h common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<



sim-naive : sim-naive.o common.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)


sim-naive.o : serial-naive.cpp common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<


bin.o : bin.cpp bin.h common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

common.o : common.cpp common.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<


