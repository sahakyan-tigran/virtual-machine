#pragma once

#include <cstdint>

// Condition unit. Each operation yields 1 or 0, which the machine writes to a
// destination like any other result -- writing it to the counter is how a
// program branches.
class Cond {
public:
	enum Op {
		NEVER = 0,
		EQUAL = 1,
		LESS = 2,
		LESS_OR_EQUAL = 3,
		ALWAYS = 4,
		NOT_EQUAL = 5,
		BIG_OR_EQUAL = 6,
		BIG = 7
	};

	static bool never(std::int32_t arg1, std::int32_t arg2);
	static bool equal(std::int32_t arg1, std::int32_t arg2);
	static bool less(std::int32_t arg1, std::int32_t arg2);
	static bool lessOrEqual(std::int32_t arg1, std::int32_t arg2);
	static bool always(std::int32_t arg1, std::int32_t arg2);
	static bool notEqual(std::int32_t arg1, std::int32_t arg2);
	static bool bigOrEqual(std::int32_t arg1, std::int32_t arg2);
	static bool big(std::int32_t arg1, std::int32_t arg2);

	// Dispatch by opcode. Throws std::invalid_argument on an unknown code.
	static bool apply(int operation, std::int32_t arg1, std::int32_t arg2);
};
