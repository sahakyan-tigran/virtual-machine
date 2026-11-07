#pragma once

#include "ALU.h"
#include "Cond.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

// A small register machine.
//
// State used to live in file-scope globals, which made the machine impossible
// to instantiate twice or to test. It is held here instead, and the input and
// output streams are injected so a test can drive a program without touching
// std::cin.
class Machine {
public:
	static const int REGISTER_COUNT = 16;
	static const int RAM_SIZE = 1024;

	// Source and destination identifiers, as documented in the README.
	enum Location {
		LOC_RAM = 0,          // ram[base register]
		LOC_BASE = 1,
		LOC_R0 = 2,           // r0 .. r15 occupy 2 .. 17
		LOC_R15 = 17,
		LOC_COUNTER = 18,
		LOC_IN = 19,          // source only
		LOC_OUT = 20          // destination only
	};

	Machine(std::istream& in, std::ostream& out);

	// Runs until the counter leaves the program, or until max_steps
	// instructions have run (which stops a runaway loop from hanging).
	// Returns the number of instructions executed.
	long long run(const std::vector<std::uint32_t>& program,
		long long max_steps = 10000000LL);

	std::int32_t reg(int index) const;
	std::int32_t ram_at(int index) const;
	std::int32_t base() const { return m_base; }
	std::int32_t counter() const { return m_counter; }

	void set_reg(int index, std::int32_t value);
	void set_ram(int index, std::int32_t value);
	void set_base(std::int32_t value) { m_base = value; }

	// Reads a big-endian stream of 32-bit instructions.
	static std::vector<std::uint32_t> read_program(const std::string& path);

private:
	void execute(std::uint32_t instruction);
	std::int32_t read_source(int source);
	void write_destination(std::int32_t value, int destination);

private:
	std::vector<std::int32_t> m_registers;
	std::vector<std::int32_t> m_ram;
	std::int32_t m_base;
	std::int32_t m_counter;

	std::istream& m_in;
	std::ostream& m_out;
};
