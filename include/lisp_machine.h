#ifndef LISP_MACHINE_INCLUDED
	#define LISP_MACHINE_INCLUDED

	#include <stdint.h>
	#include <stdbool.h>
	#include "stack.h"

	#define NUM_OF_CELLS 65536

	#define SYS_FUNC_MAX_LENGTH 10

	#define EVAL_CONTEXT_EVAL 0
	#define EVAL_CONTEXT_EVLIS 1
	#define EVAL_CONTEXT_CONENV 2

	#define SYS_GENERAL 0
	#define SYS_CAR 	1
	#define SYS_CDR 	2
	#define SYS_CONS 	3
	#define SYS_EQ		4
	#define SYS_ATOM 	5
	#define SYS_IF 		6
	#define SYS_QUOTE 	7
	#define SYS_QUIT	8

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

		int mem_used;
		int mem_free;
		Cell *free_mem;
		Cell *nil;

		// $[func]
		Cell * sys_eval;
		Cell * sys_apply;
		Cell * sys_evlis;
		Cell * sys_evif;
		Cell * sys_conenv;
		Cell * sys_lookup;

		// System environment
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

		void *sys_func_memory_block;
		char **sys_funcs;
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
	void init_sys_function_list(char * funcs);
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