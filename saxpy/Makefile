CXX = g++
CXXFLAGS = -O3 -march=native -mavx2 -mfma -pthread -std=c++17
TARGET = saxpy_experiment
SOURCES = saxpy_experiment.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) *.csv

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run