CXX = g++
CXXFLAGS = -O3 -march=native -mavx2 -mfma -pthread -std=c++17
TARGET = sqrt_benchmark
SOURCES = sqrt_benchmark.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) *.csv

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run