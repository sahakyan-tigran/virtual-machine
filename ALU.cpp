#include "ALU.h"

#include <stdexcept>

std::int32_t ALU::func_or(std::int32_t arg1, std::int32_t arg2) {
	return arg1 | arg2;
}

std::int32_t ALU::func_nand(std::int32_t arg1, std::int32_t arg2) {
	return ~(arg1 & arg2);
}

std::int32_t ALU::func_nor(std::int32_t arg1, std::int32_t arg2) {
	return ~(arg1 | arg2);
}

std::int32_t ALU::func_and(std::int32_t arg1, std::int32_t arg2) {
	return arg1 & arg2;
}

std::int32_t ALU::func_add(std::int32_t arg1, std::int32_t arg2) {
	// Wrap through the unsigned type: signed overflow is undefined behaviour,
	// and a machine word is expected to wrap rather than trap.
	return static_cast<std::int32_t>(
		static_cast<std::uint32_t>(arg1) + static_cast<std::uint32_t>(arg2));
}

std::int32_t ALU::func_sub(std::int32_t arg1, std::int32_t arg2) {
	return static_cast<std::int32_t>(
		static_cast<std::uint32_t>(arg1) - static_cast<std::uint32_t>(arg2));
}

std::int32_t ALU::apply(int operation, std::int32_t arg1, std::int32_t arg2) {
	switch (operation) {
	case OR:   return func_or(arg1, arg2);
	case NAND: return func_nand(arg1, arg2);
	case NOR:  return func_nor(arg1, arg2);
	case AND:  return func_and(arg1, arg2);
	case ADD:  return func_add(arg1, arg2);
	case SUB:  return func_sub(arg1, arg2);
	default:
		throw std::invalid_argument("ALU: undefined operation");
	}
}
