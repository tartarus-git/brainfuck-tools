#include <iostream>

#include "meta_brainfuck_compiler.h"

	// program for fibonacci numbers, doesn't stop, you gotta interrupt it.
	auto compiled_brainfuck_program = META_COMPILE_BRAINFUCK(R"(>++++++++++>+>+[
    [+++++[>++++++++<-]>.<++++++[>--------<-]+<<<]>.>>[
        [-]<[>+<-]>>[<<+>+>-]<[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
            [>+<-[>+<-[>+<-[>[-]>+>+<<<-[>+<-]]]]]]]]]]]+>>>
    ]<<<
])",
		[]() -> uint16_t {
			return std::cin.get();
		}, 
		[](char character) -> bool {
			std::cout << character;
			return true;
		});

int main() {
	switch (compiled_brainfuck_program.run()) {
	case meta::brainfuck_run_return_t::SUCCESS: break;
	case meta::brainfuck_run_return_t::ALLOCATION_FAILURE: std::cout << "allocation failure\n"; break;
	case meta::brainfuck_run_return_t::INPUT_FAILURE: std::cout << "input failure\n"; return 0;
	case meta::brainfuck_run_return_t::OUTPUT_FAILURE: std::cout << "output failure\n"; return 0;
	}

	std::cout << "state\n";

	std::cout << "data_ptr offset: " << compiled_brainfuck_program.data_ptr - compiled_brainfuck_program.data.data << "\n";

	std::cout << "mem view\n";

	for (size_t i = 0; i < compiled_brainfuck_program.data.length; i++) {
		std::cout << (int)compiled_brainfuck_program.data[i] << " ";
	}
	std::cout << "\n";
}
