#ifndef LISP_MACHINE_INCLUDED
	#define LISP_MACHINE_INCLUDED

	#include <stdint.h>
	#include <stdbool.h>
	#include "stack.h"

	#define NUM_OF_CELLS 65536

	#define INSTR_MAX_LENGTH 10

	#define EVAL_CONTEXT_EVAL 0
	#define EVAL_CONTEXT_EVLIS 1
	#define EVAL_CONTEXT_CONENV 2

	// General variable symbol
	#define SYS_GENERAL 0

	// System instructions
	#define SYS_SYM_CAR 	1
	#define SYS_SYM_CDR 	2
	#define SYS_SYM_CONS 	3
	#define SYS_SYM_EQ		4
	#define SYS_SYM_ATOM 	5
	#define SYS_SYM_IF 		6
	#define SYS_SYM_QUOTE 	7
	#define SYS_SYM_QUIT	8

	// Arguments to evaluation functions
	// Has their own registers
	#define SYS_ARG_APPLY_FUNC 		9
	#define SYS_ARG_APPLY_ARGS 		10
	#define SYS_ARG_APPLY_ENV 		11
	#define SYS_ARG_EVIF_PRED 		12
	#define SYS_ARG_EVIF_THEN 		13
	#define SYS_ARG_EVIF_ELSE 		14
	#define SYS_ARG_EVIF_ENV 		15
	#define SYS_ARG_LOOKUP_VAR 		16
	#define SYS_ARG_LOOKUP_ENV 		17
	// These are kept in the sytem stack
	#define SYS_ARG_EVAL_EXPR		18
	#define SYS_ARG_EVAL_ENV		19
	#define SYS_ARG_EVLIS_ARGS		20
	#define SYS_ARG_EVLIS_ENV		21
	#define SYS_ARG_CONENV_VARS		22
	#define SYS_ARG_CONENV_ARGS		23
	#define SYS_ARG_CONENV_ENV		24

	// Evaluation functions
	#define SYS_FUNC_EVAL			25
	#define SYS_FUNC_APPLY			26
	#define SYS_FUNC_EVLIS			27
	#define SYS_FUNC_EVIF			28
	#define SYS_FUNC_CONENV			29
	#define SYS_FUNC_LOOKUP			30

	// Other reserved symbols
	#define SYS_SYM_NULL			32
	#define SYS_SYM_FALSE			33
	#define SYS_SYM_TRUE			34

	extern int chars_per_pointer;

	typedef struct cell_t Cell;
	typedef struct lisp_machine_t Lisp_Machine;
	typedef struct eval_context_t Eval_Context;

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

		// Evaluation functions
		// $[func]
		Cell * sys_eval;
		Cell * sys_apply;
		Cell * sys_evlis;
		Cell * sys_evif;
		Cell * sys_conenv;
		Cell * sys_lookup;

		// System environment
		// This serve as registers to hold the arguments to the evaluation functions
		Stack sys_env_stack;

		Cell *apply_func;
		Cell *apply_args;
		Cell *apply_env;

		Cell *evif_pred;
		Cell *evif_then;
		Cell *evif_else;
		Cell *evif_env;

		Cell *lookup_var;
		Cell *lookup_env;

		int num_of_instrs;
		void *instr_memory_block;	// The actual memory supporting the variable instructions.
		char **instructions;				// Holds the list of supported instructions in alphabetical order.
		uint8_t *instr_types;			// Once we find a symbol matching a function, we return its corresponding type
	};

	union Eval_Context_Frame {
		Cell *eval_args[2];
		Cell *evlis_args[2];
		Cell *conenv_args[3];
	};

	struct eval_context_t {
		int func;
		union Eval_Context_Frame frame;
	};

	Lisp_Machine * init_machine();
	void init_instr_list(char * funcs);
	void destroy_machine(Lisp_Machine *machine);
	Cell * get_free_cell();
	void store_cell(Cell * cell);
	void execute(Lisp_Machine * lm);

	Cell * car(Cell * cell);
	Cell * cdr(Cell * cell);
	Cell * cons(Cell * cell1, Cell * cell2);
	Cell * quote(Cell * cell);
	Cell * atom(Cell * cell);
	Cell * eq(Cell * cell1, Cell * cell2);
	Cell * if_cond(Cell * pred, Cell * true_value, Cell * false_value);

#endif