CXX = g++
CXXFLAGS = -O3 -march=native -mavx2 -pthread -std=c++17
TARGET = mandelbrot
SOURCES = mandelbrot.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) *.ppm *.csv *.png

run: $(TARGET)
	./$(TARGET) 800 800 1000

.PHONY: all clean run