// Entry point: load a binary program and run it.
//
//   virtualmachine <program.bin>
//
// The previous version prompted for a path, read it, and then overwrote the
// answer with a hardcoded one, so it only ever ran a file from one particular
// machine.

#include "Machine.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
	std::string path;

	if (argc > 1) {
		path = argv[1];
	}
	else {
		std::cout << "Enter the path of your binary file: ";
		if (!std::getline(std::cin, path) || path.empty()) {
			std::cerr << "No input file given.\n";
			return 2;
		}
	}

	try {
		std::vector<std::uint32_t> program = Machine::read_program(path);
		Machine machine(std::cin, std::cout);
		machine.run(program);
		return 0;
	}
	catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}
}
