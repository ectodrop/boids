# Compiler and compiler flags
CC = g++
CFLAGS = -Wall -g
LIBS= -lm -fopenmp

# Source files (modify these lists as needed)
MAINSRCS = main_serial.cpp main_serial_grid.cpp main_omp.cpp
OTHERSRCS = $(filter-out $(MAINSRCS), $(wildcard *.cpp))

# Header files
HEADERS := $(wildcard *.h)

# Object files
MAINOBJS = $(MAINSRCS:.cpp=.o)
OTHEROBJS = $(OTHERSRCS:.cpp=.o)

# Executable names (derived from main source files)
EXECUTABLES = $(MAINSRCS:.cpp=)

# Default target
all: $(EXECUTABLES)

# Rule to build object files from other source files
$(OTHEROBJS): %.o : %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

# Rule to build object files from main source files
$(MAINOBJS): %.o : %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

# Rule to link executables
$(EXECUTABLES): % : %.o $(OTHEROBJS)
	$(CC) $(CFLAGS) -o $@ $< $(OTHEROBJS) $(LIBS)

# Clean up generated files
clean:
	rm -f $(EXECUTABLES) *.o
