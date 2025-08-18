# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra

# Unified compile command macro (as requested)
COMPILE := $(CXX) $(CXXFLAGS)

# Files
SRC := main.cpp Graph.cpp EulerChecker.cpp
OBJ := $(SRC:.cpp=.o)

# Flags for code coverage
COVERAGE_FLAGS := -fprofile-arcs -ftest-coverage
GCOV_FILES := *.gcno *.gcda *.gcov

# Valgrind tools
MEMCHECK := valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
CALLGRIND := valgrind --tool=callgrind

.PHONY: all run clean coverage gprof valgrind callgrind

# ---------- Build ----------
all: main

main: $(OBJ)
	$(COMPILE) -o $@ $^

%.o: %.cpp
	$(COMPILE) -c $< -o $@

# ---------- Run ----------
run: main
	./main -v 5 -e 6 -s 42

# ---------- Coverage ----------
coverage: clean
	$(COMPILE) $(COVERAGE_FLAGS) -c main.cpp -o main.o
	$(COMPILE) $(COVERAGE_FLAGS) -c Graph.cpp -o Graph.o
	$(COMPILE) $(COVERAGE_FLAGS) -c EulerChecker.cpp -o EulerChecker.o
	$(COMPILE) $(COVERAGE_FLAGS) -o main main.o Graph.o EulerChecker.o
	./main -v 5 -e 6 -s 42
	gcov main.cpp Graph.cpp EulerChecker.cpp

# ---------- GProf ----------
gprof: clean
	$(COMPILE) -pg -c main.cpp -o main.o
	$(COMPILE) -pg -c Graph.cpp -o Graph.o
	$(COMPILE) -pg -c EulerChecker.cpp -o EulerChecker.o
	$(COMPILE) -pg -o main main.o Graph.o EulerChecker.o
	./main -v 10000 -e 4500000 -s 42
	gprof ./main gmon.out > gprof_report.txt
	

# ---------- Valgrind ----------
valgrind: main
	$(MEMCHECK) ./main -v 5 -e 6 -s 42

callgrind: main
	$(CALLGRIND) ./main -v 5 -e 6 -s 42
	

# ---------- Clean ----------
clean:
	rm -f $(OBJ) main $(GCOV_FILES) gmon.out callgrind.out.* gprof_report.txt
