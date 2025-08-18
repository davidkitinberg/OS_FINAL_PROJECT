CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra

.PHONY: all clean run-server run-client

all: server client

server:
	$(CXX) $(CXXFLAGS) -o server EulerServer.cpp Graph.cpp EulerChecker.cpp

client:
	$(CXX) $(CXXFLAGS) -o client EulerClient.cpp

run-server: server
	./server

run-client: client
	./client -v 5 -e 6

clean:
	rm -f server client *.o *.gcno *.gcda *.gcov gmon.out callgrind.out.* gprof_report.txt
