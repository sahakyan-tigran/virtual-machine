#pragma once

#include <cstdint>

// Arithmetic and logic unit.
//
// Operands and results are 32-bit, matching the machine's registers. These
// used to be uint8_t while every caller passed an int, so every result was
// silently truncated to 8 bits and the machine could not hold a value above
// 255.
class ALU {
public:
	enum Op {
		OR = 0,
		NAND = 1,
		NOR = 2,
		AND = 3,
		ADD = 4,
		SUB = 5
	};

	static std::int32_t func_or(std::int32_t arg1, std::int32_t arg2);
	static std::int32_t func_nand(std::int32_t arg1, std::int32_t arg2);
	static std::int32_t func_nor(std::int32_t arg1, std::int32_t arg2);
	static std::int32_t func_and(std::int32_t arg1, std::int32_t arg2);
	static std::int32_t func_add(std::int32_t arg1, std::int32_t arg2);
	static std::int32_t func_sub(std::int32_t arg1, std::int32_t arg2);

	// Dispatch by opcode. Throws std::invalid_argument on an unknown code.
	static std::int32_t apply(int operation, std::int32_t arg1, std::int32_t arg2);
};
