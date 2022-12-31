#include <iostream>

#include "meta_brainfuck_compiler.h"

	auto compiled_brainfuck_program = META_COMPILE_BRAINFUCK("+[]",
		[]() -> uint16_t {
			return std::cin.get();
		}, 
		[](char character) -> bool {
			std::cout << (int)character << '\n';
			return true;
		});

int main() {
	if (!compiled_brainfuck_program.run()) { std::cout << "allocation error\n"; return 0; }

	std::cout << "mem view\n";

	for (size_t i = 0; i < compiled_brainfuck_program.data.length; i++) {
		std::cout << (int)compiled_brainfuck_program.data[i] << " ";
	}
	std::cout << "\n";
}
