#ifndef LISP_MACHINE_INCLUDED
	#define LISP_MACHINE_INCLUDED

	#include <stdint.h>
	#include <stdbool.h>
	#include <time.h>
	#include "stack.h"

	#define NUM_OF_CELLS 65536

	#define INSTR_MAX_LENGTH 10
	#define INPUT_BUFFER_LENGTH 64

	/********************************* Cell Types *******************************/
	// General variable symbol
	#define SYS_GENERAL 0

	// Type for return function record in system stack
	#define SYS_RETURN_RECORD 1

	// System instructions
	#define SYS_SYM_MULT	2
	#define SYS_SYM_ADD		3
	#define SYS_SYM_SUB		4
	#define SYS_SYM_DIV		5
	#define SYS_SYM_LESS	6
	#define SYS_SYM_EQUAL	7
	#define SYS_SYM_GREAT	8
	#define SYS_SYM_AND		9
	#define SYS_SYM_ATOM 	10
	#define SYS_SYM_BEGIN	11
	#define SYS_SYM_CAR 	12
	#define SYS_SYM_CDR		13
	#define SYS_SYM_CHARAT	14
	#define SYS_SYM_CONS	15
	#define SYS_SYM_DEFINE	16
	#define SYS_SYM_EQ 		17
	#define SYS_SYM_EVAL	18
	#define SYS_SYM_FALSE	19
	#define SYS_SYM_IF		20
	#define SYS_SYM_IN 		21
	#define SYS_SYM_JOIN	22
	#define SYS_SYM_LAMBDA	23
	#define SYS_SYM_MOD		24
	#define SYS_SYM_NOT		25
	#define SYS_SYM_NULL	26
	#define SYS_SYM_OR		27
	#define SYS_SYM_OUT 	28
	#define SYS_SYM_QUIT	29
	#define SYS_SYM_QUOTE	30
	#define SYS_SYM_SUBSTR	31
	#define SYS_SYM_TRUE	32

	// Self evaluating number
	#define SYS_SYM_NUM		33

	// Tag for a string
	#define SYS_SYM_STRING	34
	#define SYS_SYM_CHAR	35

	/********************************* System Calling Functions ***************************/
	// Used for machine.calling_func so that functions know where to return.
	#define SYS_EVAL		0
	#define SYS_APPLY_0		1
	#define SYS_APPLY_1		2
	#define SYS_APPLY_2		3
	#define SYS_EVLIS_0		4
	#define SYS_EVLIS_1		5
	#define SYS_EVIF		6
	#define SYS_EVBEGIN		7
	#define SYS_CONENV		8
	#define SYS_RETURN 		9
	#define SYS_REPL		10

	#define SYSCALL(func)													\
	do {																	\
		if(runtime_info_flag) {												\
			print_runtime_info(#func);										\
			struct timespec t = {0, 150999999};								\
			nanosleep(&t, NULL);											\
		}																\
		goto func;															\
	} while(0)																\


	extern int chars_per_pointer;

	typedef struct cell_t Cell;
	typedef struct lisp_machine_t Lisp_Machine;

	struct cell_t {
		Cell *car;
		char null_byte1; // Used to provide a null terminator if the car is interpreted as a string
		Cell *cdr;
		bool is_atom;
		int type;

		// Runtime information
		bool is_fetched;
	};

	// ******************** Functions that should be present in the global environment *********************
	// These functions won't actually be in the environment variable, they will instead be
	// specially marked by the parser as they are encountered.
	/*
		car
		cdr
		cons
		eq?
		atom?
		if
		quote

		quit
	*/

	struct lisp_machine_t {
		bool is_running;
		Cell * memory_block;

		// System memory info
		int mem_used;
		int mem_free;
		Cell *free_mem;
		Cell *nil;
		//bool needs_return_address; // Set when the calling function needs it's return address. Otherwise, we will use that
			// stack frame to record the next return address.

		int memory_access_count;
		int cycle_count;

		// System environment
		// This serve as registers to hold the arguments to the evaluation functions
		Cell * sys_stack;
		int sys_stack_size;

		// Small, one context sized stack, that holds all the args necessary
		// Holds the result from the last evaluation
		Cell * result;

		// Used at temporary storage of current_args variables so that we
		// don't have to car and cdr so much. Might not be used
		// in actual hardware implementation.
		Cell *args[4];
		uint8_t calling_func;

		int num_of_instrs;
		void *instr_memory_block;	// The actual memory supporting the variable instructions.
		char **instructions;				// Holds the list of supported instructions in alphabetical order.
	};

	Lisp_Machine * init_machine();
	void init_instr_list(char * funcs);
	void destroy_machine(Lisp_Machine *machine);
	Cell * get_free_cell();
	void store_cell(Cell * cell);
	void push_system_args(int arg_count);
	void pop_system_args();
	void execute();

	Cell * car(Cell * cell);
	Cell * cdr(Cell * cell);
	Cell * cons(Cell * cell1, Cell * cell2);
	Cell * quote(Cell * cell);
	Cell * atom(Cell * cell);
	Cell * eq(Cell * cell1, Cell * cell2);

#endif