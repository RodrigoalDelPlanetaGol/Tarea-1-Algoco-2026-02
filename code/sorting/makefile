CXX = g++

CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2

TARGET = sorting

SOURCES = \
	sorting.cpp \
	algorithms/mergesort.cpp \
	algorithms/quicksort.cpp \
	algorithms/patiencesort.cpp \
	algorithms/sort.cpp

HEADERS =

DATA_DIR = data
INPUT_DIR = $(DATA_DIR)/array_input
OUTPUT_DIR = $(DATA_DIR)/array_output
MEASUREMENTS_DIR = $(DATA_DIR)/measurements

.PHONY: all clean run prepare

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

prepare:
	mkdir -p $(INPUT_DIR)
	mkdir -p $(OUTPUT_DIR)
	mkdir -p $(MEASUREMENTS_DIR)

run: $(TARGET) prepare
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f *.o