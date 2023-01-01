*SUPER IMPORTANT: The compiler that I've used (clang++-15 (I've also tested it on g++-12, but that doesn't work either)) sometimes breaks the program if you tell it to optimize. Most of the time, this doesn't happen, but if your brainfuck source code contains a relatively simple infinite loop, the compilers I've tested this on will mistakenly optimize it out. This means, while you can turn on optimizations and get optimized compiled brainfuck, you probably shouldn't, just to be on the safe side (ALTHOUGH I will say it handles most programs fine, since they're sufficiently complex). As soon as your loop is more complicated, the compiler doesn't optimize it out and you're fine, the problem is simply that I have no idea what constitutes a simple loop and what constitutes a more complicated loop. I'm sure that as compilers get better, this problem will go away and you can safely use optimizations with this library, but until then all I'm trying to say is be cautious with the optimizations.
NOTE: To compile an unoptimized build, the command is:*
```bash
make unoptimized
```
*You can add all the flags and variables to that cmdline that you would normally add to the normal optimized make command.*

# brainfuck-tools
Some tooling for brainfuck. Currently only contains a C++ compile-time optimizing brainfuck compiler. Future plans include an interpreter and a brainfuck to turing machine table compiler, as well as possibly a compiler for a more advanced brainfuck with functions and such.

# C++ compile-time optimizing brainfuck compiler
This will compile your brainfuck source code into binary at compile-time and generate a function that you can call, which executes the brainfuck code natively.

## Usage
```cpp
#include "meta_brainfuck_compiler.h"

auto compiled_brainfuck_program = META_COMPILE_BRAINFUCK("<bf-src>", read_character_functor, write_character_functor);

int main() {
  meta::brainfuck_run_return_t ret = compiled_brainfuck_program.run();
  switch (ret) {
  case meta::brainfuck_run_return_t::SUCCESS: break;
  case meta::brainfuck_run_return_t::ALLOCATION_FAILURE: break;
  case meta::brainfuck_run_return_t::INPUT_FAILURE: break;
  case meta::brainfuck_run_return_t::OUTPUT_FAILURE: break;
  }
}
```

## Specifics
First of all, the code isn't very complicated and it's all in one header file, so if you have questions, try looking in there, maybe it'll become clearer.
For questions that still remain, open an issue or something and I'll try to explain.
Some important things to know are:
  + The brainfuck programs have unlimited memory to work with. Practically, it'll be limited by your hardware though. The way it works is the programs start at index 0 in memory. You can never go lower than 0, but as long as you abide by that, you can go as far right in the memory as you want, memory will be allocated by the runtime as you go.
  + To help you (the user) handle the boundless memory (which is implemented as a std::vector-like data-structure), I've added two functions to the return type of META_COMPILE_BRAINFUCK, next to the already existing run() function.
    + 1. \<prog\>.reset_state_keep_vec_reserved() --> resets the state so you can run the program from scratch again, but doesn't unreserve the data that the vector owns. This means that there will be potentially very little allocation done on the next run() call since all the required memory is still allocated.
    + 2. \<prog\>.reset_state_unreserve_vec() --> You can probably guess what this does. The reason this exists is so that you aren't forced to keep the memory allocated if you don't want to take up too much memory on the system. You can run() after this call, but the allocations will simply have to be done again while run() is running, since memory needs to be created.
  + The input and output functors that you (the user) are required to give when using META_COMPILE_BRAINFUCK serve as the only interface with which brainfuck code can directly communicate with you. These functions get called when either ',' or '.' is encountered in the brainfuck source code. You can implement these functions by reading and writing characters from the console, or you can read and write from the network or a buffer or whatever you want.
  + I should explain the signature of the functors though:
    + The input functor returns a uint16_t simply so that it can represent an input error with (uint16_t)-1. This will then cause run() to abort and the error to be reported back the the user. Every other return value must be a valid uint8_t value, which has been cast to uint16_t.
    + The output functor receives a uint8_t as an argument and returns either true or false, based on whether it succeeded or failed.
  + The signatures of your functors can be whatever you want as long as they are callable through the signatures that I've described above. This is checked at compile-time and we don't compile if they're not.
  + With long brainfuck sources, the recurive template instantiation depth limit of the compiler might get hit, in which case you might have to increase that limit so that your program can compile. You can do this with a make variable that I've built into the makefile: The cmdline looks as follows:
  ```bash
  make CUSTOM_RECURSIVE_TEMPLATE_MAX_DEPTH:=<new-max-depth>
  ```
  
## How does it work?
Well for one, it sounds a lot more complex that it actually is. I didn't actually need to write a fully fledged compiler and I didn't even need to touch optimization explicitly. I don't know what this structure is called (if it even has a name), but I've used recursive function calls containing constexpr if statements (while providing the brainfuck source code as a template parameter) to generate C++ code at compile-time. This works because the recursion is easy to optimize out because it's tail-call recursion (mostly), which causes the compiler to convert my huge ladder of recursive function calls into one function that contains a concatination of source codes of the recursive functions. This generated C++ code is obviously converted into binary by the surrounding C++ compiler and even optimized if you tell the C++ compiler to optimize, making the resulting binary pretty fast. This doesn't mean that the brainfuck code will be as fast as C++ code. The C++ compiler isn't able to optimize the code to the same degree as it can with normal C++ code because it hasn't been programmed to look for typical brainfuck programming patterns. Maybe in the future I'll implement my own compile-time optimization pass in order to detect some common patterns and translate those into optimized C++ code, which the surrounding C++ compiler can better work with. The second reason the optimization isn't fantastic is because brainfuck is incredibly low-level, meaning the goals of the programmer don't come through as much. It's a lot harder to know what the programmer was trying to do and as such it's harder to optimize in order to help him achieve his goal.
