CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra

.PHONY: all clean run-server run-client

all: server client

# ---- Server ----
server:
	$(CXX) $(CXXFLAGS) -o server \
		Server.cpp \
		AlgorithmFactory.cpp \
		EulerAlgorithm.cpp \
		MSTAlgorithm.cpp \
		SCCAlgorithm.cpp \
		MaxFlowAlgorithm.cpp \
		HamiltonianAlgorithm.cpp \
		Graph.cpp

# ---- Client ----
client:
	$(CXX) $(CXXFLAGS) -o client Client.cpp

# ---- Convenience ----
run-server: server
	./server

run-client: client
	./client -v 5 -e 6

clean:
	rm -f server client *.o *.gcno *.gcda *.gcov gmon.out callgrind.out.* gprof_report.txt
