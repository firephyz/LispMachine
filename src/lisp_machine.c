#include "lisp_machine.h"
#include "expr_parser.h"
#include "repl.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

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
	machine->nil = get_free_cell();
	machine->nil->car = NULL;
	machine->nil->cdr = NULL;
	machine->nil->is_atom = true;

	// Initialize the supported instruction lists
	// null, false and true are pseudo system symbols. They get
	// translated to something else during parsing
	init_instr_list("atom? car cdr cons eq? false if lambda null quit quote true");

	// Initialize the machine system environment
	machine->sys_stack = machine->nil;
	machine->sys_stack_size = 0;

	if(verbose_flag) {
		printf("Machine initialized!\n");
	}

	return machine;
}

// Creates an array with all the names of the system functions.
// Expects a string containing all the function names
// each seperated by some whitespace. Must have no whitespace 
// surrounding the whole string
void init_instr_list(char * funcs) {

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

	char (*memory_block)[INSTR_MAX_LENGTH + 1] = malloc(sizeof(char) * (INSTR_MAX_LENGTH + 1) * func_count);
	char **instructions = malloc(sizeof(char *) * func_count);

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
		instructions[i] = (char *)&memory_block[i];
	}

	machine->instr_memory_block = memory_block;
	machine->instructions = instructions;
	machine->num_of_instrs = func_count;
}

Cell * push_system_args(Cell * stack, int arg_count, ...) {

	va_list args;
	va_start(args, arg_count);

	for(int i = 0; i < arg_count; ++i) {
		Cell * cell = get_free_cell();
		cell->car = va_arg(args, Cell *);
		cell->cdr = stack;
		++machine->sys_stack_size;
		stack = cell;
	}

	return stack;
}

void destroy_machine(Lisp_Machine *machine) {

	free(machine->instr_memory_block);
	free(machine->instructions);
	free(machine->memory_block);
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

// Implements the various system evalution functions using gotos so that
// we don't use the normal stack with normal function calls. We must use
// our own machine stack built from cons cells.
void execute() {

	Cell * code = make_expression("(quote a)");
	Cell * environment = make_expression("()");
	machine->current_args = push_system_args(machine->nil, 2, code, environment);

	// Used at temporary storage of current_args variables so that we
	// don't have to car and cdr so much. Might not be used
	// in actual hardware implementation.
	Cell * arg0;
	Cell * arg1;
	Cell * arg2;
	Cell * arg3;

sys_eval:
	arg0 = machine->current_args->car;
	arg1 = machine->current_args->cdr->car;

	if(arg0->is_atom) {
		goto sys_lookup;
	}
	else {
		switch(arg0->car->type) {
			case SYS_SYM_IF:
				machine->current_args = push_system_args(
					machine->nil,
					4, 
					arg0->cdr->car, 
					arg0->cdr->cdr->car, 
					arg0->cdr->cdr->cdr->car, 
					arg1);
				goto sys_evif;
			case SYS_SYM_LAMBDA:
				machine->result = arg0;
				break;
			case SYS_SYM_QUOTE:
				machine->result = arg0->cdr->car;
				break;
			default:
				// Push args for later access
				machine->sys_stack = push_system_args(machine->sys_stack, 2, arg0, arg1);
				// Set up the args for sys_evlis
				machine->current_args = push_system_args(machine->nil, 2, arg0->cdr, arg1);
				// Evaluate the function args
				goto sys_evlis;
sys_eval_evlis_continue:
				// Set up args for sys_apply
				machine->current_args = push_system_args(machine->nil, 3, machine->sys_stack->car->car, machine->result, machine->sys_stack->cdr->car);
				// Pop the stack
				machine->sys_stack = machine->sys_stack->cdr->cdr;
				goto sys_apply;
		}
	}

	// Return from sys_eval
sys_eval_return:
	switch(machine->calling_func) {
		case SYS_EVAL:;
		case SYS_APPLY:;
		case SYS_EVLIS:;
		case SYS_EVIF:;
		case SYS_CONENV:;
		case SYS_LOOKUP:;
		case SYS_REPL:;
		default:;
	}

sys_apply:
sys_evlis:

	arg0 = machine->current_args->car;
	arg1 = machine->current_args->cdr->car;

	if(arg0 == machine->nil) {
		machine->result = machine->nil;
	}
	else {
		machine->sys_stack = push_system_args(machine->sys_stack, 2, arg0, arg1);
		machine->current_args = push_system_args(machine->nil, 2, arg0->car, arg1);
		goto sys_eval;
sys_evlis_eval_continue:
		machine->sys_stack = push_system_args(machine->sys_stack, 1, machine->result);
		machine->current_args = push_system_args(machine->nil, 2, machine->sys_stack->cdr->car->cdr, machine->sys_stack->cdr->cdr->car);
		goto sys_evlis;
sys_evlis_evlis_continue:
		machine->result = cons(machine->sys_stack->car, machine->result);
		machine->sys_stack = machine->sys_stack->cdr->cdr->cdr;
	}

	// switch(machine->)

// (define evlis
//   (lambda (args env)
//     (if (eq? args (quote ()))
//         '()
//         (cons (eval (car args) env)
//               (evlis (cdr args) env)))))
sys_evif:;
sys_lookup:;
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