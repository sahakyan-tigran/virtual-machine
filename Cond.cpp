#include "Cond.h"

#include <stdexcept>

bool Cond::never(std::int32_t, std::int32_t) { return false; }
bool Cond::equal(std::int32_t arg1, std::int32_t arg2) { return arg1 == arg2; }
bool Cond::less(std::int32_t arg1, std::int32_t arg2) { return arg1 < arg2; }
bool Cond::lessOrEqual(std::int32_t arg1, std::int32_t arg2) { return arg1 <= arg2; }
bool Cond::always(std::int32_t, std::int32_t) { return true; }
bool Cond::notEqual(std::int32_t arg1, std::int32_t arg2) { return arg1 != arg2; }
bool Cond::bigOrEqual(std::int32_t arg1, std::int32_t arg2) { return arg1 >= arg2; }
bool Cond::big(std::int32_t arg1, std::int32_t arg2) { return arg1 > arg2; }

bool Cond::apply(int operation, std::int32_t arg1, std::int32_t arg2) {
	switch (operation) {
	case NEVER:         return never(arg1, arg2);
	case EQUAL:         return equal(arg1, arg2);
	case LESS:          return less(arg1, arg2);
	case LESS_OR_EQUAL: return lessOrEqual(arg1, arg2);
	case ALWAYS:        return always(arg1, arg2);
	case NOT_EQUAL:     return notEqual(arg1, arg2);
	case BIG_OR_EQUAL:  return bigOrEqual(arg1, arg2);
	case BIG:           return big(arg1, arg2);
	default:
		throw std::invalid_argument("Cond: undefined operation");
	}
}
