CXX := g++
GCOVFLAGS   := -fprofile-arcs -ftest-coverage -g -O0
CXXFLAGS := -std=c++17 -Wall -Wextra

SERVER_SRCS := \
	Server.cpp \
	AlgorithmFactory.cpp \
	EulerAlgorithm.cpp \
	MSTAlgorithm.cpp \
	SCCAlgorithm.cpp \
	MaxFlowAlgorithm.cpp \
	HamiltonianAlgorithm.cpp \
	Graph.cpp

ALG_SRCS := \
	AlgorithmFactory.cpp \
	EulerAlgorithm.cpp \
	MSTAlgorithm.cpp \
	SCCAlgorithm.cpp \
	MaxFlowAlgorithm.cpp \
	HamiltonianAlgorithm.cpp \
	Graph.cpp

.PHONY: all clean distclean run-server run-client gcov coverage run


all: server client

# ---- Server ----
server:
	$(CXX) $(CXXFLAGS) -o server $(SERVER_SRCS)
# ---- Client ----
client:
	$(CXX) $(CXXFLAGS) -o client Client.cpp

# ---- Convenience ----
run-server: server
	./server

run-client: client
	./client -v 5 -e 6

gcov:
	# fresh start for coverage artifacts
	rm -f server client *.gcda *.gcno *.gcov
	$(CXX) $(CXXFLAGS) $(GCOVFLAGS) -o server $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) $(GCOVFLAGS) -o client Client.cpp

coverage: server client
	gcov -b -c server-*.gcno > Server_coverage.txt
	gcov -b -c client-*.gcno > Client_coverage.txt
	gcov -b -c $(ALG_SRCS) > Algorithms_coverage.txt

clean:
	rm -f server client *.o *.gcno *.gcda gmon.out callgrind.out.* gprof_report.txt

	# delete all *.gcov except the wanted reports
	find . -maxdepth 1 -name '*.gcov' ! -name 'Server.cpp.gcov' ! -name 'Client.cpp.gcov' ! -name 'Graph.cpp.gcov' ! -name 'AlgorithmFactory.cpp.gcov' ! -name 'MaxFlowAlgorithm.cpp.gcov' ! -name 'HamiltonianAlgorithm.cpp.gcov' ! -name 'MSTAlgorithm.cpp.gcov' ! -name 'SCCAlgorithm.cpp.gcov' -delete
