CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I.
PYTHON   ?= python3

CORE   = Machine.cpp ALU.cpp Cond.cpp
VM     = virtualmachine
TESTS  = tests/test_vm

.PHONY: all test example clean

all: $(VM)

$(VM): VirtualMachine.cpp $(CORE) Machine.h ALU.h Cond.h
	$(CXX) $(CXXFLAGS) VirtualMachine.cpp $(CORE) -o $@

test: $(TESTS)
	./$(TESTS)

$(TESTS): tests/test_vm.cpp $(CORE) Machine.h ALU.h Cond.h
	$(CXX) $(CXXFLAGS) tests/test_vm.cpp $(CORE) -o $@

# Assemble and run the worked example; should print 55.
example: $(VM)
	$(PYTHON) tools/assemble.py examples/sum_to_ten.asm sum_to_ten.bin
	./$(VM) sum_to_ten.bin

clean:
	rm -f $(VM) $(VM).exe $(TESTS) $(TESTS).exe *.bin
