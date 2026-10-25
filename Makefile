CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -fopenmp
LDFLAGS = -fopenmp

TARGET = smart_parking.exe
SRC_DIR = src
SRCS = $(SRC_DIR)/generator.cpp \
       $(SRC_DIR)/cost.cpp \
       $(SRC_DIR)/validator.cpp \
       $(SRC_DIR)/serial_alloc.cpp \
       $(SRC_DIR)/parallel_alloc.cpp \
       $(SRC_DIR)/main.cpp

OBJS = $(SRCS:.cpp=.o)
HELLO_TARGET = hello_openmp.exe

.PHONY: all clean test benchmark hello

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

hello: $(SRC_DIR)/hello_openmp.cpp
	$(CXX) $(CXXFLAGS) $< -o $(HELLO_TARGET) $(LDFLAGS)
	./$(HELLO_TARGET)

test: $(TARGET)
	./$(TARGET) --test

benchmark: $(TARGET)
	./$(TARGET) --benchmark --csv benchmark_results.csv

clean:
	-del /F /Q $(subst /,\,$(OBJS)) $(TARGET) $(HELLO_TARGET) benchmark_results.csv 2>NUL || rm -f $(OBJS) $(TARGET) $(HELLO_TARGET) benchmark_results.csv
