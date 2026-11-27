# Compiler and compiler flags
CC      = mpicxx
CFLAGS  = -Wall -g -O3 -Isrc
LIBS    = -lm -fopenmp

SRC_DIR   = src
BUILD_DIR = build
BIN_DIR   = bin

# Each of these sources contains its own main() and becomes one executable
MAINS     = main_serial main_serial_grid main_omp main_omp_grid main_omp_mpi_grid
MAINSRCS  = $(addprefix $(SRC_DIR)/,$(addsuffix .cpp,$(MAINS)))
OTHERSRCS = $(filter-out $(MAINSRCS), $(wildcard $(SRC_DIR)/*.cpp))
HEADERS   = $(wildcard $(SRC_DIR)/*.h)

OTHEROBJS   = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(OTHERSRCS))
EXECUTABLES = $(addprefix $(BIN_DIR)/,$(MAINS))

.PHONY: all clean
all: $(EXECUTABLES)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

$(BIN_DIR)/%: $(BUILD_DIR)/%.o $(OTHEROBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(OTHEROBJS) $(LIBS)

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

.PRECIOUS: $(BUILD_DIR)/%.o

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) *.bin
