#include "lisp_machine.h"
#include "expr_parser.h"
#include "repl.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int chars_per_pointer = sizeof(uintptr_t) / sizeof(char);
bool verbose_flag;
Lisp_Machine * machine;

Lisp_Machine * init_machine() {

	machine = malloc(sizeof(Lisp_Machine));
	machine->is_running = true;
	machine->mem_used = 0;
	machine->mem_free = NUM_OF_CELLS;

	if(verbose_flag) {
		printf("Initializing machine...\n");
	}

	// Create and link the memory cells
	machine->memory_block = calloc(sizeof(Cell) * NUM_OF_CELLS, sizeof(Cell));
	machine->free_mem = machine->memory_block;
	for(int i = 0; i < NUM_OF_CELLS - 1; ++i) {
		machine->free_mem[i].cdr = &machine->free_mem[i + 1];
	}

	// Setup the nil atom
	machine->nil = malloc(sizeof(Cell));
	machine->nil->car = NULL;
	machine->nil->cdr = NULL;
	machine->nil->is_atom = true;

	// Initialize the system functions character list
	init_sys_function_list("atom? car cdr cons eq? if quit quote");

	// Setup the evaluator code
	// Stack-recursive
	machine->sys_eval = make_expression("							\
		(if (atom? @[expr])											\
            ($[lookup] @[expr] @[env])								\
            (if (eq? (car @[expr]) (quote if))						\
                ($[evif] (car (cdr @[expr]))						\
                         (car (cdr (cdr @[expr])))					\
                         (car (cdr (cdr (cdr @[expr]))))			\
                         @[env])									\
                (if (eq? (car @[expr]) (quote quote))				\
                    (car (cdr @[expr]))								\
                    (if (eq? (car @[expr]) (quote lambda))			\
                        @[expr] 									\
                        ($[apply] (car @[expr])						\
                                  ($[evlis] (cdr @[expr]) @[env])	\
                                  @[env])))))");
	// Tail-recursive
	machine->sys_apply = make_expression("							\
		(if (atom? @[func])											\
            (if (eq? @[func] (quote car))							\
                (car (car @[args]))									\
                (if (eq? @[func] (quote cdr))						\
                    (cdr (car @[args]))								\
                    (if (eq? @[func] (quote cons))					\
                        (cons (car @[args]) (car (cdr @[args])))	\
                        (if (eq? @[func] (quote eq?))				\
                            (eq? (car @[args]) (car (cdr @[args])))	\
                            (if (eq? @[func] (quote atom?))			\
                                (atom? (car @[args]))				\
                                ($[apply] ($[eval] @[func] @[env])	\
                                          @[args] 					\
                                          @[env]))))))				\
            ($[eval] (car (cdr (cdr @[func])))						\
                     ($[conenv] (car (cdr @[func]))					\
                                @[args]								\
                                @[env])))");
    // Stack-recursive
	machine->sys_evlis = make_expression("							\
		(if (eq? @[args] @[null])									\
            @[null]													\
            (cons ($[eval] (car @[args]) @[env])					\
                  ($[evlis] (cdr @[args]) @[env])))");
	// Non-recursive
	machine->sys_evif = make_expression("							\
		(if ($[eval] @[pred] @[env])								\
            ($[eval] @[then] @[env])								\
            ($[eval] @[else] @[env]))");
	// Stack-recursive
	machine->sys_conenv = make_expression("							\
		(if (eq? @[vars] @[null])									\
            @[env]													\
            (cons (cons (car @[vars]) (car @[args]))				\
                  ($[conenv] (cdr @[vars])							\
                             (cdr @[args])							\
                             @[env])))");
	// tail-recursive
	machine->sys_lookup = make_expression("							\
		(if (eq? (car (car @[env])) @[var])							\
            (cdr (car @[env]))										\
            ($[lookup] @[var] (cdr @[env])))");

	// Initialize the machine system environment
	MAKE_STACK(machine->sys_env_stack, Eval_Context);

	if(verbose_flag) {
		printf("Machine initialized!\n");
	}

	return machine;
}

// Creates an array with all the names of the system functions.
// Expects a string containing all the function names
// each seperated by some whitespace. Must have no whitespace 
// surrounding the whole string
void init_sys_function_list(char * funcs) {

	int func_count = 1;
	bool parsing_white_space = false;
	for(int i = 0; i < strlen(funcs); ++i) {
		if(funcs[i] == ' ' || funcs[i] == '\t') {
			if(!parsing_white_space) {
				++func_count;
				parsing_white_space = true;
			}
		}
		else {
			parsing_white_space = false;
		}
	}

	char (*memory_block)[SYS_FUNC_MAX_LENGTH + 1] = malloc(sizeof(char) * (SYS_FUNC_MAX_LENGTH + 1) * func_count);
	char **sys_funcs = malloc(sizeof(char *) * func_count);
	uint8_t *sys_func_types = malloc(sizeof(uint8_t) * func_count);

	int func_index = 0;
	int string_index = 0;
	for(int i = 0; i < strlen(funcs); ++i) {
		if(funcs[i] == ' ' || funcs[i] == '\t') {
			if(!parsing_white_space) {
				++func_index;
				parsing_white_space = true;
			}

			string_index = 0;
		}
		else {
			parsing_white_space = false;
			memory_block[func_index][string_index] = funcs[i];
			++string_index;
		}
	}

	for(int i = 0; i < func_count; ++i) {
		sys_funcs[i] = (char *)&memory_block[i];
	}

	machine->sys_func_memory_block = memory_block;
	machine->sys_funcs = sys_funcs;
	machine->num_of_sys_funcs = func_count;
	machine->sys_func_types = sys_func_types;

	sys_func_types[0] = SYS_CAR;
}

void destroy_machine(Lisp_Machine *machine) {

	DESTROY_STACK(&(machine->sys_env_stack));
	free(machine->sys_func_memory_block);
	free(machine->sys_funcs);
	free(machine->memory_block);
	free(machine->nil);
	free(machine);
}

Cell * get_free_cell() {

	++machine->mem_used;
	--machine->mem_free;

	Cell * new_cell = machine->free_mem;
	machine->free_mem = cdr(machine->free_mem);
	new_cell->cdr = NULL;

	return new_cell;
}

void store_cell(Cell * cell) {
	cell->car = machine->free_mem;
	machine->free_mem = cell;
}

void execute(Lisp_Machine * lm) {

	Cell * cell = lm->nil;

	while(machine->is_running) {
		switch(cell->type) {
			default:
				break;
		}
	}
}

/*
 * Machine specific LISP functions
 */

Cell * car(Cell * cell) {
	return cell->car;
}

Cell * cdr(Cell * cell) {
	return cell->cdr;
}

Cell * cons(Cell * cell1, Cell * cell2) {

	Cell * new_cell = get_free_cell();

	new_cell->car = cell1;
	new_cell->cdr = cell2;

	return new_cell;
}

Cell * quote(Cell * cell) {
	return cell;
}

Cell * atom(Cell * cell) {

	if(cell->is_atom) {
		return NULL;
	}
	else {
		return machine->nil;
	}
}

// Really only implements an if statement, not cond
Cell * if_cond(Cell * pred, Cell * true_value, Cell * false_value) {

	if(pred == machine->nil) {
		return false_value;
	}
	else return true_value;
}

Cell * eq(Cell * cell1, Cell * cell2) {

	if(cell1 == machine->nil && cell2 == machine->nil) {
		return NULL;
	}
	else if (cell1 != machine->nil && cell2 != machine->nil) {

		if(cell1->is_atom && cell2->is_atom) {

			while(true) {
				uintptr_t copy1 = (uintptr_t)cell1;
				uintptr_t copy2 = (uintptr_t)cell2;

				char name1[chars_per_pointer];
				char name2[chars_per_pointer];

				for(int i = chars_per_pointer - 1; i >= 0; --i) {
					name1[i] = (char)(copy1);
					name2[i] = (char)(copy2);

					copy1 = copy1 >> 1;
					copy2 = copy2 >> 2;
				}

				if(strcmp(name1, name2) == 0) {
					if(cdr(cell1) != NULL && cdr(cell2) != NULL) {
						cell1 = cdr(cell1);
						cell2 = cdr(cell2);
					}
					else if(cdr(cell1) == NULL && cdr(cell2) == NULL) {
						return NULL;
					}
					else {
						return machine->nil;
					}
				}
				else {
					return machine->nil;
				}
			}
		}
		else {
			// Somehow report that the given arguments are not atomic
		}
	}
	else {
		return machine->nil;
	}

	// Should never reach this. Make compiler happy
	return NULL;
}