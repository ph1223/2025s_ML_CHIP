HOME = /home/2024_summer/2024train_10
LIB_DIR=$(HOME)/systemc-3.0.2/lib-linux64
INC_DIR=$(HOME)/systemc-3.0.2/include
LIB=-lsystemc-3.0.2
RPATH=-Wl,-rpath,$(HOME)/systemc-3.0.2/lib-linux64

O = run
C = *.cpp

# Default target
all: $(O)

# Compile and link
$(O): $(C)
	g++ -I . -I  $(INC_DIR) -L . -L $(LIB_DIR) -o $(O) $(C) $(LIB) $(RPATH) -g

# Run with cat.txt
cat: $(O)
	./run cat.txt

# Run with dog.txt
dog: $(O)
	./run dog.txt

# Clean up compiled files
clean:
	rm -rf $(O)