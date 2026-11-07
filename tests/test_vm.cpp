// Test suite for the virtual machine.
//
//   g++ -std=c++17 -Wall -Wextra -I.. tests/test_vm.cpp ../Machine.cpp ../ALU.cpp ../Cond.cpp -o tests/test_vm
//   ./tests/test_vm
//
// Programs are assembled inline by encode(), so the tests do not depend on the
// Python assembler.

#include "../ALU.h"
#include "../Cond.h"
#include "../Machine.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& what) {
	++g_checks;
	if (!condition) {
		++g_failures;
		std::cout << "  FAIL: " << what << "\n";
	}
}

template <typename T>
void check_eq(const T& actual, const T& expected, const std::string& what) {
	++g_checks;
	if (!(actual == expected)) {
		++g_failures;
		std::cout << "  FAIL: " << what << " (got " << actual
			<< ", expected " << expected << ")\n";
	}
}

template <typename Fn>
bool throws(Fn fn) {
	try { fn(); }
	catch (const std::exception&) { return true; }
	return false;
}

// ---- inline assembler ------------------------------------------------------

struct Arg {
	int value;
	bool immediate;
};

Arg imm(int v) { return Arg{ v, true }; }
Arg loc(int v) { return Arg{ v, false }; }
Arg r(int i) { return Arg{ Machine::LOC_R0 + i, false }; }

std::uint32_t encode(bool is_alu, int op, Arg a1, Arg a2, int dest) {
	std::uint32_t word = static_cast<std::uint32_t>(op & 0x7) << 24;
	if (is_alu) { word |= 1u << 27; }
	if (a1.immediate) { word |= 1u << 31; }
	if (a2.immediate) { word |= 1u << 30; }
	word |= static_cast<std::uint32_t>(a1.value & 0xFF) << 16;
	word |= static_cast<std::uint32_t>(a2.value & 0xFF) << 8;
	word |= static_cast<std::uint32_t>(dest & 0x1F);
	return word;
}

std::uint32_t alu(int op, Arg a1, Arg a2, int dest) { return encode(true, op, a1, a2, dest); }
std::uint32_t cnd(int op, Arg a1, Arg a2, int dest) { return encode(false, op, a1, a2, dest); }

// ---- tests -----------------------------------------------------------------

void test_alu_unit() {
	std::cout << "ALU\n";

	check_eq(ALU::func_or(0xF0, 0x0F), 0xFF, "or");
	check_eq(ALU::func_and(0xFF, 0x0F), 0x0F, "and");
	check_eq(ALU::func_nand(0xFF, 0x0F), ~0x0F, "nand");
	check_eq(ALU::func_nor(0xF0, 0x0F), ~0xFF, "nor");
	check_eq(ALU::func_add(2, 3), 5, "add");
	check_eq(ALU::func_sub(3, 5), -2, "sub goes negative");

	// The ALU used to take and return uint8_t, so anything above 255 was
	// silently truncated.
	check_eq(ALU::func_add(200, 100), 300, "add past 255");
	check_eq(ALU::func_add(100000, 1), 100001, "add past 16 bits");

	check(throws([] { ALU::apply(99, 1, 2); }), "unknown ALU op throws");
}

void test_cond_unit() {
	std::cout << "Cond\n";

	check(!Cond::apply(Cond::NEVER, 1, 1), "never");
	check(Cond::apply(Cond::EQUAL, 5, 5), "equal");
	check(!Cond::apply(Cond::EQUAL, 5, 6), "not equal");
	check(Cond::apply(Cond::LESS, 4, 5), "less");
	check(Cond::apply(Cond::LESS_OR_EQUAL, 5, 5), "less or equal");
	check(Cond::apply(Cond::ALWAYS, 0, 0), "always");
	check(Cond::apply(Cond::NOT_EQUAL, 4, 5), "not equal op");
	check(Cond::apply(Cond::BIG_OR_EQUAL, 5, 5), "bigger or equal");
	check(Cond::apply(Cond::BIG, 6, 5), "bigger");

	check(throws([] { Cond::apply(99, 1, 2); }), "unknown Cond op throws");
}

void test_basic_execution() {
	std::cout << "Execution\n";

	std::istringstream in;
	std::ostringstream out;
	Machine m(in, out);

	// r0 = 2 + 3; out = r0
	std::vector<std::uint32_t> program = {
		alu(ALU::ADD, imm(2), imm(3), Machine::LOC_R0),
		alu(ALU::ADD, r(0), imm(0), Machine::LOC_OUT),
	};
	check_eq(m.run(program), 2LL, "two instructions executed");
	check_eq(m.reg(0), 5, "r0 holds the sum");
	check_eq(out.str(), std::string("5\n"), "out received the value");

	// Registers, base register and RAM.
	std::ostringstream out2;
	Machine m2(in, out2);
	std::vector<std::uint32_t> program2 = {
		alu(ALU::ADD, imm(7), imm(0), Machine::LOC_BASE),   // base = 7
		alu(ALU::ADD, imm(99), imm(0), Machine::LOC_RAM),   // ram[7] = 99
		alu(ALU::ADD, loc(Machine::LOC_RAM), imm(1), r(1).value),  // r1 = ram[7] + 1
	};
	m2.run(program2);
	check_eq(m2.base(), 7, "base register written");
	check_eq(m2.ram_at(7), 99, "RAM written through the base register");
	check_eq(m2.reg(1), 100, "RAM read through the base register");

	// A condition writes 1 or 0.
	std::ostringstream out3;
	Machine m3(in, out3);
	std::vector<std::uint32_t> program3 = {
		cnd(Cond::LESS, imm(3), imm(9), r(2).value),
		cnd(Cond::LESS, imm(9), imm(3), r(3).value),
	};
	m3.run(program3);
	check_eq(m3.reg(2), 1, "true condition writes 1");
	check_eq(m3.reg(3), 0, "false condition writes 0");
}

void test_input_output() {
	std::cout << "Input and output\n";

	std::istringstream in("11 31");
	std::ostringstream out;
	Machine m(in, out);

	std::vector<std::uint32_t> program = {
		alu(ALU::ADD, loc(Machine::LOC_IN), loc(Machine::LOC_IN), Machine::LOC_OUT),
	};
	m.run(program);
	check_eq(out.str(), std::string("42\n"), "reads two values from input and prints the sum");

	// Reading past the end of the input is an error, not a silent zero.
	std::istringstream empty;
	std::ostringstream out2;
	Machine m2(empty, out2);
	check(throws([&] { m2.run(program); }), "exhausted input throws");
}

void test_jumps() {
	std::cout << "Control flow\n";

	std::istringstream in;
	std::ostringstream out;
	Machine m(in, out);

	// Writing the counter jumps. Instruction 1 is skipped.
	std::vector<std::uint32_t> program = {
		alu(ALU::ADD, imm(2), imm(0), Machine::LOC_COUNTER),  // jump to 2
		alu(ALU::ADD, imm(1), imm(0), r(0).value),            // skipped
		alu(ALU::ADD, imm(5), imm(0), r(1).value),
	};
	m.run(program);
	check_eq(m.reg(0), 0, "jumped over the middle instruction");
	check_eq(m.reg(1), 5, "landed on the target");

	// The counter is readable as a source.
	std::ostringstream out2;
	Machine m2(in, out2);
	std::vector<std::uint32_t> program2 = {
		alu(ALU::ADD, imm(0), imm(0), r(0).value),
		alu(ALU::ADD, loc(Machine::LOC_COUNTER), imm(0), r(1).value),
	};
	m2.run(program2);
	check_eq(m2.reg(1), 2, "counter reads as the already-advanced value");

	// A runaway loop is cut off rather than hanging.
	std::ostringstream out3;
	Machine m3(in, out3);
	std::vector<std::uint32_t> forever = {
		alu(ALU::ADD, imm(0), imm(0), Machine::LOC_COUNTER),
	};
	check_eq(m3.run(forever, 1000), 1000LL, "infinite loop stops at max_steps");
}

// The example program from examples/sum_to_ten.asm, assembled by hand.
void test_sum_to_ten() {
	std::cout << "Example: sum 1..10\n";

	const int LOOP = 2;
	const int DONE = 10;

	std::istringstream in;
	std::ostringstream out;
	Machine m(in, out);

	std::vector<std::uint32_t> program = {
		/* 0 */ alu(ALU::ADD, imm(0), imm(0), r(0).value),   // sum = 0
		/* 1 */ alu(ALU::ADD, imm(1), imm(0), r(1).value),   // i = 1
		/* 2 */ alu(ALU::ADD, r(0), r(1), r(0).value),       // sum += i
		/* 3 */ alu(ALU::ADD, r(1), imm(1), r(1).value),     // i += 1
		/* 4 */ cnd(Cond::LESS_OR_EQUAL, r(1), imm(10), r(2).value),
		/* 5 */ alu(ALU::SUB, imm(0), r(2), r(3).value),     // -cond
		/* 6 */ alu(ALU::SUB, r(2), imm(1), r(4).value),     // cond - 1
		/* 7 */ alu(ALU::AND, r(3), imm(LOOP), r(3).value),
		/* 8 */ alu(ALU::AND, r(4), imm(DONE), r(4).value),
		/* 9 */ alu(ALU::OR, r(3), r(4), Machine::LOC_COUNTER),
		/*10 */ alu(ALU::ADD, r(0), imm(0), Machine::LOC_OUT),
	};

	m.run(program);
	check_eq(m.reg(0), 55, "sum of 1..10");
	check_eq(out.str(), std::string("55\n"), "printed the sum");
}

void test_errors() {
	std::cout << "Error handling\n";

	std::istringstream in;
	std::ostringstream out;
	Machine m(in, out);

	// Source 21 and above are not defined.
	std::vector<std::uint32_t> bad_source = { alu(ALU::ADD, loc(25), imm(0), r(0).value) };
	check(throws([&] { m.run(bad_source); }), "unknown source throws");

	// `out` cannot be read.
	std::vector<std::uint32_t> read_out = {
		alu(ALU::ADD, loc(Machine::LOC_OUT), imm(0), r(0).value) };
	check(throws([&] { m.run(read_out); }), "reading out throws");

	// `in` cannot be written.
	std::vector<std::uint32_t> write_in = {
		alu(ALU::ADD, imm(1), imm(0), Machine::LOC_IN) };
	check(throws([&] { m.run(write_in); }), "writing in throws");

	// A base register outside RAM must be reported, not read past the array.
	std::vector<std::uint32_t> bad_base = {
		alu(ALU::ADD, imm(200), imm(0), Machine::LOC_BASE),
		alu(ALU::ADD, imm(200), imm(0), Machine::LOC_BASE),
		alu(ALU::ADD, loc(Machine::LOC_RAM), imm(0), r(0).value),
	};
	std::ostringstream out2;
	Machine m2(in, out2);
	// base ends at 200, which is in range; push it out of range instead.
	std::vector<std::uint32_t> way_out = {
		alu(ALU::NAND, imm(0), imm(0), Machine::LOC_BASE),  // base = -1
		alu(ALU::ADD, loc(Machine::LOC_RAM), imm(0), r(0).value),
	};
	check(throws([&] { m2.run(way_out); }), "out-of-range base throws");
	(void)bad_base;

	check(throws([] { Machine::read_program("no_such_file_here.bin"); }),
		"missing program file throws");
}

}  // namespace

int main() {
	test_alu_unit();
	test_cond_unit();
	test_basic_execution();
	test_input_output();
	test_jumps();
	test_sum_to_ten();
	test_errors();

	std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
	if (g_failures != 0) {
		std::cout << g_failures << " FAILED\n";
		return 1;
	}
	std::cout << "All tests passed.\n";
	return 0;
}
