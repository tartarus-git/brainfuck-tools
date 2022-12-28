#pragma once

#include <vector>
#include <cstdint>

namespace meta {

	struct const_char_const_ptr {
		const char* const ptr;

		consteval const_char_const_ptr offset(size_t amount) { return { ptr + amount }; }
	}

	template <const char* source_code_ptr, typename input_functor_t, typename output_functor_t>
	class compiled_brainfuck_t {
		std::vector<uint8_t> data;
		uint8_t* data_ptr = &data;

		input_functor_t read_input_byte;
		output_functor_t write_output_byte;

		// TODO: Make sure that you can't do anything with
		// this class that your not supposed to.
		// Copying it around could be cool though
		// because then you could save the state of a program
		// super easily into a backup or something.

		// TODO: Add a debugger mode where the run
		// function adds instructions into the mix that
		// return after every brainfuck instruction with
		// an enum saying which instruction it was at
		// which position in the program source code.

		consteval compiled_brainfuck_t(input_functor_t read_input_byte_callback, output_functor_t write_output_byte) : read_input_byte(read_input_byte_callback), write_output_byte(write_output_byte_callback) { }

		bool increment_data_ptr() noexcept {
			data_ptr++;
			if (data_ptr == data.size()) { data.push_back(0); }
			return true;
			// TODO: Replace vector with your own that
			// reports errors properly.
		}

		bool decrement_data_ptr() noexcept {
			return --data_ptr < &data;
		}

		enum class loop_run_return_t : uint8_t {
			ALLOCATION_FAILURE
			ESCAPED,
			LOOPED,
		};

		template <const char* source_code_ptr>
		loop_run_return_t loop_run() noexcept {
			if constexpr (*source_code_ptr == '>') {
				if (!increment_data_ptr()) { return false; }
			} else if constexpr (*source_code_ptr == '<') {
				if (!decrement_data_ptr()) { return false; }
			} else if constexpr (*source_code_ptr == '+') {
				*data_ptr++;
			} else if constexpr (*source_code_ptr == '-') {
				*data_ptr--;
			} else if constexpr (*source_code_ptr == '[') {
				while (true) {
					loop_run_return_t loop_result = loop_run<source_code_ptr + 1>();
					if (loop_result == loop_run_return_t::LOOPED) { continue; }
					return loop_result;
				}
			} else if constexpr (*source_code_ptr == ']') {
				if (*data_ptr != 0) { return loop_run_return_t::LOOPED; }
				// NOTE: This return statement is ignoring the loop_run_return_t enum on purpose, it's fine-tuned so that this works.
				return inner_run<source_code_ptr + 1>();
			} else if constexpr (*source_code_ptr == ',') {
				uint16_t input_result = read_input_byte();
				if (input_result == (uint16_t)-1) { return false; }
				*data_ptr = input_result;
			} else if constexpr (*source_code_ptr == '.') {
				if (!write_output_byte(*data_ptr)) { return false; }
			}

			return loop_run<source_code_ptr + 1>();
		}

		template <const char* source_code_ptr>
		bool inner_run() noexcept {
			if constexpr (*source_code_ptr == '>') {
				if (!increment_data_ptr()) { return false; }
			} else if constexpr (*source_code_ptr == '<') {
				if (!decrement_data_ptr()) { return false; }
			} else if constexpr (*source_code_ptr == '+') {
				*data_ptr++;
			} else if constexpr (*source_code_ptr == '-') {
				*data_ptr--;
			} else if constexpr (*source_code_ptr == '[') {
				while (true) {
					loop_run_return_t loop_result = loop_run<source_code_ptr + 1>();
					if (loop_result == loop_run_return_t::LOOPED) { continue; }
					return loop_result;
				}
			} else if constexpr (*source_code_ptr == ',') {
				uint16_t input_result = read_input_byte();
				if (input_result == (uint16_t)-1) { return false; }
				*data_ptr = input_result;
			} else if constexpr (*source_code_ptr == '.') {
				if (!write_output_byte(*data_ptr)) { return false; }
			}

			static_assert(*source_code_ptr != ']', "brainfuck compilation failed: ']' character must come after a '[' character");

			return inner_run<source_code_ptr + 1>();
		}

		bool run() noexcept { return inner_run<source_code_ptr>(); }
	};

// NOTE: The callbacks can be any functor, including function pointers,
// but you probably shouldn't use function pointers, since
// calls to other functors are easier to optimize out for the compiler.
// If you do use function pointers, at least have the
// target functions reside in the same translation unit as the
// compiled brainfuck program. That way, the compiler
// should have a much easier time inlining the function calls.
#define META_COMPILE_BRAINFUCK(source_code, read_input_byte_callback, write_output_byte_callback) []() { \
constexpr const char* source_code_ptr = source_code; \
return meta::compiled_brainfuck_t<source_code_ptr>(read_input_byte_callback, write_output_byte_callback); \
return meta::compiled_brainfuck_t<source_code_ptr>();		/* TODO: Does this work? I thought this always became a type? */ \
}()

}
