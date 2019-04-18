CXX = g++
CXXFLAGS += -O2 -Wall -std=c++11
EXEC = TCPScanner
OBJS = TCPScanner.o

$(EXEC): $(OBJS)
	$(CXX) -o $(EXEC) $(OBJS)  -lpthread

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJS) $(EXEC)