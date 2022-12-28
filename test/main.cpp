#include <iostream>

#include "meta_brainfuck_compiler.h"

auto compiled_brainfuck_program = META_COMPILE_BRAINFUCK("brainfuck goes here");

int main() {
	compiled_brainfuck_program.run();

	while (true) {
		if (!compiled_brainfuck_program.step()) { break; }
	}

	for (size_t i = 0; i < compiled_brainfuck_program.data.size(); i++) {
		std::cout << compiled_brainfuck_program.data[0] << " ";
	}
	std::cout << "\n";
}
