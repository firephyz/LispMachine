#ifndef LISP_MACHINE_INCLUDED
	#define LISP_MACHINE_INCLUDED

	#include <stdint.h>
	#include <stdbool.h>
	#include "stack.h"

	#define NUM_OF_CELLS 65536

	#define INSTR_MAX_LENGTH 10

	/********************************* Cell Types *******************************/
	// General variable symbol
	#define SYS_GENERAL 0

	// Type for return function record in system stack
	#define SYS_RETURN_RECORD 1

	// System instructions
	#define SYS_SYM_ATOM 	2
	#define SYS_SYM_CAR 	3
	#define SYS_SYM_CDR		4
	#define SYS_SYM_CONS	5
	#define SYS_SYM_EQ 		6
	#define SYS_SYM_FALSE	7
	#define SYS_SYM_IF		8
	#define SYS_SYM_LAMBDA	9
	#define SYS_SYM_NULL	10
	#define SYS_SYM_QUIT	11
	#define SYS_SYM_QUOTE	12
	#define SYS_SYM_TRUE	13
	/********************************* End Cell Types ***************************/

	// Used for machine.calling_func so that functions know where to return.
	#define SYS_EVAL_0		0
	#define SYS_EVAL_1		1
	#define SYS_APPLY_0		2
	#define SYS_APPLY_1		3
	#define SYS_EVLIS		4
	#define SYS_EVIF		5
	#define SYS_CONENV		6
	#define SYS_LOOKUP		7
	#define SYS_REPL		8

	extern int chars_per_pointer;

	typedef struct cell_t Cell;
	typedef struct lisp_machine_t Lisp_Machine;

	struct cell_t {
		Cell *car;
		char null_byte1; // Used to provide a null terminator if the car is interpreted as a string
		Cell *cdr;
		bool is_atom;
		int type;
	};

	// ******************** Notice regarding the Evaluation Environment  *********************
	// NOTICE:
	// I have a choice in regards to the implementation of the evaluator environment.
	// 1) I can used a simple association list. Fairly straight forward
	// 		- Actually, this may not work. If evaluation arguments are found in
	//			an A-list to be evaluated by the evaluator itself, then what stops
	//			the infinitely recursive A-lists required for evaluation?
	//		- One choice to resolve this is to not use the evaluator to self-evaluate
	//			the A-lists.
	// 2) I can instead use special registers for storing the addresses of lists
	// 		necessary for the evaluation of a evaluation function.
	// The current implementation will use method 2.

	// *******************  Machine registers and atom evaluation rules *********************
	/*
	The machine keeps track of important lists necessary for evaluation.
	$[func] - these are calls to the system evaluation functions (eval, apply, evlis, evif, conenv, lookup)
	@[func] - Denotes a variable to be found in the system environment.
				- These include:
					expr (the current expression being evaluated)
					env  (the current environment in which to evaluate the expression)
					pred (the predicate used in the $[evif] evaluator function)
	Everything else is split among the axiomatic functions (car, cdr, cons, eq?, atom?, if, quote, lambda)
	*/

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
	Cell * if_cond(Cell * pred, Cell * true_value, Cell * false_value);

#endif