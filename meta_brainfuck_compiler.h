#pragma once

#define META_BRAINFUCK_COMPILER_DATA_VECTOR_BUCKET_INC_DEFAULT (1024 * 4)

#include <new>
#include <cstdint>
#include <utility>
#include <cstdlib>

namespace meta {

	/*
	   Sadly, there's no step() function, so you can't step through your brainfuck code and execute it like that.
	   The reason is speed. This library effectively compiles brainfuck into machine code, and the goal is to be able to run it super
	   duper fast. If I have to return out of the execution function after every brainfuck operation and then go back to that
	   operation and start executing from there again when you call the step() function again, that adds a bunch of overhead.

	   I was thinking about making a debugger mode that you can opt in to, which would add these extra instructions to make stepping
	   through possible, but that would just overcomplicate things given that there is a simple alternative:
	   	--> If you want to step through your brainfuck programs, simply use a brainfuck interpreter instead of a brainfuck
		compiler. It's trivially integratable into an interpreter so why not just do that?
		I'll build an interpreter and put a debugger mode in it. (TODO)
	*/

	/*
NOTE: One weird thing you have to remember about passing const char*'s into templates is this:
const char*'s into templates are technically allowed, it's just that they can't point to string literals.
Now, we've explained this before, the reasoning (AFAIK) is two same string literals don't have to have the same pointer and as such
two instantiations of the same type with the same string literal template args can have totally different types.

IMPORTANT: The root cause of this disability in C++ is not related to string literals, there is simply a rule that states that
non-type template parameters have to have external linkage. AFAIK, there is no real reason why this is so,
the standard just figured (a long time ago) that is might be harder to implement if internal linkage was considered as well (idk if that
turned out to be true).
--> point is: a string literal returns an rvalue const char* ptr without a name, which can't really have external linkage,
so that's why that's not allowed. That's also why it's not allowed to fill a char array with a string literal first and then pass the
pointer to the char array. It simply does not have external linkage (unless you specify it, which is impossible to do with static variables in
functions).
--> That's why we have to pass strings the way we do: a reference to a pointer to a string literal.
Since references don't have the restrictions when it comes to their targets. Very strange, but alas C++ is a strange beast.
	*/

	namespace helpers {

		template <typename element_t, size_t bucket_size>
		class non_bad_vector {
		public:
			element_t* data = nullptr;
			size_t bucket_length = 0;
			size_t length = 0;

			consteval non_bad_vector() = default;

			// NOTE: See comment way below somewhere about how this deletes the assignment operator as well.
			non_bad_vector(const non_bad_vector& other) = delete;

			constexpr non_bad_vector(non_bad_vector&& other) noexcept : 
				data(other.data), bucket_length(other.bucket_length), length(other.length)
			{
				other.data = nullptr;
			}

			static non_bad_vector create_nulled_out_vec(size_t length) {
				non_bad_vector result;
				result.length = length;
				result.bucket_length = ((length - 1) % bucket_size) + 1;
				result.data = (element_t*)malloc(result.bucket_length * sizeof(element_t));
				if (!result.data) { return result; }
				for (size_t i = 0; i < length; i++) { new (result.data + i) element_t(0); }
				return result;
			}

			/*
			   The rule in C++ is that before you use an object, you have to start it's lifetime (construct it),
			   and if you at any point stop it's lifetime (destruct it), you can't use it after that unless you start
			   it's lifetime again (construct it again).
			   Stopping lifetime isn't necessarily required (that would be annoying, having to explicitly destruct
			   every array of chars you free from the heap), but STARTING LIFETIME IS OF UTMOST IMPORTANCE
			   if you want to avoid UB, even if it technically does practically absolutely nothing in this case.
			   Starting the lifetime serves to also set the underlying data type of the memory, which means it is also
			   something to consider when thinking about strict aliasing:

			   A_t* data = (A_t*)malloc(x);
			   B_t* data_b = new (data) B_t[y];
			   // ACCESSING THROUGH data IS UB
			   // ACCESSING THROUGH data_b IS ABSOLUTELY FINE

			   IMPORTANT: I assume placement new is super duper undefined if that pointer position already contains a
			   constructed object, so please don't do that. Once you set the underlying type, you can't change it without
			   destructing the object and re-constructing it, which (you are required to assume this) destroys the data as well.
			*/

			bool push_uninitialized_mem(size_t amount) noexcept {
				size_t new_bucket_length = bucket_length + amount;
				element_t* new_data = (element_t*)realloc(data, new_bucket_length * sizeof(element_t));
				if (!new_data) { return false; }
				bucket_length = new_bucket_length;
				data = new_data;
				return true;
			}

			// NOTE: We could use pointers instead of indices since that would suit this class's use-case a bit more.
			// But keeping this class general has it's advantages.
			// You also must consider that the compiler should easily be able to optimize the parts of the code that I have in mind,
			// so it's pretty much all good.

			template <typename element_ref_t>
			bool push_back(element_ref_t&& new_element) noexcept {
				if (length == bucket_length) { if (!push_uninitialized_mem(bucket_size)) { return false; } }
				element_t* ptr = new (data + length++) element_t;	// placement new to start lifetime of object
				*ptr = std::forward<element_ref_t>(new_element);
				return true;
			}

			const element_t& operator[](size_t index) const noexcept { return data[index]; }
			element_t& operator[](size_t index) noexcept { return data[index]; }

			void release_length_keep_reserve() noexcept {
				for (size_t i = 0; i < length; i++) { data[i].~element_t(); }
				length = 0;
			}

			// NOTE: Obviously using this requires you to push_back(0) afterwards in the case of this program's use-case.
			// This frees only to allocate again, which is slower than it need be.
			// Provided that realloc doesn't unnecessarily move the block around (this technically can't be assumed),
			// using realloc instead of this and push_back(0) would be faster.
			// The reason I don't do that is because I don't want to specialize this class for this program, I wanna keep it general.
			// That alone isn't a good reason, but combine it with the fact that the resetting code doesn't have to be fast at all
			// and the fact that there's a silver lining (freeing and re-allocating it is better for preventing
			// heap fragmentation, although for this program it doesn't really make a difference since we're not doing anything
			// else with the heap anyway).
			// That sounds like a good deal.
			void reset_everything() noexcept {
				release_length_keep_reserve();
				free(data);
				data = nullptr;		// NOTE: necessary for realloc to work right
				bucket_length = 0;
			}

			// We aren't actually required to end the lifetime of objects, but it seems fitting here.
			~non_bad_vector() {
				// NOTE: We aren't actually allowed to call delete here because that would try to free it from the heap,
				// which is bad since we're not allowed to mix C++ and C heap functions.
				// INSTEAD, we simply directly call the destructor, but only if the pointer is valid.
				// THEN, we free it from the heap with a C function.
				// IMPORTANT: Don't worry about the type not having a destructor, C++ makes sure all types have a valid,
				// callable destructor so that codes like this can remain simple.
				// INTERESTINGLY: I think it's only valid in a general context such as this. Writing x.~int() is illegal,
				// even if x is an int (AFAIK).
				if (data) { for (size_t i = 0; i < length; i++) { data[i].~element_t(); } }
				free(data);
			}
		};

	}

	enum class brainfuck_run_return_t : uint8_t {
		ALLOCATION_FAILURE,
		SUCCESS,
		INPUT_FAILURE,
		OUTPUT_FAILURE
	};

	template <typename A_t, typename B_t>
	struct are_types_same { consteval operator bool() const { return false; } };

	template <typename A_t>
	struct are_types_same<A_t, A_t> { consteval operator bool() const { return true; } };

	template <typename T, typename conduit_t>
	struct is_invokable_through { };

	template <typename T, typename conduit_ret_t, typename... conduit_param_types>
	struct is_invokable_through<T, conduit_ret_t(conduit_param_types...)> {
	private:
		using yes_ret_t = char[2];
		using no_ret_t = char[1];

		template <typename U, typename ret_t, typename... param_types, 
			 typename std::enable_if<are_types_same<decltype((*(U*)nullptr)((*(param_types*)nullptr)...)), ret_t> { }, bool>::type = true>
		consteval yes_ret_t& filter(void*) const;

		// NOTE: Apparently only funcs with templated definitions are considered in overload resolution, so we have to put this here.
		template <typename U, typename ret_t, typename... param_types>
		consteval no_ret_t& filter(...) const;

	public:
		consteval operator bool() const { return sizeof(filter<T, conduit_ret_t, conduit_param_types...>(nullptr)) == sizeof(yes_ret_t); }
	};

	template <const char * const& source_code_const_string_original, typename input_functor_t, typename output_functor_t, size_t data_vector_bucket_size, 
		 // NOTE: Doesn't care if the functions are noexcept or not, which is fine since that doesn't matter for us in this case.
		 // NOTE: Also doesn't give any thought to whether they're const or anything, which is also fine since that doesn't concern us.
		 typename std::enable_if<is_invokable_through<input_functor_t, uint16_t()> { } &&
			 is_invokable_through<output_functor_t, bool(uint8_t)> { }, bool>::type = true>
	class compiled_brainfuck_t {
	public:
		helpers::non_bad_vector<uint8_t, data_vector_bucket_size> data = helpers::non_bad_vector<uint8_t, data_vector_bucket_size>::create_nulled_out_vec(1);
		uint8_t* data_end_ptr = data.data + data.length;
		uint8_t* data_ptr = data.data;

		input_functor_t read_input_byte;
		output_functor_t write_output_byte;

		compiled_brainfuck_t(const input_functor_t& read_input_byte_callback, const output_functor_t& write_output_byte_callback) : read_input_byte(read_input_byte_callback), write_output_byte(write_output_byte_callback) { }

		consteval compiled_brainfuck_t(compiled_brainfuck_t<source_code_const_string_original, input_functor_t, output_functor_t, data_vector_bucket_size>&& other) : 
			data(std::move(other.data)), data_ptr(other.data_ptr), data_end_ptr(other.data_end_ptr), 
			read_input_byte(other.read_input_byte), write_output_byte(other.write_output_byte)
		{ }

		// NOTE: I forgot when this is going to be introduced, but I remember that this should (eventually) imply
		// that the copy assignment operator is also deleted. So this one line does everything we need it to.
		compiled_brainfuck_t(const compiled_brainfuck_t& other) = delete;

		bool increment_data_ptr() noexcept {
			if (++data_ptr == data_end_ptr) {
				if (!data.push_back(0)) { return false; }
				data_end_ptr = data.data + data.length;
				data_ptr = data_end_ptr - 1;
			}
			return true;
		}

		bool decrement_data_ptr() noexcept {
			// NOTE: We have to do it like this to avoid pointer underflow/overflow, which is super duper UB.
			if (data_ptr == data.data) { return false; }
			data_ptr--;
			return true;
		}

		/*
		   NOTE: You've been wondering why compiled switch statements (on my x86-64 machine)
		   choose to load a target addr from memory and slow jump to that instead of slow jumping to an addr
		   at a specific offset, which contains a fast (unconditional) jump to the target code.
		   Basically it comes down to this question: Is a fast jump of memory access faster?
		   Apparently, at least on my machine, memory access must be faster, or else this optimization wouldn't be done.
		   I've done some research, and when the L1 cache is hit (which I guess is what the compiler is optimizing for here),
		   the access time is just a tiny tad bit slower than talking to a register, so it's unbelievably fast.
		   You might think fast jumps are faster because they must only take like 1 clock cycle right?
		   That's not actually true, they can be slow (and I guess they are slower than L1 cache hits most of the time),
		   since because of the way the CPU is pipelined, the CPU doesn't actually know if the jump instruction is
		   conditional or unconditional when it first enters the pipe, it takes a few stages till it definitively figures that out.
		   That means it still has to use the branch predictor, even for fast jumps, since it needs to keep fetching instructions
		   somewhere and it has no other option than to predict where until it can correct.
		   This adds overhead and makes even a fast jump undesirable in the face of a perfect memory access.
		*/

		template <const char * const& source_code_const_string, size_t src_offset, size_t baseline_depth, size_t depth>
		brainfuck_run_return_t loop_skip() noexcept {
			constexpr const char* source_code_ptr = source_code_const_string + src_offset;

			if constexpr (*source_code_ptr == '[') { return loop_skip<source_code_const_string, src_offset + 1, baseline_depth, depth + 1>(); }

			else if constexpr (*source_code_ptr == ']') {
				if constexpr (depth == baseline_depth) { return inner_run<source_code_const_string, src_offset + 1, baseline_depth - 1>(); }
				else { return loop_skip<source_code_const_string, src_offset + 1, baseline_depth, depth - 1>(); }
			}

			else {
				static_assert(*source_code_ptr != '\0', "brainfuck compilation failed: '[' character did not posess a succeeding ']' character");
				return loop_skip<source_code_const_string, src_offset + 1, baseline_depth, depth>();
			}
		}

		template <const char * const& source_code_const_string, size_t src_offset, size_t depth>
		brainfuck_run_return_t inner_run() noexcept {
			constexpr const char* source_code_ptr = source_code_const_string + src_offset;

			if constexpr (*source_code_ptr == '>') {
				if (!increment_data_ptr()) { return brainfuck_run_return_t::ALLOCATION_FAILURE; }
				return inner_run<source_code_const_string, src_offset + 1, depth>();
			}

			else if constexpr (*source_code_ptr == '<') {
				if (!decrement_data_ptr()) { return brainfuck_run_return_t::ALLOCATION_FAILURE; }
				return inner_run<source_code_const_string, src_offset + 1, depth>();
			}

			// NOTE: Don't worry, the brainfuck program can totally overflow the data values without causing UB. It's ok.
			else if constexpr (*source_code_ptr == '+') {
				(*data_ptr)++;
				return inner_run<source_code_const_string, src_offset + 1, depth>();
			}

			else if constexpr (*source_code_ptr == '-') {
				(*data_ptr)--;
				return inner_run<source_code_const_string, src_offset + 1, depth>();
			}

			else if constexpr (*source_code_ptr == '[') {
				while (*data_ptr != 0) {
					brainfuck_run_return_t return_value = inner_run<source_code_const_string, src_offset + 1, depth + 1>();
					if (return_value != brainfuck_run_return_t::SUCCESS) { return return_value; }
				}
				constexpr size_t new_depth = depth + 1;
				return loop_skip<source_code_const_string, src_offset + 1, new_depth, new_depth>();
			}

			else if constexpr (*source_code_ptr == ']') {
				static_assert(depth != 0, "brainfuck compilation failed: ']' character did not posess a preceding '[' character");
				return brainfuck_run_return_t::SUCCESS;
			}
			
			else if constexpr (*source_code_ptr == ',') {
				uint16_t input_result = read_input_byte();
				if (input_result == (uint16_t)-1) { return brainfuck_run_return_t::INPUT_FAILURE; }
				*data_ptr = input_result;
				return inner_run<source_code_const_string, src_offset + 1, depth>();
			}
			
			else if constexpr (*source_code_ptr == '.') {
				if (!write_output_byte(*data_ptr)) { return brainfuck_run_return_t::OUTPUT_FAILURE; }
				return inner_run<source_code_const_string, src_offset + 1, depth>();
			}

			else if constexpr (*source_code_ptr == '\0') {
				static_assert(depth == 0, "brainfuck compilation failed: '[' character did not posess a succeeding ']' character");
				return brainfuck_run_return_t::SUCCESS;
			}

			else { return inner_run<source_code_const_string, src_offset + 1, depth>(); }
		}

		brainfuck_run_return_t run() noexcept {
			if (!data.data) { return brainfuck_run_return_t::ALLOCATION_FAILURE; }
			return inner_run<source_code_const_string_original, 0, 0>();
		}

		bool reset_state_keep_vec_reserved() noexcept {
			data.release_length_keep_reserve();
			data.push_back(0);
			data_ptr = data.data;
			data_end_ptr = data_ptr + 1;
		}

		bool reset_state_unreserve_vec() noexcept {
			data.reset_everything();
			data.push_back(0);
			data_ptr = data.data;
			data_end_ptr = data_ptr + 1;
		}
	};

// NOTE: The callbacks can be any functor, including function pointers,
// but you probably shouldn't use function pointers, since
// calls to other functors are easier to optimize out for the compiler.
// If you do use function pointers, at least have the
// target functions reside in the same translation unit as the
// compiled brainfuck program. That way, the compiler
// should have a much easier time inlining the function calls.
#define META_COMPILE_BRAINFUCK_WITH_CUSTOM_BUCKET_INC_SIZE(source_code, read_input_byte_callback, write_output_byte_callback, bucket_inc_size) []() { \
auto read_input_byte_callback_temp = read_input_byte_callback; \
auto write_output_byte_callback_temp = write_output_byte_callback; \
static constexpr const char *source_code_ptr = source_code; \
return meta::compiled_brainfuck_t<source_code_ptr, decltype(read_input_byte_callback_temp), decltype(write_output_byte_callback_temp), bucket_inc_size>(read_input_byte_callback_temp, write_output_byte_callback_temp); \
}()
//return meta::compiled_brainfuck_t<source_code_const_string>();	// This is something we had in a previous revision, seems weird but works.
// Explanation: because it can't be a type in this context, the compiler is smart enought to pick up on the meaning.
// For template args it's different because first an argument list is constructed and then that list is compared with every found template
// definition of the function and a set of valid overloads is constructed. These are then further narrowed down by comparing the function
// argument lists.
// The bottom line is that the compiler doesn't know what type of argument it's shooting for before constructing the initial list, so it
// always assumes type instead of non-type when it can, because that makes the most sense.

#define META_COMPILE_BRAINFUCK(source_code, read_input_byte_callback, write_output_byte_callback) META_COMPILE_BRAINFUCK_WITH_CUSTOM_BUCKET_INC_SIZE(source_code, read_input_byte_callback, write_output_byte_callback, META_BRAINFUCK_COMPILER_DATA_VECTOR_BUCKET_INC_DEFAULT)

}
