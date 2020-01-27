#include "libencoding/encoding.h"
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

template <typename Instruction>
struct Executor;

class VM;

template <typename Instruction>
struct Executor {
	void operator()(VM &vm, Instruction &instruction);
};

namespace _VM {
regular::InstructionTypes step(VM &vm);
};

template <size_t... Registers>
auto MakeVMRegisters(std::index_sequence<Registers...>) {
	return std::unordered_map<regular::Register, uint32_t>{
	    std::make_pair(static_cast<regular::Register>(Registers), 0)...,
	};
}

class VM {
	friend auto step(VM &vm);

  public:
	std::unordered_map<regular::Register, uint32_t> registers =
	    MakeVMRegisters(std::make_index_sequence<static_cast<std::underlying_type_t<regular::Register>>(regular::Register::_count)>());
	unsigned char memory[640 * 1000 /*  640K ought to be enough for anyone */] = {};

	auto step() {
		return _VM::step(*this);
	}
};

#define EXECUTOR(i, implementation)                                       \
	template <>                                                           \
	struct Executor<i> {                                                  \
		void operator()(/* [[maybe_usused ]] */ VM &vm, i &instruction) { \
			/* [[maybe_unused]] */ auto operands = instruction.operands;  \
			implementation                                                \
		}                                                                 \
	};

EXECUTOR(regular::NopInstruction, )
EXECUTOR(regular::AddInstruction, vm.registers[operands.rA] = vm.registers[operands.rB] + vm.registers[operands.rC];)
EXECUTOR(regular::SubInstruction, vm.registers[operands.rA] = vm.registers[operands.rB] - vm.registers[operands.rC];)
EXECUTOR(regular::AndInstruction, vm.registers[operands.rA] = vm.registers[operands.rB] & vm.registers[operands.rC];)
EXECUTOR(regular::OrrInstruction, vm.registers[operands.rA] = vm.registers[operands.rB] | vm.registers[operands.rC];)
EXECUTOR(regular::XorInstruction, vm.registers[operands.rA] = vm.registers[operands.rB] ^ vm.registers[operands.rC];)
EXECUTOR(regular::NotInstruction, vm.registers[operands.rA] = ~vm.registers[operands.rB];)
EXECUTOR(
    regular::LshInstruction,
    auto shift = static_cast<int32_t>(vm.registers[operands.rC]);
    if (shift >= 0) {
	    vm.registers[operands.rA] = vm.registers[operands.rB] << shift;
    } else {
	    vm.registers[operands.rA] = vm.registers[operands.rB] >> -shift;
    })
EXECUTOR(
    regular::AshInstruction,
    auto shift = static_cast<int32_t>(vm.registers[operands.rC]);
    if (shift >= 0) {
	    vm.registers[operands.rA] = static_cast<int32_t>(vm.registers[operands.rB]) << shift;
    } else {
	    vm.registers[operands.rA] = static_cast<int32_t>(vm.registers[operands.rB]) >> -shift;
    })
EXECUTOR(
    regular::TcuInstruction,
    auto sign = 0;
    if (vm.registers[operands.rB] > vm.registers[operands.rC]) {
	    sign = 1;
    } else if (vm.registers[operands.rB] < vm.registers[operands.rC]) {
	    sign = -1;
    } vm.registers[operands.rA] = sign;)
EXECUTOR(
    regular::TcsInstruction,
    auto sign = 0;
    if (static_cast<int32_t>(vm.registers[operands.rB]) > static_cast<int32_t>(vm.registers[operands.rC])) {
	    sign = 1;
    } else if (static_cast<int32_t>(vm.registers[operands.rB]) < static_cast<int32_t>(vm.registers[operands.rC])) {
	    sign = -1;
    } vm.registers[operands.rA] = sign;)
EXECUTOR(regular::SetInstruction, vm.registers[operands.rA] = operands.imm;)
EXECUTOR(regular::MovInstruction, vm.registers[operands.rA] = vm.registers[operands.rB];)
EXECUTOR(regular::LdwInstruction,
         vm.registers[operands.rA] =
             vm.memory[vm.registers[operands.rB] + 0] << 0 |
             vm.memory[vm.registers[operands.rB] + 1] << 8 |
             vm.memory[vm.registers[operands.rB] + 2] << 16 |
             vm.memory[vm.registers[operands.rB] + 3] << 24;)
EXECUTOR(regular::StwInstruction,
         vm.memory[vm.registers[operands.rA] + 0] = vm.registers[operands.rB] >> 0 & 0xff;
         vm.memory[vm.registers[operands.rA] + 1] = vm.registers[operands.rB] >> 8 & 0xff;
         vm.memory[vm.registers[operands.rA] + 2] = vm.registers[operands.rB] >> 16 & 0xff;
         vm.memory[vm.registers[operands.rA] + 3] = vm.registers[operands.rB] >> 24 & 0xff;)
EXECUTOR(regular::LdbInstruction, vm.registers[operands.rA] &= vm.memory[vm.registers[operands.rB]] & 0xff;)
EXECUTOR(regular::StbInstruction, vm.memory[vm.registers[operands.rA]] = vm.registers[operands.rB] & 0xff;)

namespace _VM {
regular::InstructionTypes step(VM &vm) {
	uint32_t encoding = vm.memory[vm.registers[regular::Register::PC] + 0] << 0 |
	                    vm.memory[vm.registers[regular::Register::PC] + 1] << 8 |
	                    vm.memory[vm.registers[regular::Register::PC] + 2] << 16 |
	                    vm.memory[vm.registers[regular::Register::PC] + 3] << 24;
	vm.registers[regular::Register::PC] += 4;
	auto instruction = regular::createInstruction(encoding);
	std::visit([&](auto &&instruction) {
		Executor<std::decay_t<decltype(instruction)>>()(vm, instruction);
	},
	           instruction);
	return instruction;
}
} // namespace _VM
