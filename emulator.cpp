#include "vm.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <type_traits>
#include <vector>

int main(int argc, char const **argv) {
	assert(argc == 2);
	VM vm;
	std::ifstream input(*++argv, std::ios::binary);
	for (auto i = vm.memory; input.read(reinterpret_cast<char *>(i), sizeof(uint32_t)); i += sizeof(uint32_t))
		;
	std::vector<regular::Register> registers;
	std::transform(regular::register_names.begin(), regular::register_names.end(), std::back_inserter(registers), [](auto &&pair) {
		return pair.first;
	});
	std::sort(registers.begin(), registers.end(), [](auto register1, auto register2) {
		return static_cast<std::underlying_type_t<decltype(register1)>>(register1) <
		       static_cast<std::underlying_type_t<decltype(register2)>>(register2);
	});
	while (true) {
		std::visit([](auto &&instruction) {
			std::cout << "Executing: " << instruction << std::endl;
		},
		           vm.step());
		int i = 0;
		for (auto r : registers) {
			std::printf("%3s = 0x%08x ", regular::register_names[r], vm.registers[r]);
			if (!(++i % 4)) {
				std::cout << std::endl;
			}
		}
		std::cout << std::endl;
	}
	return 0;
}
