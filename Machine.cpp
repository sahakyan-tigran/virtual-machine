#include "Machine.h"

#include <fstream>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace {

	// Instruction layout, most significant bit first:
	//
	//   31     arg1 is an immediate
	//   30     arg2 is an immediate
	//   29-28  unused
	//   27     unit: 1 = ALU, 0 = Cond
	//   26-24  operation
	//   23-16  arg1: 8-bit immediate, or a 5-bit source id in the low bits
	//   15-8   arg2: likewise
	//   7-0    destination: a 5-bit id in the low bits
	const std::uint32_t ARG1_IMMEDIATE_BIT = 1u << 31;
	const std::uint32_t ARG2_IMMEDIATE_BIT = 1u << 30;
	const std::uint32_t ALU_BIT = 1u << 27;

	int operation_of(std::uint32_t instruction) {
		return static_cast<int>((instruction >> 24) & 0x7u);
	}
	int arg1_field(std::uint32_t instruction) {
		return static_cast<int>((instruction >> 16) & 0xFFu);
	}
	int arg2_field(std::uint32_t instruction) {
		return static_cast<int>((instruction >> 8) & 0xFFu);
	}
	int destination_field(std::uint32_t instruction) {
		return static_cast<int>(instruction & 0x1Fu);
	}
}

Machine::Machine(std::istream& in, std::ostream& out)
	: m_registers(REGISTER_COUNT, 0)
	, m_ram(RAM_SIZE, 0)
	, m_base(0)
	, m_counter(0)
	, m_in(in)
	, m_out(out)
{}

std::int32_t Machine::reg(int index) const {
	if (index < 0 || index >= REGISTER_COUNT) {
		throw std::out_of_range("Machine: register index out of range");
	}
	return m_registers[static_cast<std::size_t>(index)];
}

void Machine::set_reg(int index, std::int32_t value) {
	if (index < 0 || index >= REGISTER_COUNT) {
		throw std::out_of_range("Machine: register index out of range");
	}
	m_registers[static_cast<std::size_t>(index)] = value;
}

std::int32_t Machine::ram_at(int index) const {
	if (index < 0 || index >= RAM_SIZE) {
		throw std::out_of_range("Machine: RAM address out of range");
	}
	return m_ram[static_cast<std::size_t>(index)];
}

void Machine::set_ram(int index, std::int32_t value) {
	if (index < 0 || index >= RAM_SIZE) {
		throw std::out_of_range("Machine: RAM address out of range");
	}
	m_ram[static_cast<std::size_t>(index)] = value;
}

std::int32_t Machine::read_source(int source) {
	if (source >= LOC_R0 && source <= LOC_R15) {
		return m_registers[static_cast<std::size_t>(source - LOC_R0)];
	}
	switch (source) {
	case LOC_RAM:
		return ram_at(m_base);                 // was unchecked: a bad base
	case LOC_BASE:                             // read straight past the array
		return m_base;
	case LOC_COUNTER:
		return m_counter;
	case LOC_IN: {
		std::int32_t value = 0;
		if (!(m_in >> value)) {
			throw std::runtime_error("Machine: could not read from input");
		}
		return value;
	}
	default:
		throw std::invalid_argument("Machine: wrong source");
	}
}

void Machine::write_destination(std::int32_t value, int destination) {
	if (destination >= LOC_R0 && destination <= LOC_R15) {
		m_registers[static_cast<std::size_t>(destination - LOC_R0)] = value;
		return;
	}
	switch (destination) {
	case LOC_RAM:
		set_ram(m_base, value);
		break;
	case LOC_BASE:
		m_base = value;
		break;
	case LOC_COUNTER:
		// Writing the counter is how a program jumps. The old code had no
		// case for this at all, so the documented encoding could not be used
		// and the machine had no control flow.
		m_counter = value;
		break;
	case LOC_OUT:
		m_out << value << "\n";
		break;
	default:
		throw std::invalid_argument("Machine: wrong destination");
	}
}

void Machine::execute(std::uint32_t instruction) {
	int operation = operation_of(instruction);

	std::int32_t arg1 = (instruction & ARG1_IMMEDIATE_BIT)
		? static_cast<std::int32_t>(arg1_field(instruction))
		: read_source(arg1_field(instruction) & 0x1F);

	std::int32_t arg2 = (instruction & ARG2_IMMEDIATE_BIT)
		? static_cast<std::int32_t>(arg2_field(instruction))
		: read_source(arg2_field(instruction) & 0x1F);

	int destination = destination_field(instruction);

	std::int32_t result = (instruction & ALU_BIT)
		? ALU::apply(operation, arg1, arg2)
		: static_cast<std::int32_t>(Cond::apply(operation, arg1, arg2) ? 1 : 0);

	write_destination(result, destination);
}

long long Machine::run(const std::vector<std::uint32_t>& program, long long max_steps) {
	m_counter = 0;
	long long steps = 0;

	while (m_counter >= 0 &&
		static_cast<std::size_t>(m_counter) < program.size() &&
		steps < max_steps) {

		std::uint32_t instruction = program[static_cast<std::size_t>(m_counter)];

		// Advance first, so that an instruction writing to the counter
		// overrides it and the write behaves as a jump.
		++m_counter;
		execute(instruction);
		++steps;
	}
	return steps;
}

std::vector<std::uint32_t> Machine::read_program(const std::string& path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		throw std::runtime_error("Machine: cannot open " + path);
	}

	// Was: `new byte[size]` with size taken from an unchecked tellg(), so a
	// missing file gave size -1 and a raw buffer that leaked on any throw.
	std::vector<char> bytes((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	if (bytes.empty()) {
		throw std::runtime_error("Machine: " + path + " is empty");
	}
	if (bytes.size() % 4 != 0) {
		throw std::runtime_error("Machine: " + path +
			" is not a whole number of 4-byte instructions");
	}

	std::vector<std::uint32_t> program;
	program.reserve(bytes.size() / 4);
	for (std::size_t i = 0; i < bytes.size(); i += 4) {
		std::uint32_t word = 0;
		for (std::size_t j = 0; j < 4; ++j) {
			word = (word << 8) | static_cast<unsigned char>(bytes[i + j]);
		}
		program.push_back(word);
	}
	return program;
}
