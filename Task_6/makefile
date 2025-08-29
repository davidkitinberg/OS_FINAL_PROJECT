CXX = g++
CXXFLAGS = -Wall -std=c++17
GCOVFLAGS = -fprofile-arcs -ftest-coverage -g

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


gcov:
	rm -f server client *.gcda *.gcno *.gcov
	g++ -Wall -std=c++17 -g -fprofile-arcs -ftest-coverage EulerServer.cpp Graph.cpp EulerChecker.cpp -o server
	g++ -Wall -std=c++17 -g -fprofile-arcs -ftest-coverage EulerClient.cpp -o client

coverage: server client
	@gcov -o . server-*.gcno  > EulerServer_coverage.txt
	@gcov -o . client-*.gcno  > EulerClient_coverage.txt


clean:
	rm -f server client *.o *.gcno *.gcda gmon.out callgrind.out.* gprof_report.txt

	# delete all *.gcov except the two reports
	find . -maxdepth 1 -name '*.gcov' ! -name 'EulerServer.cpp.gcov' ! -name 'EulerClient.cpp.gcov' -delete