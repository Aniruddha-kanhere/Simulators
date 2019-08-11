CC = g++
OPT = -O3
#OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List of all .cc/.cpp files
SIM_SRC = sim_cache.cpp

# List of all corresponding compiled object files here (.o files)
SIM_OBJ = sim_cache.o
 
#################################
# default rule
all: cache
	@echo "my work is done here..."

# rule for making sim_cache
cache: $(SIM_OBJ)
	$(CC) -o sim_cache $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH sim_cache-----------"


# generic rule for converting any .cpp file to any .o file
.cc.o:
	$(CC) $(CFLAGS)  -c $<

.cpp.o:
	$(CC) $(CFLAGS)  -c $<


# type "make clean" to remove all .o files plus the sim_cache binary
clean:
	rm -f *.o sim_cache


# type "make clobber" to remove all .o files (leaves sim_cache binary)
clobber:
	rm -f *.o
